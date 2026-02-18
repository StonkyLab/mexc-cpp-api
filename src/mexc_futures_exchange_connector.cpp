/**
MEXC Exchange Connector

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#include <vk/mexc/mexc_futures_exchange_connector.h>
#include "vk/mexc/mexc_futures_rest_client.h"

namespace vk {

namespace {
/// Map common CandleInterval (seconds-based) to MEXC CandleInterval (name-based)
mexc::CandleInterval toMexcInterval(const CandleInterval interval) {
    switch (interval) {
        case CandleInterval::_1m: return mexc::CandleInterval::_1m;
        case CandleInterval::_5m: return mexc::CandleInterval::_5m;
        case CandleInterval::_15m: return mexc::CandleInterval::_15m;
        case CandleInterval::_30m: return mexc::CandleInterval::_30m;
        case CandleInterval::_1h: return mexc::CandleInterval::_60m;
        case CandleInterval::_4h: return mexc::CandleInterval::_4h;
        case CandleInterval::_8h: return mexc::CandleInterval::_8h;
        case CandleInterval::_1d: return mexc::CandleInterval::_1d;
        case CandleInterval::_1w: return mexc::CandleInterval::_1W;
        case CandleInterval::_1M: return mexc::CandleInterval::_1M;
        default:
            throw std::runtime_error("Unsupported candle interval for MEXC");
    }
}
}  // namespace

struct MEXCFuturesExchangeConnector::P {
    std::unique_ptr<mexc::futures::RESTClient> m_restClient{};
};

MEXCFuturesExchangeConnector::MEXCFuturesExchangeConnector() : m_p(std::make_unique<P>()) {
    m_p->m_restClient = std::make_unique<mexc::futures::RESTClient>("","");
}

MEXCFuturesExchangeConnector::~MEXCFuturesExchangeConnector() {
    m_p->m_restClient.reset();
}

std::string MEXCFuturesExchangeConnector::exchangeId() const {
    return std::string(magic_enum::enum_name(ExchangeId::MEXCFutures));
}

std::string MEXCFuturesExchangeConnector::version() const {
    return "2.0.0";
}

void MEXCFuturesExchangeConnector::setLoggerCallback(const onLogMessage& onLogMessageCB) {
}

void MEXCFuturesExchangeConnector::login(const std::tuple<std::string, std::string, std::string>& credentials) {
    m_p->m_restClient.reset();

    const auto& [apiKey, apiSecret, passPhrase] = credentials;

    if (apiSecret.empty()) {
        // WEB token authentication: apiKey contains the WEB session token
        m_p->m_restClient = std::make_unique<mexc::futures::RESTClient>(apiKey, mexc::futures::AuthSource::Web);
    } else {
        // Standard OpenAPI authentication
        m_p->m_restClient = std::make_unique<mexc::futures::RESTClient>(apiKey, apiSecret);
    }
}

Trade MEXCFuturesExchangeConnector::placeOrder(const Order& order) {
    mexc::futures::OrderRequest req;
    req.symbol = order.symbol;
    req.vol = order.quantity;

    // Map common Side + OrderType to MEXC-specific OrderSide
    // For futures: Buy = open long or close short, Sell = open short or close long
    // Without position context, assume opening new positions
    if (order.side == Side::Buy) {
        req.side = mexc::futures::OrderSide::OpenLong;
    } else {
        req.side = mexc::futures::OrderSide::OpenShort;
    }

    // Override side if customData contains "close" flag
    if (order.customData.contains("close") && order.customData["close"].get<bool>()) {
        if (order.side == Side::Buy) {
            req.side = mexc::futures::OrderSide::CloseShort;
        } else {
            req.side = mexc::futures::OrderSide::CloseLong;
        }
    }

    // Map order type
    switch (order.type) {
        case OrderType::Market:
            req.type = mexc::futures::OrderType::Market;
            break;
        case OrderType::Limit:
            req.type = mexc::futures::OrderType::Limit;
            req.price = order.price;
            break;
        default:
            req.type = mexc::futures::OrderType::Market;
            break;
    }

    // Map time in force
    if (order.timeInForce == TimeInForce::IOC) {
        req.type = mexc::futures::OrderType::IOC;
    } else if (order.timeInForce == TimeInForce::FOK) {
        req.type = mexc::futures::OrderType::FOK;
    }

    // Margin type and leverage from customData
    req.openType = mexc::futures::MarginType::Cross;
    if (order.customData.contains("openType")) {
        req.openType = static_cast<mexc::futures::MarginType>(order.customData["openType"].get<int>());
    }
    if (order.customData.contains("leverage")) {
        req.leverage = order.customData["leverage"].get<int>();
    }
    if (order.customData.contains("positionId")) {
        req.positionId = order.customData["positionId"].get<std::int64_t>();
    }

    const auto response = m_p->m_restClient->submitOrder(req);

    Trade trade;
    if (response.success) {
        trade.orderStatus = OrderStatus::New;
        trade.customData["orderId"] = response.orderId;
    } else {
        trade.orderStatus = OrderStatus::Rejected;
        trade.customData["errorCode"] = response.code;
    }

    return trade;
}

TickerPrice MEXCFuturesExchangeConnector::getTickerPrice(const std::string& symbol) const {
    const auto ticker = m_p->m_restClient->getContractTicker(symbol);

    TickerPrice retVal;
    retVal.bidPrice = ticker.bid1.convert_to<double>();
    retVal.askPrice = ticker.ask1.convert_to<double>();
    retVal.volume24h = ticker.volume24.convert_to<double>();
    retVal.turnover24h = ticker.amount24.convert_to<double>();
    retVal.time = ticker.timestamp;
    return retVal;
}

Balance MEXCFuturesExchangeConnector::getAccountBalance(const std::string& currency) const {
    const auto wb = m_p->m_restClient->getWalletBalance(currency);

    Balance retVal;
    retVal.balance = wb.equity.convert_to<double>();
    retVal.customData["availableBalance"] = wb.availableBalance.convert_to<double>();
    retVal.customData["cashBalance"] = wb.cashBalance.convert_to<double>();
    retVal.customData["frozenBalance"] = wb.frozenBalance.convert_to<double>();
    retVal.customData["unrealized"] = wb.unrealized.convert_to<double>();
    return retVal;
}

FundingRate MEXCFuturesExchangeConnector::getFundingRate(const std::string& symbol) const {
    const auto fr = m_p->m_restClient->getContractFundingRate(symbol);

    FundingRate retVal;
    retVal.symbol = fr.symbol;
    retVal.fundingRate = fr.fundingRate.convert_to<double>();
    retVal.fundingTime = fr.nextSettleTime;
    return retVal;
}

std::vector<FundingRate> MEXCFuturesExchangeConnector::getFundingRates() const {
    std::vector<FundingRate> retVal;

    for (const auto& rate : m_p->m_restClient->getContractFundingRates()) {
        FundingRate fr;
        fr.symbol = rate.symbol;
        fr.fundingRate = rate.fundingRate.convert_to<double>();
        fr.fundingTime = rate.nextSettleTime;
        retVal.push_back(fr);
    }

    return retVal;
}

std::vector<Symbol> MEXCFuturesExchangeConnector::getSymbolInfo(const std::string& symbol) const {
    throw std::runtime_error("Unimplemented: MEXCFuturesExchangeConnector::getSymbolInfo");
}

std::int64_t MEXCFuturesExchangeConnector::getServerTime() const {
    return m_p->m_restClient->getServerTime();
}

std::vector<Position> MEXCFuturesExchangeConnector::getPositionInfo(const std::string& symbol) const {
    const auto positions = m_p->m_restClient->getOpenPositions(symbol);
    std::vector<Position> retVal;
    retVal.reserve(positions.size());

    for (const auto& pos : positions) {
        Position p;
        p.symbol = pos.symbol;
        p.side = (pos.positionType == 1) ? Side::Buy : Side::Sell;
        p.size = pos.holdVol.convert_to<double>();
        p.avgPrice = pos.openAvgPrice.convert_to<double>();
        p.value = p.size * p.avgPrice;
        p.createdTime = pos.createTime;
        p.updatedTime = pos.updateTime;
        p.leverage = static_cast<double>(pos.leverage);
        retVal.push_back(p);
    }

    return retVal;
}

std::vector<FundingRate> MEXCFuturesExchangeConnector::getHistoricalFundingRates(
    const std::string& symbol, const std::int64_t startTime, const std::int64_t endTime) const {
    std::vector<FundingRate> retVal;

    // MEXC uses paginated API, fetch all pages
    std::int32_t pageNum = 1;
    constexpr std::int32_t pageSize = 1000;

    while (true) {
        const auto page = m_p->m_restClient->getContractFundingRateHistory(symbol, pageNum, pageSize);

        for (const auto& rate : page.resultList) {
            // Filter by time range (MEXC doesn't support time range params directly)
            if (rate.settleTime >= startTime && rate.settleTime <= endTime) {
                FundingRate fr;
                fr.symbol = rate.symbol;
                fr.fundingRate = rate.fundingRate.convert_to<double>();
                fr.fundingTime = rate.settleTime;
                retVal.push_back(fr);
            }
        }

        if (pageNum >= page.totalPage || page.resultList.empty()) {
            break;
        }

        // If the oldest rate in this page is before our startTime, no need to fetch more
        if (!page.resultList.empty() && page.resultList.back().settleTime < startTime) {
            break;
        }

        ++pageNum;
    }

    // Sort ascending by fundingTime
    std::ranges::sort(retVal, [](const auto& a, const auto& b) {
        return a.fundingTime < b.fundingTime;
    });

    return retVal;
}

std::vector<Candle> MEXCFuturesExchangeConnector::getHistoricalCandles(
    const std::string& symbol, const CandleInterval interval,
    const std::int64_t startTime, const std::int64_t endTime) const {

    const auto mexcInterval = toMexcInterval(interval);

    // MEXC REST client expects timestamps in seconds, interface uses ms
    const auto candles = m_p->m_restClient->getHistoricalPrices(
        symbol, mexcInterval, startTime / 1000, endTime / 1000);

    std::vector<Candle> retVal;
    retVal.reserve(candles.size());

    for (const auto& c : candles) {
        Candle candle;
        candle.openTime = c.openTime;  // Already in ms (REST client converts)
        candle.open = c.open.convert_to<double>();
        candle.high = c.high.convert_to<double>();
        candle.low = c.low.convert_to<double>();
        candle.close = c.close.convert_to<double>();
        candle.volume = c.volume.convert_to<double>();
        retVal.push_back(candle);
    }

    return retVal;
}

std::vector<mexc::futures::ContractDetail> MEXCFuturesExchangeConnector::getContractDetails(const std::string &symbol) const {
    return m_p->m_restClient->getContractDetails(symbol);
}
} // namespace vk
