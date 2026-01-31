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
	result["symbol"] = symbol;

	if (compress) {
		result["compress"] = compress;
	}

	if (limit != -1) {
		result["limit"] = limit;
	}

	if (!interval.empty()) {
		result["interval"] = interval;
	}

	return result;
}

void WSSubscriptionParameters::fromJson(const nlohmann::json &json) {
	throw std::runtime_error("Unimplemented: WSSubscription::fromJson()");
}

nlohmann::json WSSubscription::toJson() const {
	nlohmann::json result;
	result["method"] = method;
	result["param"] = parameters.toJson();
	return result;
}

void WSSubscription::fromJson(const nlohmann::json &json) {
	throw std::runtime_error("Unimplemented: WSSubscription::fromJson()");
}

nlohmann::json Event::toJson() const {
	throw std::runtime_error("Unimplemented: Event::toJson()");
}

void Event::fromJson(const nlohmann::json &json) {
	readValue<std::string>(json, "channel", channel);
	readValue<std::string>(json, "symbol", symbol);
	readValue<std::int64_t>(json, "ts", ts);
	data = json["data"];
}

nlohmann::json EventTicker::toJson() const {
	throw std::runtime_error("Unimplemented: EventTicker::toJson()");
}

void EventTicker::fromJson(const nlohmann::json &json) {
	readValue<std::string>(json, "symbol", symbol);
	readValue<double>(json, "bid1", bid1);
	readValue<double>(json, "ask1", ask1);
	readValue<double>(json, "volume24", volume24);
	readValue<double>(json, "holdVol", holdVol);
	readValue<double>(json, "lower24Price", lower24Price);
	readValue<double>(json, "high24Price", high24Price);
	readValue<double>(json, "riseFallRate", riseFallRate);
	readValue<double>(json, "riseFallValue", riseFallValue);
	readValue<double>(json, "indexPrice", indexPrice);
	readValue<double>(json, "fairPrice", fairPrice);
	readValue<double>(json, "fundingRate", fundingRate);
	readValue<std::int64_t>(json, "m_timestamp", timestamp);
}

nlohmann::json EventCandlestick::toJson() const {
	throw std::runtime_error("Unimplemented: EventCandlestick::toJson()");
}

void EventCandlestick::fromJson(const nlohmann::json &json) {
	readValue<std::string>(json, "symbol", symbol);
	readValue<double>(json, "a", amount);
	readMagicEnum<CandleInterval>(json, "interval", interval);
	readValue<double>(json, "o", open);
	readValue<double>(json, "h", high);
	readValue<double>(json, "l", low);
	readValue<double>(json, "c", close);
	readValue<double>(json, "q", volume);
	readValue<std::int64_t>(json, "t", start);
}
}
