/**
MEXC Event Data Models

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#include "vk/mexc/mexc_event_models.h"
#include "vk/utils/utils.h"
#include "vk/utils/json_utils.h"

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

void WSSubscriptionParameters::fromJson(const nlohmann::json &json) {
	throw std::runtime_error("Unimplemented: WSSubscription::fromJson()");
}

nlohmann::json WSSubscription::toJson() const {
	nlohmann::json result;
	result["method"] = m_method;
	result["param"] = m_parameters.toJson();
	return result;
}

void WSSubscription::fromJson(const nlohmann::json &json) {
	throw std::runtime_error("Unimplemented: WSSubscription::fromJson()");
}

nlohmann::json Event::toJson() const {
	throw std::runtime_error("Unimplemented: Event::toJson()");
}

void Event::fromJson(const nlohmann::json &json) {
	readValue<std::string>(json, "channel", m_channel);
	readValue<std::string>(json, "symbol", m_symbol);
	readValue<std::int64_t>(json, "ts", m_ts);
	m_data = json["data"];
}

nlohmann::json EventTicker::toJson() const {
	throw std::runtime_error("Unimplemented: EventTicker::toJson()");
}

void EventTicker::fromJson(const nlohmann::json &json) {
	readValue<std::string>(json, "symbol", m_symbol);
	readValue<double>(json, "bid1", m_bid1);
	readValue<double>(json, "ask1", m_ask1);
	readValue<double>(json, "volume24", m_volume24);
	readValue<double>(json, "holdVol", m_holdVol);
	readValue<double>(json, "lower24Price", m_lower24Price);
	readValue<double>(json, "high24Price", m_high24Price);
	readValue<double>(json, "riseFallRate", m_riseFallRate);
	readValue<double>(json, "riseFallValue", m_riseFallValue);
	readValue<double>(json, "indexPrice", m_indexPrice);
	readValue<double>(json, "fairPrice", m_fairPrice);
	readValue<double>(json, "fundingRate", m_fundingRate);
	readValue<std::int64_t>(json, "m_timestamp", m_timestamp);
}

nlohmann::json EventCandlestick::toJson() const {
	throw std::runtime_error("Unimplemented: EventCandlestick::toJson()");
}

void EventCandlestick::fromJson(const nlohmann::json &json) {
	readValue<std::string>(json, "symbol", m_symbol);
	readValue<double>(json, "a", m_amount);
	readMagicEnum<CandleInterval>(json, "interval", m_interval);
	readValue<double>(json, "o", m_open);
	readValue<double>(json, "h", m_high);
	readValue<double>(json, "l", m_low);
	readValue<double>(json, "c", m_close);
	readValue<double>(json, "q", m_volume);
	readValue<std::int64_t>(json, "t", m_start);
}
}
