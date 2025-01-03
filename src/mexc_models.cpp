/**
MEXC Data Models

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#include "vk/mexc/mexc_models.h"
#include "vk/tools/utils.h"
#include "vk/tools/json_utils.h"

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
nlohmann::json FundingRate::toJson() const {
    return Response::toJson();
}

void FundingRate::fromJson(const nlohmann::json &json) {
    Response::fromJson(json);
}
}
