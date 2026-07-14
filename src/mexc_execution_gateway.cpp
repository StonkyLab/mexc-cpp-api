/**
MEXC Execution Gateway

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2026 Vitezslav Kot <vitezslav.kot@stonky.cz>, Stonky s.r.o.
*/

#include "stonky/mexc/mexc_execution_gateway.h"
#include "stonky/mexc/mexc_futures_rest_client.h"
#include "stonky/mexc/mexc_ws_private_stream_manager.h"
#include "stonky/mexc/mexc_ws_stream_manager.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>
#include <map>
#include <mutex>
#include <thread>

namespace stonky::execution {
using namespace stonky::mexc::futures;
using SteadyClock = std::chrono::steady_clock;
using SteadyTime = SteadyClock::time_point;

namespace {
void logForwarder(const LogSeverity severity, const std::string &message) {
    switch (severity) {
        case LogSeverity::Info:
            spdlog::info(message);
            break;
        case LogSeverity::Warning:
            spdlog::warn(message);
            break;
        case LogSeverity::Critical:
        case LogSeverity::Error:
            spdlog::error(message);
            break;
        default:
            spdlog::debug(message);
            break;
    }
}

std::string toLower(std::string s) {
    std::ranges::transform(s, s.begin(), [](const unsigned char c) { return std::tolower(c); });
    return s;
}

/// Decimal places implied by a tick/step (0.1 → 1, 0.01 → 2, 1 → 0).
int decimalsOf(const double step) {
    if (step <= 0.0) {
        return 8;
    }

    int d = 0;
    double t = step;

    while (d < 12 && std::abs(t - std::round(t)) > 1e-9) {
        t *= 10.0;
        d++;
    }

    return d;
}

/// Round to N decimals via a string round-trip so the resulting double
/// serializes WITHOUT floating-point noise (e.g. 61394.200000000004 → 61394.2).
/// MEXC rejects (code 2015) a price/qty with excess decimals; OrderRequest::toJson
/// sends the raw double, so the value must already be clean.
double roundToDecimals(const double v, const int decimals) {
    return std::stod(fmt::format("{:.{}f}", v, decimals));
}

/// Classify a MEXC reject (the RESTClient error carries "code: N, msg: ...").
/// Message-based first (robust), code fallback. The exact post-only-cross
/// string/code is confirmed against a live funded fill test; until then a
/// would-cross reject falls through to Hard and is simply re-posted after
/// backoff, which is safe (never a fatal cap on its own for a moving book).
RejectKind classifyReject(const std::string &reason) {
    const auto r = toLower(reason);

    /// 510 "Requests are too frequent" — a venue throttle (CloudFront/WAF), NOT
    /// a rejection of the order's content. The HTTP session already retried it 4×
    /// before it surfaced here, so the congestion is real: back off, don't count
    /// it toward the fatal cap, don't resubmit into the storm.
    /// 513 "Invalid request, please try again later" is 510's sibling: every
    /// live occurrence (4/4) landed at the tail of an exhausted 510 retry
    /// ladder, where the re-sent Request-Time was ~11 s stale — congestion, not
    /// order content ("please try again later"; the same order fills seconds
    /// later). Match the full transient phrase, not bare "invalid request",
    /// so a genuine parameter error still classifies Hard.
    if (r.find("code: 510") != std::string::npos || r.find("too frequent") != std::string::npos || r.find("too many requests") != std::string::npos ||
        r.find("code: 513") != std::string::npos || r.find("invalid request, please try again later") != std::string::npos) {
        return RejectKind::Throttled;
    }

    /// 2009 "Position is nonexistent or closed" — only a reduce/close order draws
    /// this, and it means the position is already gone (goal met). Ending the leg
    /// cleanly beats looping on the same reject to the 20-hard-reject cap
    /// (live-observed: ~500 s burned on one SPELL_USDT cleanup leg).
    if (r.find("code: 2009") != std::string::npos || r.find("nonexistent") != std::string::npos) {
        return RejectKind::PositionClosed;
    }

    if (r.find("maker") != std::string::npos && (r.find("match") != std::string::npos || r.find("immediately") != std::string::npos || r.find("take") != std::string::npos)) {
        return RejectKind::BenignPostOnlyCross; // post-only would take liquidity
    }

    if (r.find("insufficient") != std::string::npos || r.find("margin") != std::string::npos || r.find("balance") != std::string::npos) {
        return RejectKind::Hard; // not self-resolving, but not a permanent config error
    }

    if (r.find("min") != std::string::npos && (r.find("vol") != std::string::npos || r.find("amount") != std::string::npos || r.find("size") != std::string::npos)) {
        return RejectKind::MinNotional;
    }

    /// "risk management"/"risk quota"/"risk limit" only — a bare "risk" would
    /// also match a transient message that merely mentions risk and abort the
    /// leg on reject #1 (Permanent is fatal for the chase).
    if (r.find("not activated") != std::string::npos || r.find("prohibited") != std::string::npos || r.find("not allowed") != std::string::npos ||
        r.find("not exist") != std::string::npos || r.find("risk management") != std::string::npos || r.find("risk quota") != std::string::npos ||
        r.find("risk limit") != std::string::npos || r.find("restricted") != std::string::npos ||
        r.find("code: 6002") != std::string::npos || r.find("code: 8950") != std::string::npos || r.find("country or region") != std::string::npos ||
        r.find("only close existing") != std::string::npos) {
        /// 6002 "futures position-opening has been restricted" — account-level
        /// block; 8950 "unavailable in your country or region ... only close
        /// existing" — region block. Both refuse every OPEN, so fail the leg fast
        /// (fatalError on reject #1) instead of burning the 20-reject cap — the
        /// executor then learns and persists the block from result.error.
        return RejectKind::Permanent;
    }

    return RejectKind::Hard;
}

/// Classify the private order channel's numeric errorCode. The substring
/// classifier above cannot parse "errorCode=NN" strings, so async rejects map
/// here. NOTE this is the order-object code space (docs: 0 normal, 1 parameter,
/// 2 balance insufficient, 3 position does not exist, 4 position insufficient,
/// 5/6 price constraints, 7 exceed risk quota, 8 system canceled) — NOT the
/// REST error space (510/2009/8950 can never arrive here). Codes observed live
/// beyond the documented table: 20 = post-only would take liquidity.
RejectKind classifyErrorCode(const std::int32_t errorCode) {
    switch (errorCode) {
        case 20:
            return RejectKind::BenignPostOnlyCross;
        case 3: /// "position does not exist" — the async twin of REST 2009: a
                /// reduce whose position is already gone; goal met, end cleanly
                /// instead of looping to the 20-hard-reject cap.
            return RejectKind::PositionClosed;
        case 7: /// "exceed risk quota restrictions" — resubmitting the same
                /// size cannot self-resolve within a chase.
            return RejectKind::Permanent;
        case 6002: /// position-opening restricted — account-level block
            return RejectKind::Permanent;
        default: /// incl. 4 "position insufficient": NOT PositionClosed — some
                 /// position remains, a benign "goal met" would mask it.
            return RejectKind::Hard;
    }
}

/// A cancel of an order that already left the book — not a failure.
bool isOrderGoneReason(const std::string &reason) {
    const auto r = toLower(reason);
    return r.find("not exist") != std::string::npos || r.find("does not exist") != std::string::npos || r.find("already") != std::string::npos ||
           r.find("code: 2013") != std::string::npos || r.find("code: 2040") != std::string::npos;
}
} // namespace

struct MexcExecutionGateway::P {
    std::unique_ptr<RESTClient> restClient;
    std::unique_ptr<WSPrivateStreamManager> privateStream;
    std::unique_ptr<WSStreamManager> publicStream;

    onOrderUpdateEvent orderUpdateCB;
    onFillEvent fillCB;
    onQuoteEvent quoteCB;

    std::mutex specM;
    std::map<std::string, InstrumentSpec> specCache;
    std::map<std::string, double> contractSize; ///< base per contract, per symbol
    std::map<std::string, int> priceDecimals;   ///< venue priceScale (price decimals), per symbol
    std::map<std::string, double> tickSize;     ///< price tick (priceUnit), per symbol
    std::map<std::string, double> volUnit;      ///< order size step in CONTRACTS, per symbol

    /// MEXC applies a much tighter effective quota to private order endpoints
    /// than to generic REST calls. All chase legs share this lane: one in-flight
    /// submit/cancel at a time and at least this spacing between starts. A caller
    /// which loses the lane gets a local Throttled result and backs off in the
    /// chase core WITHOUT sending a request to MEXC.
    std::mutex orderLaneM;
    SteadyTime nextOrderRequestAt{};
    SteadyTime orderCooldownUntil{};
    int consecutiveOrderThrottles{};
    static constexpr auto orderRequestSpacing = std::chrono::milliseconds(1250);

    [[nodiscard]] std::unique_lock<std::mutex> acquireOrderLane() {
        std::unique_lock lk(orderLaneM, std::try_to_lock);

        if (!lk.owns_lock()) {
            throw GatewayError(RejectKind::Throttled, "MEXC local order lane busy — request not sent");
        }

        const auto now = SteadyClock::now();

        if (now < orderCooldownUntil) {
            throw GatewayError(RejectKind::Throttled, "MEXC local order lane cooldown — request not sent");
        }

        if (now < nextOrderRequestAt) {
            throw GatewayError(RejectKind::Throttled, "MEXC local order lane pacing — request not sent");
        }

        nextOrderRequestAt = now + orderRequestSpacing;
        return lk;
    }

    /// Caller owns orderLaneM. A venue 510 is shared congestion, not a defect
    /// of one symbol: stop every other leg from retrying into the same window.
    void extendOrderCooldownAfterThrottle() {
        consecutiveOrderThrottles = std::min(consecutiveOrderThrottles + 1, 4);
        const auto seconds = 5LL << (consecutiveOrderThrottles - 1); // 5/10/20/40 → capped below
        const auto cooldown = std::chrono::seconds(std::min(seconds, 30LL));
        const auto until = SteadyClock::now() + cooldown;
        orderCooldownUntil = std::max(orderCooldownUntil, until);
        nextOrderRequestAt = std::max(nextOrderRequestAt, until);
    }

    /// Caller owns orderLaneM. A successful private order operation proves the
    /// cooldown has cleared, so the next real 510 starts from the short delay.
    void clearOrderThrottleStreak() { consecutiveOrderThrottles = 0; }

    [[nodiscard]] int priceDecimalsFor(const std::string &symbol) {
        std::lock_guard lk(specM);

        if (const auto it = priceDecimals.find(symbol); it != priceDecimals.end()) {
            return it->second;
        }

        return 2;
    }

    [[nodiscard]] double tickFor(const std::string &symbol) {
        std::lock_guard lk(specM);

        if (const auto it = tickSize.find(symbol); it != tickSize.end()) {
            return it->second;
        }

        return 0.0;
    }

    [[nodiscard]] double volUnitFor(const std::string &symbol) {
        std::lock_guard lk(specM);

        if (const auto it = volUnit.find(symbol); it != volUnit.end()) {
            return it->second;
        }

        return 1.0;
    }

    /// Cumulative filled contracts already emitted per order — fills are driven
    /// from the order channel (see the order handler) and this dedups them.
    std::mutex fillM;
    std::map<std::int64_t, double> accountedDealVol;

    /// Return the newly-filled contracts for an order given its cumulative
    /// dealVol (0 when nothing new). Idempotent against event re-delivery.
    [[nodiscard]] double newlyFilled(const std::int64_t orderId, const double cumDealVol) {
        std::lock_guard lk(fillM);
        double &acc = accountedDealVol[orderId];

        if (cumDealVol <= acc) {
            return 0.0;
        }

        const double inc = cumDealVol - acc;
        acc = cumDealVol;
        return inc;
    }

    [[nodiscard]] double contractSizeFor(const std::string &symbol) {
        std::lock_guard lk(specM);

        if (const auto it = contractSize.find(symbol); it != contractSize.end()) {
            return it->second;
        }

        return 0.0;
    }

    /// The venue reported an order gone at cancel time — gone can mean FILLED.
    /// If the private WS dropped that fill (reconnect gap, 2× live-observed),
    /// the core's accounting is stale and its next submit would OVERFILL the
    /// target. Pull venue truth by externalOid and credit anything unaccounted
    /// through the normal fill path — fillId dedup (same "orderId-dealVol" key
    /// as the WS path) makes re-delivery harmless.
    void reconcileMissedFills(const std::string &clientOrderId, const std::string &symbol) {
        if (!fillCB) {
            return;
        }

        try {
            const auto order = restClient->getOrderByExternalOid(symbol, clientOrderId);

            if (order.dealVol <= 0.0) {
                return;
            }

            const double cs = contractSizeFor(symbol);
            const double csEff = cs > 0.0 ? cs : 1.0;

            if (const double inc = newlyFilled(order.orderId, order.dealVol); inc > 0.0) {
                FillEvent fill;
                fill.clientOrderId = clientOrderId;
                fill.symbol = symbol;
                fill.fillId = fmt::format("{}-{}", order.orderId, order.dealVol);
                fill.qty = inc * csEff;
                fill.price = order.dealAvgPrice > 0.0 ? order.dealAvgPrice : order.price;
                fill.isMaker = true;

                spdlog::warn("MexcGW: {} order {} gone at cancel with {} contracts unaccounted — fill credited from REST (private WS gap?)", symbol, clientOrderId, inc);
                fillCB(fill);
            }
        } catch (std::exception &e) {
            spdlog::warn("MexcGW: {} fill reconciliation for {} failed ({}) — verify the position; a WS-gap fill may be uncredited", symbol, clientOrderId, e.what());
        }
    }

    static Quote toQuote(const EventTicker &ticker) {
        Quote quote;
        quote.bid = ticker.bid1;
        quote.ask = ticker.ask1;
        quote.receivedAt = std::chrono::steady_clock::time_point(std::chrono::milliseconds(ticker.receivedTimestamp));
        return quote;
    }

    /// Buy/Sell + reduceOnly → MEXC's four-way position side (one-way account).
    /// Param is the interface side (execution::OrderSide, from the current
    /// namespace); return is the MEXC side (explicitly qualified).
    static mexc::futures::OrderSide toMexcSide(const OrderSide side, const bool reduceOnly) {
        if (side == OrderSide::Buy) {
            return reduceOnly ? mexc::futures::OrderSide::CloseShort : mexc::futures::OrderSide::OpenLong;
        }

        return reduceOnly ? mexc::futures::OrderSide::CloseLong : mexc::futures::OrderSide::OpenShort;
    }
};

MexcExecutionGateway::MexcExecutionGateway(const std::string &apiKey, const std::string &apiSecret) : m_p(std::make_unique<P>()) {
    m_p->restClient = std::make_unique<RESTClient>(apiKey, apiSecret);
    m_p->privateStream = std::make_unique<WSPrivateStreamManager>(apiKey, apiSecret);
    m_p->publicStream = std::make_unique<WSStreamManager>();

    m_p->privateStream->setLoggerCallback(&logForwarder);
    m_p->publicStream->setLoggerCallback(&logForwarder);
    m_p->publicStream->setTimeout(1);

    m_p->publicStream->setTickerUpdateCallback([this](const EventTicker &ticker) {
        if (m_p->quoteCB) {
            m_p->quoteCB(ticker.symbol, P::toQuote(ticker));
        }
    });

    m_p->privateStream->setOrderUpdateCallback([this](const EventOrder &event) {
        spdlog::debug("MexcGW order event: {} state={} extId={} orderId={} price={} vol={} dealVol={} err={}", event.symbol, static_cast<int>(event.state),
                      event.externalOid, event.orderId, event.price, event.vol, event.dealVol, event.errorCode);

        if (event.externalOid.empty()) {
            return;
        }

        const double cs = m_p->contractSizeFor(event.symbol);
        const double csEff = cs > 0.0 ? cs : 1.0;

        /// Emit fills from the ORDER channel's cumulative dealVol, BEFORE the
        /// state update below. MEXC's separate deal channel lags the order
        /// event; crediting the fill here (atomic with the state) means the
        /// core has the qty before Filled/Cancelled clears the active order —
        /// otherwise the chase re-submits in the gap and overfills the target.
        if (m_p->fillCB && event.dealVol > 0.0) {
            if (const double inc = m_p->newlyFilled(event.orderId, event.dealVol); inc > 0.0) {
                FillEvent fill;
                fill.clientOrderId = event.externalOid;
                fill.symbol = event.symbol;
                fill.fillId = fmt::format("{}-{}", event.orderId, event.dealVol); // unique per increment
                fill.qty = inc * csEff;                                            // contracts → base
                fill.price = event.dealAvgPrice > 0.0 ? event.dealAvgPrice : event.price;
                fill.isMaker = true;                                               // post-only orders are makers
                m_p->fillCB(fill);
            }
        }

        /// Terminal order → drop its fill-dedup entry (unbounded growth
        /// otherwise; a late duplicate event re-creates it harmlessly — the
        /// core dedups by fillId and its routes are swept at leg teardown).
        if (event.state == mexc::futures::OrderState::Completed || event.state == mexc::futures::OrderState::Cancelled ||
            event.state == mexc::futures::OrderState::Invalid) {
            std::lock_guard lk(m_p->fillM);
            m_p->accountedDealVol.erase(event.orderId);
        }

        if (!m_p->orderUpdateCB) {
            return;
        }

        OrderUpdate update;
        update.clientOrderId = event.externalOid;
        update.symbol = event.symbol;
        update.price = event.price;
        update.cumFilledQty = event.dealVol * csEff;
        update.reason = fmt::format("errorCode={}", event.errorCode);

        switch (event.state) {
            case mexc::futures::OrderState::New:
            case mexc::futures::OrderState::Uncompleted:
                update.state = OrderState::Accepted;
                break;
            case mexc::futures::OrderState::Completed:
                update.state = OrderState::Filled;
                break;
            case mexc::futures::OrderState::Cancelled:
                /// MEXC delivers a post-only cross as Cancelled + errorCode 20 —
                /// translate to a benign reject so the chase applies its cross
                /// backoff instead of a plain re-submit.
                if (event.errorCode == 20) {
                    update.state = OrderState::Rejected;
                    update.rejectKind = RejectKind::BenignPostOnlyCross;
                } else {
                    update.state = OrderState::Cancelled;
                }
                break;
            case mexc::futures::OrderState::Invalid:
                update.state = OrderState::Rejected;
                update.rejectKind = classifyErrorCode(event.errorCode);
                break;
            default:
                return;
        }

        m_p->orderUpdateCB(update);
    });

    /// The deal channel is metadata only now — fills are accounted from the
    /// order channel (above) to keep them atomic with the state change.
    m_p->privateStream->setDealCallback([this](const EventDeal &event) {
        spdlog::debug("MexcGW deal: {} orderId={} vol={} px={} maker={}", event.symbol, event.orderId, event.vol, event.price, !event.isTaker);
    });
}

MexcExecutionGateway::~MexcExecutionGateway() = default;

std::string MexcExecutionGateway::name() const { return "MEXC"; }

void MexcExecutionGateway::start() {
    m_p->privateStream->connect();

    for (int i = 0; i < 100; ++i) {
        if (m_p->privateStream->isAuthenticated()) {
            spdlog::info("MexcExecutionGateway: private stream authenticated");
            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    throw std::runtime_error("MexcExecutionGateway: private stream login timeout (10 s)");
}

InstrumentSpec MexcExecutionGateway::instrumentSpec(const std::string &symbol) {
    {
        std::lock_guard lk(m_p->specM);

        if (const auto it = m_p->specCache.find(symbol); it != m_p->specCache.end()) {
            return it->second;
        }
    }

    for (const auto details = m_p->restClient->getContractDetails(symbol); const auto &detail: details) {
        if (detail.symbol == symbol) {
            const double cs = detail.contractSize > 0.0 ? detail.contractSize : 1.0;

            InstrumentSpec spec;
            spec.symbol = symbol;
            spec.tickSize = detail.priceUnit;                              // actual price tick (e.g. 0.1 USDT)
            spec.qtyStep = static_cast<double>(detail.volUnit) * cs;       // one vol step in base units
            spec.minQty = static_cast<double>(detail.minVol) * cs;
            spec.maxQty = static_cast<double>(detail.maxVol) * cs;
            spec.minNotional = 0.0;                                        // MEXC has no min order value; minVol (→ minQty) is the floor

            std::lock_guard lk(m_p->specM);
            m_p->specCache[symbol] = spec;
            m_p->contractSize[symbol] = cs;
            /// Venue-authoritative decimals (contract "priceScale"); deriving
            /// them from the tick is the fallback for a missing field only —
            /// the venue rejects any excess decimal with 2015, even one it
            /// could round itself.
            m_p->priceDecimals[symbol] = detail.pricePrecision > 0 ? detail.pricePrecision : decimalsOf(detail.priceUnit);
            m_p->tickSize[symbol] = detail.priceUnit;
            m_p->volUnit[symbol] = detail.volUnit > 0 ? static_cast<double>(detail.volUnit) : 1.0;
            return spec;
        }
    }

    throw std::runtime_error(fmt::format("MEXC: unknown instrument {}", symbol));
}

void MexcExecutionGateway::refreshInstruments() {
    /// Specs are lazily fetched per symbol from getContractDetails — dropping
    /// the cache makes the next instrumentSpec() pull fresh venue parameters.
    /// contractSize/priceDecimals stay (event parsing needs last-known values)
    /// and are overwritten on the refetch.
    std::lock_guard lk(m_p->specM);
    m_p->specCache.clear();
}

void MexcExecutionGateway::subscribeQuotes(const std::string &symbol) { m_p->publicStream->subscribeTickerStream(symbol); }

void MexcExecutionGateway::unsubscribeQuotes(const std::string &symbol) { m_p->publicStream->unsubscribeTickerStream(symbol); }

std::optional<Quote> MexcExecutionGateway::lastQuote(const std::string &symbol) {
    if (const auto ticker = m_p->publicStream->readEventTicker(symbol)) {
        return P::toQuote(*ticker);
    }

    return std::nullopt;
}

void MexcExecutionGateway::setOrderUpdateCallback(const onOrderUpdateEvent &cb) { m_p->orderUpdateCB = cb; }

void MexcExecutionGateway::setFillCallback(const onFillEvent &cb) { m_p->fillCB = cb; }

void MexcExecutionGateway::setQuoteCallback(const onQuoteEvent &cb) { m_p->quoteCB = cb; }

void MexcExecutionGateway::submitPostOnlyLimit(const std::string &clientOrderId, const std::string &symbol, const OrderSide side, const double qty, const double price,
                                               const bool reduceOnly) {
    const double cs = m_p->contractSizeFor(symbol);

    if (cs <= 0.0) {
        throw GatewayError(RejectKind::Hard, fmt::format("MEXC: no contractSize for {} (instrumentSpec not fetched)", symbol));
    }

    OrderRequest req;
    req.symbol = symbol;

    /// Last line of defense against 2015 "Price or quantity precision error":
    /// the chase prices off the spec it fetched at LEG START — snap to the
    /// gateway's CURRENT tick grid and round to the venue priceScale, so a
    /// mid-chase metadata change cannot send an off-grid value. The venue
    /// rejects rather than rounds, even at the 6th decimal.
    double px = price;
    if (const double tick = m_p->tickFor(symbol); tick > 0.0) {
        px = std::round(px / tick) * tick;
    }
    req.price = roundToDecimals(px, m_p->priceDecimalsFor(symbol));

    double vol = std::round(qty / cs);              // base → whole contracts
    if (const double vu = m_p->volUnitFor(symbol); vu > 1.0) {
        vol = std::floor(vol / vu + 1e-9) * vu;     // whole volUnit multiples; never round an order UP
    }

    if (vol <= 0.0) {
        throw GatewayError(RejectKind::Hard, fmt::format("MEXC: qty {} is below one volUnit for {} — not submitting", qty, symbol));
    }

    req.vol = vol;
    req.side = P::toMexcSide(side, reduceOnly);
    req.type = OrderType::PostOnly;
    req.openType = MarginType::Cross;               // leverage is an account-side setting
    req.externalOid = clientOrderId;

    /// Keep the permit alive for the full REST call (including MEXC's internal
    /// 510 retries), so another leg cannot slip a submit into the same storm.
    [[maybe_unused]] auto orderLane = m_p->acquireOrderLane();

    try {
        const auto response = m_p->restClient->submitOrder(req);
        m_p->clearOrderThrottleStreak();
        spdlog::debug("MexcGW submit ok: {} {} vol={} px={} extId={} orderId={}", symbol, static_cast<int>(req.side), req.vol, price, clientOrderId, response.orderId);
    } catch (std::exception &e) {
        const auto kind = classifyReject(e.what());

        /// acquireOrderLane() is intentionally outside the REST try/catch so a
        /// local Throttled result preserves its classification. A real 510,
        /// however, extends the shared cooldown for every currently-running leg.
        if (kind == RejectKind::Throttled) {
            /// The REST call held orderLaneM for its whole lifetime.
            m_p->extendOrderCooldownAfterThrottle();
        }

        refetchSpecAfterPrecisionReject(symbol, e.what());

        spdlog::debug("MexcGW submit reject: {} {} vol={} px={} extId={} — {}", symbol, static_cast<int>(req.side), req.vol, req.price, clientOrderId, e.what());
        throw GatewayError(kind, e.what());
    }
}

void MexcExecutionGateway::refetchSpecAfterPrecisionReject(const std::string &symbol, const std::string &reason) {
    if (const auto r = toLower(reason); r.find("code: 2015") == std::string::npos && r.find("precision") == std::string::npos) {
        return;
    }

    {
        std::lock_guard lk(m_p->specM);
        m_p->specCache.erase(symbol);
    }

    try {
        [[maybe_unused]] const auto fresh = instrumentSpec(symbol);
        spdlog::warn("MexcGW: {} precision reject (2015) — contract spec refetched: tick {:.10g}, priceScale {}, volUnit {:.6g}", symbol, m_p->tickFor(symbol),
                     m_p->priceDecimalsFor(symbol), m_p->volUnitFor(symbol));
    } catch (std::exception &refetchErr) {
        spdlog::warn("MexcGW: {} precision reject (2015) — spec refetch failed: {}", symbol, refetchErr.what());
    }
}

bool MexcExecutionGateway::supportsAmend() const { return false; }

void MexcExecutionGateway::amendPrice(const std::string &, const std::string &, double) {
    throw std::runtime_error("MEXC has no amend endpoint (supportsAmend() is false)");
}

bool MexcExecutionGateway::cancel(const std::string &clientOrderId, const std::string &symbol) {
    [[maybe_unused]] auto orderLane = m_p->acquireOrderLane();

    try {
        [[maybe_unused]] const auto resp = m_p->restClient->cancelOrderByExternalOid(symbol, clientOrderId);
        m_p->clearOrderThrottleStreak();
        return true;
    } catch (std::exception &e) {
        if (isOrderGoneReason(e.what())) {
            /// "Gone" can mean FILLED — if the private WS dropped the fill,
            /// the core would resubmit the full remaining and overfill.
            /// Reconcile from REST before reporting the order gone.
            m_p->reconcileMissedFills(clientOrderId, symbol);
            return false;
        }

        const auto kind = classifyReject(e.what());

        if (kind == RejectKind::Throttled) {
            m_p->extendOrderCooldownAfterThrottle();
        }

        throw GatewayError(kind, e.what());
    }
}

void MexcExecutionGateway::submitReduceOnlyMarket(const std::string &clientOrderId, const std::string &symbol, const OrderSide side, const double qty) {
    const double cs = m_p->contractSizeFor(symbol);

    if (cs <= 0.0) {
        throw GatewayError(RejectKind::Hard, fmt::format("MEXC: no contractSize for {}", symbol));
    }

    OrderRequest req;
    req.symbol = symbol;
    req.price = 0.0;

    double vol = std::round(qty / cs);
    if (const double vu = m_p->volUnitFor(symbol); vu > 1.0) {
        vol = std::floor(vol / vu + 1e-9) * vu;     // whole volUnit multiples; never round an order UP
    }

    if (vol <= 0.0) {
        throw GatewayError(RejectKind::Hard, fmt::format("MEXC: qty {} is below one volUnit for {} — not submitting", qty, symbol));
    }

    req.vol = vol;
    req.side = P::toMexcSide(side, true);           // reduce-only close side
    req.type = OrderType::Market;
    req.openType = MarginType::Cross;
    req.externalOid = clientOrderId;

    [[maybe_unused]] auto orderLane = m_p->acquireOrderLane();

    try {
        [[maybe_unused]] const auto response = m_p->restClient->submitOrder(req);
        m_p->clearOrderThrottleStreak();
    } catch (std::exception &e) {
        const auto kind = classifyReject(e.what());

        if (kind == RejectKind::Throttled) {
            m_p->extendOrderCooldownAfterThrottle();
        }

        refetchSpecAfterPrecisionReject(symbol, e.what());

        throw GatewayError(kind, e.what());
    }
}

} // namespace stonky::execution
