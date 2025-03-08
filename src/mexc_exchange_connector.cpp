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
};

MEXCFuturesExchangeConnector::MEXCFuturesExchangeConnector() : m_p(std::make_unique<P>()) {
    m_p->m_wsClient = std::make_unique<mexc::futures::WSClient>();
    m_p->m_restClient = std::make_unique<mexc::futures::RESTClient>("","");
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
    m_p->m_restClient->setCredentials(std::get<0>(credentials),std::get<1>(credentials));
}

Trade MEXCFuturesExchangeConnector::placeOrder(const Order& order) {
    // TODO: Implement me!
    return {};
}

TickerPrice MEXCFuturesExchangeConnector::getTickerPrice(const std::string& symbol) const {
    // TODO: Implement me!
    return {};
}

Balance MEXCFuturesExchangeConnector::getAccountBalance(const std::string& currency) const {
    // TODO: Implement me!
    return {};
}

FundingRate MEXCFuturesExchangeConnector::getLastFundingRate(const std::string& symbol) const {
    // TODO: Implement me!
    return {};
}

std::vector<FundingRate> MEXCFuturesExchangeConnector::getFundingRates(const std::string& symbol, const std::int64_t startTime, const std::int64_t endTime) const {
    // TODO: Implement me!
    return {};
}

std::vector<Ticker> MEXCFuturesExchangeConnector::getTickerInfo(const std::string& symbol) const {
    // TODO: Implement me!
    return {};
}
}
