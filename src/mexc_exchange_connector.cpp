/**
Binance Exchange Connector

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#include <vk/mexc/mexc_exchange_connector.h>
#include "vk/mexc/mexc_futures_rest_client.h"
#include "vk/mexc/mexc_futures_ws_client.h"

namespace vk {
struct MEXCFuturesExchangeConnector::P {
    std::unique_ptr<mexc::futures::RESTClient> m_restClient{};
    std::unique_ptr<mexc::futures::WSClient> m_wsClient{};
    bool m_demo = true;
    std::map<std::string, onTickerPriceEvent> m_tickerStreamCallbacks{};

    // static mexc::Side generalOrderSideToBinanceOrderSide(const OrderSide &side) {
    //     switch (side) {
    //         case OrderSide::Buy:
    //             return mexc::Side::BUY;
    //         case OrderSide::Sell:
    //             return binance::Side::SELL;
    //         default:
    //             return binance::Side::BUY;
    //     }
    // }

    // static binance::futures::OrderType generalOrderTypeToBinanceOrderType(const OrderType &type) {
    //     switch (type) {
    //         case OrderType::Market:
    //             return binance::futures::OrderType::MARKET;
    //         case OrderType::Limit:
    //             return binance::futures::OrderType::LIMIT;
    //         case OrderType::Stop:
    //             return binance::futures::OrderType::STOP_MARKET;
    //         case OrderType::StopLimit:
    //             return binance::futures::OrderType::STOP;
    //         default:
    //             return binance::futures::OrderType::MARKET;
    //     }
    // }

    // static binance::TimeInForce generalTImeInForceToBinanceTimeInForce(const TimeInForce &timeInForce) {
    //     switch (timeInForce) {
    //         case TimeInForce::GTC:
    //             return binance::TimeInForce::GTC;
    //         case TimeInForce::IOC:
    //             return binance::TimeInForce::IOC;
    //         case TimeInForce::FOK:
    //             return binance::TimeInForce::FOK;
    //         default:
    //             return binance::TimeInForce::GTC;
    //     }
    // }

    //static OrderStatus binanceOrderStatusToGeneralOrderStatus(const binance::futures::OrderStatus &status) {
    // switch (status) {
    //     case binance::futures::OrderStatus::NEW:
    //         return OrderStatus::New;
    //     case binance::futures::OrderStatus::PARTIALLY_FILLED:
    //     case binance::futures::OrderStatus::FILLED:
    //         return OrderStatus::Filled;
    //     case binance::futures::OrderStatus::CANCELED:
    //     case binance::futures::OrderStatus::PENDING_CANCEL:
    //         return OrderStatus::Cancelled;
    //     case binance::futures::OrderStatus::REJECTED:
    //         return OrderStatus::Rejected;
    //     case binance::futures::OrderStatus::EXPIRED:
    //         return OrderStatus::Expired;
    //     default:
    //        return OrderStatus::Filled;
    // }
    //}
};

MEXCFuturesExchangeConnector::MEXCFuturesExchangeConnector() : m_p(std::make_unique<P>()) {
    m_p->m_wsClient = std::make_unique<mexc::futures::WSClient>();
    m_p->m_wsClient->setDataEventCallback([&](const mexc::futures::Event& event) {
        if (event.m_channel == "push.ticker") {
            if (const auto it = m_p->m_tickerStreamCallbacks.find(event.m_symbol); it != m_p->m_tickerStreamCallbacks.
                end()) {
                mexc::futures::EventTicker eventTicker;
                eventTicker.fromJson(event.m_data);

                Ticker ticker;
                ticker.time = eventTicker.m_timestamp;
                ticker.askPrice = eventTicker.m_ask1;
                ticker.bidPrice = eventTicker.m_bid1;
                ticker.customData = event.m_data;
                it->second(ticker);
            }
        }
    });
}

MEXCFuturesExchangeConnector::~MEXCFuturesExchangeConnector() {
    m_p->m_wsClient.reset();
    m_p->m_restClient.reset();
}

std::string MEXCFuturesExchangeConnector::name() const {
    return std::string(magic_enum::enum_name(ExchangeId::MEXCFutures));
}

std::string MEXCFuturesExchangeConnector::version() const {
    return "1.0.4";
}

void MEXCFuturesExchangeConnector::setLoggerCallback(const onLogMessage& onLogMessageCB) {
    m_p->m_wsClient->setLoggerCallback(onLogMessageCB);
}

void MEXCFuturesExchangeConnector::login(const std::tuple<std::string, std::string, std::string>& credentials) {
    m_p->m_restClient = std::make_unique<mexc::futures::RESTClient>(std::get<0>(credentials),
                                                                    std::get<1>(credentials));
}

void MEXCFuturesExchangeConnector::setDemo(const bool demo) {
    m_p->m_demo = demo;
}

bool MEXCFuturesExchangeConnector::isDemo() const {
    return m_p->m_demo;
}

Trade MEXCFuturesExchangeConnector::placeOrder(const Order& order) {
    Trade retVal;
    // TODO: Implement me!
    return retVal;
}

Ticker MEXCFuturesExchangeConnector::getSymbolTicker(const std::string& symbol) const {
    Ticker retVal;
    // const binance::futures::BookTickerPrice bookTickerPrice = m_p->restClient->getBookTickerPrice("BTCUSDT");
    // retVal.askPrice = bookTickerPrice.m_askPrice;
    // retVal.bidPrice = bookTickerPrice.m_bidPrice;
    return retVal;
}

Balance MEXCFuturesExchangeConnector::getAccountBalance(const std::string& currency) const {
    Balance retVal;

    // for (const auto accountBalances = m_p->restClient->getAccountBalances(); const auto &el: accountBalances) {
    //     if (el.m_asset == currency) {
    //         retVal.balance = el.m_balance;
    //     }
    // }
    return retVal;
}

FundingRate MEXCFuturesExchangeConnector::getLastFundingRate(const std::string& symbol) const {
    // const auto fr = m_p->restClient->getLastFundingRate(symbol);
    // return {fr.m_symbol, fr.m_fundingRate, fr.m_fundingTime};
    return {};
}

std::vector<FundingRate> MEXCFuturesExchangeConnector::getFundingRates(
    const std::string& symbol, const std::int64_t startTime, const std::int64_t endTime) const {
    std::vector<FundingRate> retVal;
    //
    // for (const auto fRates = m_p->restClient->getFundingRates(symbol, startTime, endTime); const auto& fr: fRates) {
    //     retVal.push_back({fr.m_symbol, fr.m_fundingRate, fr.m_fundingTime, {}});
    // }
    return retVal;
}

void MEXCFuturesExchangeConnector::subscribeTickerStream(const std::string& symbol,
                                                         const onTickerPriceEvent& tickerPriceEventCB) const {
    m_p->m_tickerStreamCallbacks.insert_or_assign(symbol, tickerPriceEventCB);
    mexc::futures::WSSubscription subscriptionRequest;
    subscriptionRequest.m_method = "sub.ticker";
    subscriptionRequest.m_parameters.m_symbol = "symbol";
    const auto subscriptionStr = subscriptionRequest.toJson().dump();
    m_p->m_wsClient->subscribe(subscriptionRequest.toJson());
}

void MEXCFuturesExchangeConnector::unSubscribeTickerStream(const std::string& symbol) const {
    // TODO: Implement me!
}
}
