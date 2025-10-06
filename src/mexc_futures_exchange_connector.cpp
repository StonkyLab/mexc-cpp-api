/**
MEXC Exchange Connector

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#include <vk/mexc/mexc_futures_exchange_connector.h>
#include "vk/mexc/mexc_futures_rest_client.h"

namespace vk {
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
    return "1.0.4";
}

void MEXCFuturesExchangeConnector::setLoggerCallback(const onLogMessage& onLogMessageCB) {
}

void MEXCFuturesExchangeConnector::login(const std::tuple<std::string, std::string, std::string>& credentials) {
    m_p->m_restClient.reset();
    m_p->m_restClient = std::make_unique<mexc::futures::RESTClient>(std::get<0>(credentials),
                                                                  std::get<1>(credentials));
}

Trade MEXCFuturesExchangeConnector::placeOrder(const Order& order) {
    Trade retVal;
    throw std::runtime_error("Unimplemented: MEXCFuturesExchangeConnector::placeOrder");
}

TickerPrice MEXCFuturesExchangeConnector::getTickerPrice(const std::string& symbol) const {
    TickerPrice retVal;
    throw std::runtime_error("Unimplemented: MEXCFuturesExchangeConnector::getTickerPrice");
}

Balance MEXCFuturesExchangeConnector::getAccountBalance(const std::string& currency) const {
    Balance retVal;
    throw std::runtime_error("Unimplemented: MEXCFuturesExchangeConnector::getAccountBalance");
}

FundingRate MEXCFuturesExchangeConnector::getFundingRate(const std::string& symbol) const {
    const auto fr = m_p->m_restClient->getContractFundingRate(symbol);
    //return {fr.m_symbol,fr.m_fundingRate.convert_to<double>(), fr.m_nextSettleTime};
    return {};
}

std::vector<FundingRate> MEXCFuturesExchangeConnector::getFundingRates() const {
    std::vector<FundingRate> retVal;

    // for (const auto& rate : m_p->m_restClient->getContractFundingRates()) {
    //     FundingRate fr;
    //     fr.symbol = rate.m_symbol;
    //     fr.fundingRate = rate.m_fundingRate.convert_to<double>();
    //     fr.fundingTime = rate.m_nextSettleTime;
    //     retVal.push_back(fr);
    // }

    return retVal;
}

std::vector<Ticker> MEXCFuturesExchangeConnector::getTickerInfo(const std::string& symbol) const {
    throw std::runtime_error("Unimplemented: MEXCFuturesExchangeConnector::getTickerInfo");
}

std::int64_t MEXCFuturesExchangeConnector::getServerTime() const {
    return m_p->m_restClient->getServerTime();
}
}
