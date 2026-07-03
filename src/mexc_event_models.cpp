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
namespace {
/// MEXC private channels send numeric fields as EITHER numbers or strings
/// (e.g. big orderIds and prices arrive as strings). These readers tolerate
/// both; readValue<T>/readStringAsX each handle only one.
double readNum(const nlohmann::json &json, const std::string &key, const double def = 0.0) {
    const auto it = json.find(key);

    if (it == json.end() || it->is_null()) {
        return def;
    }

    try {
        if (it->is_string()) {
            const auto &s = it->get_ref<const std::string &>();
            return s.empty() ? def : std::stod(s);
        }

        if (it->is_number()) {
            return it->get<double>();
        }
    } catch (std::exception &) {
    }

    return def;
}

std::int64_t readI64(const nlohmann::json &json, const std::string &key, const std::int64_t def = 0) {
    const auto it = json.find(key);

    if (it == json.end() || it->is_null()) {
        return def;
    }

    try {
        if (it->is_string()) {
            const auto &s = it->get_ref<const std::string &>();
            return s.empty() ? def : std::stoll(s);
        }

        if (it->is_number()) {
            return it->get<std::int64_t>();
        }
    } catch (std::exception &) {
    }

    return def;
}

std::int32_t readI32(const nlohmann::json &json, const std::string &key, const std::int32_t def = 0) {
    return static_cast<std::int32_t>(readI64(json, key, def));
}

bool readBoolT(const nlohmann::json &json, const std::string &key, const bool def = false) {
    const auto it = json.find(key);

    if (it == json.end() || it->is_null()) {
        return def;
    }

    if (it->is_boolean()) {
        return it->get<bool>();
    }

    if (it->is_string()) {
        const auto &s = it->get_ref<const std::string &>();
        return s == "true" || s == "1";
    }

    if (it->is_number()) {
        return it->get<double>() != 0.0;
    }

    return def;
}
} // namespace

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

	/// Guarded — const operator[] on a missing key is UB, and an unexpected
	/// venue message without "data" must not take the io thread down.
	if (const auto it = json.find("data"); it != json.end()) {
		data = *it;
	}
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
	readValue<std::int64_t>(json, "timestamp", timestamp);
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
	orderId = readI64(json, "orderId", orderId);
	readValue<std::string>(json, "symbol", symbol);
	readValue<std::string>(json, "externalOid", externalOid);

	/// MEXC sends the enums as integers (1..N), not names — read the int and cast.
	side = static_cast<OrderSide>(readI32(json, "side", 1));
	orderType = static_cast<OrderType>(readI32(json, "orderType", 1));
	state = static_cast<OrderState>(readI32(json, "state", 1));

	price = readNum(json, "price", price);
	vol = readNum(json, "vol", vol);
	dealVol = readNum(json, "dealVol", dealVol);
	dealAvgPrice = readNum(json, "dealAvgPrice", dealAvgPrice);
	errorCode = readI32(json, "errorCode", errorCode);
	updateTime = readI64(json, "updateTime", updateTime);
}

nlohmann::json EventDeal::toJson() const {
	throw std::runtime_error("Unimplemented: EventDeal::toJson()");
}

void EventDeal::fromJson(const nlohmann::json &json) {
	id = readI64(json, "id", id);
	orderId = readI64(json, "orderId", orderId);
	readValue<std::string>(json, "symbol", symbol);

	side = static_cast<OrderSide>(readI32(json, "side", 1));

	price = readNum(json, "price", price);
	vol = readNum(json, "vol", vol);
	fee = readNum(json, "fee", fee);
	isTaker = readBoolT(json, "isTaker", isTaker);
	timestamp = readI64(json, "timestamp", timestamp);
}
}
