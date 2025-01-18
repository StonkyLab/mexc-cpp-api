/**
MEXC Event Data Models

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#include "vk/mexc/mexc_event_models.h"
#include "vk/tools/utils.h"
#include "vk/tools/json_utils.h"

namespace vk::mexc::futures {
nlohmann::json WSSubscriptionParameters::toJson() const {
    nlohmann::json result;
    result["symbol"] = m_symbol;

    if (m_compress) {
        result["compress"] = m_compress;
    }

    if (m_limit != -1) {
        result["limit"] = m_limit;
    }

    if (!m_interval.empty()) {
        result["interval"] = m_interval;
    }

    return result;
}

void WSSubscriptionParameters::fromJson(const nlohmann::json& json) {
    throw std::runtime_error("Unimplemented: WSSubscription::fromJson()");
}

nlohmann::json WSSubscription::toJson() const {
    nlohmann::json result;
    result["method"] = m_method;
    result["param"] = m_parameters.toJson();
    return result;
}

void WSSubscription::fromJson(const nlohmann::json& json) {
    throw std::runtime_error("Unimplemented: WSSubscription::fromJson()");
}

nlohmann::json Event::toJson() const {
    throw std::runtime_error("Unimplemented: Event::toJson()");
}

void Event::fromJson(const nlohmann::json& json) {
    readValue<std::string>(json, "channel", m_channel);
    readValue<std::string>(json, "symbol", m_symbol);
    readValue<std::int64_t>(json, "ts", m_ts);
    m_data = json["data"];
}

nlohmann::json EventTicker::toJson() const {
    throw std::runtime_error("Unimplemented: EventTicker::toJson()");
}

void EventTicker::fromJson(const nlohmann::json& json) {
    readValue<std::string>(json, "symbol", m_symbol);
    m_bid1.assign(json["bid1"].get<std::string>());
    m_ask1.assign(json["ask1"].get<std::string>());
    m_volume24.assign(json["volume24"].get<std::string>());
    m_holdVol.assign(json["holdVol"].get<std::string>());
    m_lower24Price.assign(json["lower24Price"].get<std::string>());
    m_high24Price.assign(json["high24Price"].get<std::string>());
    m_riseFallRate.assign(json["riseFallRate"].get<std::string>());
    m_riseFallValue.assign(json["riseFallValue"].get<std::string>());
    m_indexPrice.assign(json["indexPrice"].get<std::string>());
    m_indexPrice.assign(json["indexPrice"].get<std::string>());
    m_fairPrice.assign(json["fairPrice"].get<std::string>());
    m_fundingRate.assign(json["fundingRate"].get<std::string>());
    readValue<std::int64_t>(json, "m_timestamp", m_timestamp);
}

nlohmann::json EventCandlestick::toJson() const {
    throw std::runtime_error("Unimplemented: EventCandlestick::toJson()");
}

void EventCandlestick::fromJson(const nlohmann::json& json) {
    readValue<std::int64_t>(json, "start", m_start);
    readValue<std::int64_t>(json, "end", m_end);
    readValue<std::string>(json, "interval", m_interval);
    m_open = readStringAsDouble(json, "open", m_open);
    m_high = readStringAsDouble(json, "high", m_high);
    m_low = readStringAsDouble(json, "low", m_low);
    m_close = readStringAsDouble(json, "close", m_close);
    m_volume = readStringAsDouble(json, "volume", m_volume);
    m_turnover = readStringAsDouble(json, "turnover", m_turnover);
    readValue<bool>(json, "confirm", m_confirm);
    readValue<std::int64_t>(json, "timestamp", m_timestamp);
}
}
