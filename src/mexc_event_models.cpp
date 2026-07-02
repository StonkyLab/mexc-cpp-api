/**
MEXC Event Data Models

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@stonky.cz>, Stonky s.r.o.
*/

#include "stonky/mexc/mexc_event_models.h"
#include "stonky/utils/utils.h"
#include "stonky/utils/json_utils.h"

namespace stonky::mexc::futures {
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

nlohmann::json EventOrder::toJson() const {
	throw std::runtime_error("Unimplemented: EventOrder::toJson()");
}

void EventOrder::fromJson(const nlohmann::json &json) {
	readValue<std::int64_t>(json, "orderId", orderId);
	readValue<std::string>(json, "symbol", symbol);
	readValue<std::string>(json, "externalOid", externalOid);

	/// MEXC sends the enums as integers (1..N), not names — read the int and cast.
	std::int32_t sideRaw{1}, typeRaw{1}, stateRaw{1};
	readValue<std::int32_t>(json, "side", sideRaw);
	readValue<std::int32_t>(json, "orderType", typeRaw);
	readValue<std::int32_t>(json, "state", stateRaw);
	side = static_cast<OrderSide>(sideRaw);
	orderType = static_cast<OrderType>(typeRaw);
	state = static_cast<OrderState>(stateRaw);

	readValue<double>(json, "price", price);
	readValue<double>(json, "vol", vol);
	readValue<double>(json, "dealVol", dealVol);
	readValue<double>(json, "dealAvgPrice", dealAvgPrice);
	readValue<std::int32_t>(json, "errorCode", errorCode);
	readValue<std::int64_t>(json, "updateTime", updateTime);
}

nlohmann::json EventDeal::toJson() const {
	throw std::runtime_error("Unimplemented: EventDeal::toJson()");
}

void EventDeal::fromJson(const nlohmann::json &json) {
	readValue<std::int64_t>(json, "id", id);
	readValue<std::int64_t>(json, "orderId", orderId);
	readValue<std::string>(json, "symbol", symbol);

	std::int32_t sideRaw{1};
	readValue<std::int32_t>(json, "side", sideRaw);
	side = static_cast<OrderSide>(sideRaw);

	readValue<double>(json, "price", price);
	readValue<double>(json, "vol", vol);
	readValue<double>(json, "fee", fee);
	readValue<bool>(json, "isTaker", isTaker);
	readValue<std::int64_t>(json, "timestamp", timestamp);
}
}
