/**
MEXC Data Models

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#include "vk/mexc/mexc_models.h"
#include "vk/utils/utils.h"
#include "vk/utils/json_utils.h"

namespace vk::mexc::spot {
nlohmann::json ServerTime::toJson() const {
	throw std::runtime_error("Unimplemented: ServerTime::toJson()");
}

void ServerTime::fromJson(const nlohmann::json &json) {
	readValue<std::int64_t>(json, "serverTime", m_serverTime);
}

nlohmann::json Candle::toJson() const {
	throw std::runtime_error("Unimplemented: Candle::toJson()");
}

void Candle::fromJson(const nlohmann::json &json) {
	m_openTime = json[0];
	m_open.assign(json[1].get<std::string>());
	m_high.assign(json[2].get<std::string>());
	m_low.assign(json[3].get<std::string>());
	m_close.assign(json[4].get<std::string>());
	m_volume.assign(json[5].get<std::string>());
	m_closeTime = json[6];
	m_quoteAssetVolume.assign(json[7].get<std::string>());
}

nlohmann::json TickerPrice::toJson() const {
	throw std::runtime_error("Unimplemented: TickerPrice::toJson()");
}

void TickerPrice::fromJson(const nlohmann::json &json) {
	readValue<std::string>(json, "symbol", m_symbol);
	m_price.assign(json["price"].get<std::string>());
}
}

namespace vk::mexc::futures {
nlohmann::json Response::toJson() const {
	throw std::runtime_error("Unimplemented: Response::toJson()");
}

void Response::fromJson(const nlohmann::json &json) {
	readValue<int>(json, "code", m_code);
	readValue<bool>(json, "success", m_success);

	if (json.contains("data")) {
		m_data = json["data"];
	}
}

nlohmann::json ServerTime::toJson() const {
	throw std::runtime_error("Unimplemented: ServerTime::toJson()");
}

void ServerTime::fromJson(const nlohmann::json &json) {
	Response::fromJson(json);
	m_serverTime = m_data.get<std::int64_t>();
}

nlohmann::json FundingRate::toJson() const {
	throw std::runtime_error("Unimplemented: FundingRate::toJson()");
}

void FundingRate::fromJson(const nlohmann::json &json) {
	Response::fromJson(json);
	readValue<std::string>(m_data, "symbol", m_symbol);
	m_fundingRate = readDecimalValue(m_data, "fundingRate");
	m_maxFundingRate = readDecimalValue(m_data, "maxFundingRate");
	m_minFundingRate = readDecimalValue(m_data, "minFundingRate");
	readValue<int>(m_data, "collectCycle", m_collectCycle);
	readValue<std::int64_t>(m_data, "nextSettleTime", m_nextSettleTime);
	readValue<std::int64_t>(m_data, "timestamp", m_timestamp);
}

nlohmann::json FundingRates::toJson() const {
	throw std::runtime_error("Unimplemented: FundingRates::toJson()");
}

void FundingRates::fromJson(const nlohmann::json &json) {
	Response::fromJson(json);

	for (const auto &el: m_data.items()) {
		nlohmann::json data;
		data["data"] = el.value();
		FundingRate fundingRate;
		fundingRate.fromJson(data);
		m_fundingRates.push_back(fundingRate);
	}
}

[[nodiscard]] nlohmann::json WalletBalance::toJson() const {
	throw std::runtime_error("Unimplemented: WalletBalance::toJson()");
}

void WalletBalance::fromJson(const nlohmann::json &json) {
	Response::fromJson(json);
	readValue<std::string>(m_data, "currency", m_currency);
	m_positionMargin = readDecimalValue(m_data, "positionMargin");
	m_availableBalance = readDecimalValue(m_data, "availableBalance");
	m_cashBalance = readDecimalValue(m_data, "cashBalance");
	m_frozenBalance = readDecimalValue(m_data, "frozenBalance");
	m_equity = readDecimalValue(m_data, "equity");
	m_unrealized = readDecimalValue(m_data, "unrealized");
	m_bonus = readDecimalValue(m_data, "bonus");
}
}
