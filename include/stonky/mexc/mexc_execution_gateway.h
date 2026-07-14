/**
MEXC Execution Gateway

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2026 Vitezslav Kot <vitezslav.kot@stonky.cz>, Stonky s.r.o.
*/

#ifndef DIRTY_CARRY_MEXC_EXECUTION_GATEWAY_H
#define DIRTY_CARRY_MEXC_EXECUTION_GATEWAY_H

#include <stonky/interface/i_execution_gateway.h>
#include <memory>

namespace stonky::execution {

/**
 * IExecutionGateway adapter over mexc-cpp-api: futures REST for order ops and
 * instrument metadata, private WS (order + deal channels) for events, public WS
 * ticker stream for top-of-book quotes.
 *
 * MEXC specifics handled inside the adapter (never leak to the chase core):
 *  - **Contract units**: MEXC trades in contracts; the core is told BASE-asset
 *    quantities and the gateway converts to/from contracts via contractSize.
 *  - **No amend endpoint**: supportsAmend() is false, so the core re-prices via
 *    cancel + resubmit.
 *  - **Four-way side**: Buy/Sell + reduceOnly map to OpenLong/OpenShort/
 *    CloseLong/CloseShort. Account must be one-way, Cross margin; leverage is an
 *    account-side setting (not sent per order).
 *  - **Deal events lack the external oid**: routed via an orderId → clientOrderId
 *    map kept from the submit response and the order channel.
 *
 * There is no MEXC testnet — everything hits production (contract.mexc.com).
 */
class MexcExecutionGateway final : public IExecutionGateway {
    struct P;
    std::unique_ptr<P> m_p{};

    /// 2015 "Price or quantity precision error" means our cached grid metadata
    /// (tick/priceScale/volUnit) disagrees with the venue — scale changes land
    /// intraday. Drop the symbol's cached spec and refetch NOW so subsequent
    /// reposts of the running chase price on the fresh grid. No-op unless the
    /// reason is a precision reject.
    void refetchSpecAfterPrecisionReject(const std::string &symbol, const std::string &reason);

public:
    MexcExecutionGateway(const std::string &apiKey, const std::string &apiSecret);

    ~MexcExecutionGateway() override;

    [[nodiscard]] std::string name() const override;

    void start() override;

    InstrumentSpec instrumentSpec(const std::string &symbol) override;

    void refreshInstruments() override;

    void subscribeQuotes(const std::string &symbol) override;

    void unsubscribeQuotes(const std::string &symbol) override;

    std::optional<Quote> lastQuote(const std::string &symbol) override;

    void setOrderUpdateCallback(const onOrderUpdateEvent &cb) override;

    void setFillCallback(const onFillEvent &cb) override;

    void setQuoteCallback(const onQuoteEvent &cb) override;

    void submitPostOnlyLimit(const std::string &clientOrderId, const std::string &symbol, OrderSide side, double qty, double price, bool reduceOnly) override;

    [[nodiscard]] bool supportsAmend() const override;

    void amendPrice(const std::string &clientOrderId, const std::string &symbol, double price) override;

    bool cancel(const std::string &clientOrderId, const std::string &symbol) override;

    void submitReduceOnlyMarket(const std::string &clientOrderId, const std::string &symbol, OrderSide side, double qty) override;
};

} // namespace stonky::execution

#endif // DIRTY_CARRY_MEXC_EXECUTION_GATEWAY_H
