/**
Binance Exchange Connector

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#include <vk/mexc/mexc_exchange_connector.h>
#include "vk/mexc/mexc_futures_rest_client.h"

namespace vk {
struct MEXCFuturesExchangeConnector::P {
    std::shared_ptr<mexc::futures::RESTClient> restClient{};
    bool m_demo = true;

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
}

MEXCFuturesExchangeConnector::~MEXCFuturesExchangeConnector() {
    //m_p->streamManager.reset();
    m_p->restClient.reset();
}

std::string MEXCFuturesExchangeConnector::name() const {
    return std::string(magic_enum::enum_name(ExchangeId::MEXCFutures));
}

std::string MEXCFuturesExchangeConnector::version() const {
    return "1.0.4";
}

void MEXCFuturesExchangeConnector::setLoggerCallback(const onLogMessage &onLogMessageCB) {
    //m_p->streamManager->setLoggerCallback(onLogMessageCB);
}

void MEXCFuturesExchangeConnector::login(const std::tuple<std::string, std::string, std::string> &credentials) {
    //m_p->streamManager.reset();
    m_p->restClient.reset();
    m_p->restClient = std::make_shared<mexc::futures::RESTClient>(std::get<0>(credentials),
                                                                  std::get<1>(credentials));
    //m_p->streamManager = std::make_unique<binance::futures::WSStreamManager>(m_p->restClient);
}

void MEXCFuturesExchangeConnector::setDemo(const bool demo) {
    m_p->m_demo = demo;
}

bool MEXCFuturesExchangeConnector::isDemo() const {
    return m_p->m_demo;
}

Trade MEXCFuturesExchangeConnector::placeOrder(const Order &order) {
    Trade retVal;

    return retVal;
}

TickerPrice MEXCFuturesExchangeConnector::getSymbolTickerPrice(const std::string &symbol) const {
    TickerPrice retVal;
    // const binance::futures::BookTickerPrice bookTickerPrice = m_p->restClient->getBookTickerPrice("BTCUSDT");
    // retVal.askPrice = bookTickerPrice.m_askPrice;
    // retVal.bidPrice = bookTickerPrice.m_bidPrice;
    return retVal;
}

Balance MEXCFuturesExchangeConnector::getAccountBalance(const std::string &currency) const {
    Balance retVal;

    // for (const auto accountBalances = m_p->restClient->getAccountBalances(); const auto &el: accountBalances) {
    //     if (el.m_asset == currency) {
    //         retVal.balance = el.m_balance;
    //     }
    // }
    return retVal;
}

FundingRate MEXCFuturesExchangeConnector::getLastFundingRate(const std::string &symbol) const {
    // const auto fr = m_p->restClient->getLastFundingRate(symbol);
    // return {fr.m_symbol, fr.m_fundingRate, fr.m_fundingTime};
    return {};
}

std::vector<FundingRate> MEXCFuturesExchangeConnector::getFundingRates(
    const std::string &symbol, const std::int64_t startTime, const std::int64_t endTime) const {
    std::vector<FundingRate> retVal;
    //
    // for (const auto fRates = m_p->restClient->getFundingRates(symbol, startTime, endTime); const auto& fr: fRates) {
    //     retVal.push_back({fr.m_symbol, fr.m_fundingRate, fr.m_fundingTime, {}});
    // }
    return retVal;
}
}
