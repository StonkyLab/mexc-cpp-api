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
nlohmann::json Response::toJson() const {
    throw std::runtime_error("Unimplemented: Response::toJson()");
}

void Response::fromJson(const nlohmann::json &json) {
    std::string codeStr;
    readValue<std::string>(json, "code", codeStr);
    readValue<std::string>(json, "msg", msg);

    if (!codeStr.empty()) {
        code = std::stoi(codeStr);
    }
}

nlohmann::json ServerTime::toJson() const {
    throw std::runtime_error("Unimplemented: ServerTime::toJson()");
}

void ServerTime::fromJson(const nlohmann::json &json) {
    Response::fromJson(json);
    readValue<std::int64_t>(json, "serverTime", serverTime);
}

nlohmann::json Candle::toJson() const {
    throw std::runtime_error("Unimplemented: Candle::toJson()");
}

void Candle::fromJson(const nlohmann::json &json) {
    Response::fromJson(json);
    openTime = json[0];
    open.assign(json[1].get<std::string>());
    high.assign(json[2].get<std::string>());
    low.assign(json[3].get<std::string>());
    close.assign(json[4].get<std::string>());
    volume.assign(json[5].get<std::string>());
    closeTime = json[6];
    quoteAssetVolume.assign(json[7].get<std::string>());
}

nlohmann::json TickerPrice::toJson() const {
    throw std::runtime_error("Unimplemented: TickerPrice::toJson()");
}

void TickerPrice::fromJson(const nlohmann::json &json) {
    Response::fromJson(json);
    readValue<std::string>(json, "symbol", symbol);
    price.assign(json["price"].get<std::string>());
}

nlohmann::json ListenKey::toJson() const {
    throw std::runtime_error("Unimplemented: ListenKey::toJson()");
}

void ListenKey::fromJson(const nlohmann::json &json) {
    Response::fromJson(json);
    readValue<std::string>(json, "listenKey", listenKey);
}

nlohmann::json ListenKeys::toJson() const {
    throw std::runtime_error("Unimplemented: ListenKey::toJson()");
}

void ListenKeys::fromJson(const nlohmann::json &json) {
    Response::fromJson(json);
    readValue<std::int32_t>(json, "available", available);
    readValue<std::int32_t>(json, "total", total);
    listenKeys = json["listenKey"];
}
}

namespace vk::mexc::futures {
nlohmann::json Response::toJson() const {
    throw std::runtime_error("Unimplemented: Response::toJson()");
}

void Response::fromJson(const nlohmann::json &json) {
    readValue<int>(json, "code", code);
    readValue<bool>(json, "success", success);

    if (json.contains("data")) {
        data = json["data"];
    }
}

nlohmann::json ServerTime::toJson() const {
    throw std::runtime_error("Unimplemented: ServerTime::toJson()");
}

void ServerTime::fromJson(const nlohmann::json &json) {
    Response::fromJson(json);
    serverTime = data.get<std::int64_t>();
}

nlohmann::json FundingRate::toJson() const {
    throw std::runtime_error("Unimplemented: FundingRate::toJson()");
}

void FundingRate::fromJson(const nlohmann::json &json) {
    Response::fromJson(json);
    readValue<std::string>(data, "symbol", symbol);

    // MEXC API returns funding rates as numbers, not strings
    if (data.contains("fundingRate") && data["fundingRate"].is_number()) {
        fundingRate.assign(std::to_string(data["fundingRate"].get<double>()));
    } else {
        fundingRate = readDecimalValue(data, "fundingRate");
    }

    if (data.contains("maxFundingRate") && data["maxFundingRate"].is_number()) {
        maxFundingRate.assign(std::to_string(data["maxFundingRate"].get<double>()));
    } else {
        maxFundingRate = readDecimalValue(data, "maxFundingRate");
    }

    if (data.contains("minFundingRate") && data["minFundingRate"].is_number()) {
        minFundingRate.assign(std::to_string(data["minFundingRate"].get<double>()));
    } else {
        minFundingRate = readDecimalValue(data, "minFundingRate");
    }

    readValue<int>(data, "collectCycle", collectCycle);
    readValue<std::int64_t>(data, "nextSettleTime", nextSettleTime);
    readValue<std::int64_t>(data, "timestamp", timestamp);
}

nlohmann::json FundingRates::toJson() const {
    throw std::runtime_error("Unimplemented: FundingRates::toJson()");
}

void FundingRates::fromJson(const nlohmann::json &json) {
    Response::fromJson(json);

    for (const auto &el: data.items()) {
        nlohmann::json data;
        data["data"] = el.value();
        FundingRate fundingRate;
        fundingRate.fromJson(data);
        fundingRates.push_back(fundingRate);
    }
}

nlohmann::json HistoricalFundingRate::toJson() const {
    throw std::runtime_error("Unimplemented: HistoricalFundingRate::toJson()");
}

void HistoricalFundingRate::fromJson(const nlohmann::json &json) {
    readValue<std::string>(json, "symbol", symbol);

    // MEXC API returns funding rates as numbers
    if (json.contains("fundingRate") && json["fundingRate"].is_number()) {
        fundingRate.assign(std::to_string(json["fundingRate"].get<double>()));
    } else {
        fundingRate = readDecimalValue(json, "fundingRate");
    }

    readValue<std::int64_t>(json, "settleTime", settleTime);
}

nlohmann::json HistoricalFundingRates::toJson() const {
    throw std::runtime_error("Unimplemented: HistoricalFundingRates::toJson()");
}

void HistoricalFundingRates::fromJson(const nlohmann::json &json) {
    Response::fromJson(json);

    readValue<std::int32_t>(data, "pageSize", pageSize);
    readValue<std::int32_t>(data, "totalCount", totalCount);
    readValue<std::int32_t>(data, "totalPage", totalPage);
    readValue<std::int32_t>(data, "currentPage", currentPage);

    if (data.contains("resultList") && data["resultList"].is_array()) {
        for (const auto &item : data["resultList"]) {
            HistoricalFundingRate rate;
            rate.fromJson(item);
            resultList.push_back(rate);
        }
    }
}

[[nodiscard]] nlohmann::json WalletBalance::toJson() const {
    throw std::runtime_error("Unimplemented: WalletBalance::toJson()");
}

void WalletBalance::fromJson(const nlohmann::json &json) {
    Response::fromJson(json);
    readValue<std::string>(data, "currency", currency);
    positionMargin = readDecimalValue(data, "positionMargin");
    availableBalance = readDecimalValue(data, "availableBalance");
    cashBalance = readDecimalValue(data, "cashBalance");
    frozenBalance = readDecimalValue(data, "frozenBalance");
    equity = readDecimalValue(data, "equity");
    unrealized = readDecimalValue(data, "unrealized");
    bonus = readDecimalValue(data, "bonus");
}

nlohmann::json Candle::toJson() const {
    throw std::runtime_error("Unimplemented: Candle::toJson()");
}

void Candle::fromJson(const nlohmann::json &json) {
    // Single candle is not parsed from JSON directly, see Candles::fromJson
    throw std::runtime_error("Unimplemented: Candle::fromJson()");
}

nlohmann::json Candles::toJson() const {
    throw std::runtime_error("Unimplemented: Candles::toJson()");
}

void Candles::fromJson(const nlohmann::json &json) {
    Response::fromJson(json);

    if (!data.contains("time") || data["time"].empty()) {
        return;
    }

    const auto &times = data["time"];
    const auto &opens = data["open"];
    const auto &highs = data["high"];
    const auto &lows = data["low"];
    const auto &closes = data["close"];
    const auto &vols = data["vol"];
    const auto &amounts = data["amount"];

    const size_t count = times.size();
    candles.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        Candle candle;
        candle.openTime = times[i].get<std::int64_t>() * 1000; // Convert seconds to ms
        candle.open.assign(std::to_string(opens[i].get<double>()));
        candle.high.assign(std::to_string(highs[i].get<double>()));
        candle.low.assign(std::to_string(lows[i].get<double>()));
        candle.close.assign(std::to_string(closes[i].get<double>()));
        candle.volume.assign(std::to_string(vols[i].get<double>()));
        candle.amount.assign(std::to_string(amounts[i].get<double>()));
        candles.push_back(candle);
    }
}

nlohmann::json ContractDetail::toJson() const {
    throw std::runtime_error("Unimplemented: ContractDetail::toJson()");
}

void ContractDetail::fromJson(const nlohmann::json &json) {
    readValue<std::string>(json, "symbol", symbol);
    readValue<std::string>(json, "displayNameEn", displayNameEn);
    readValue<std::string>(json, "baseCoin", baseCoin);
    readValue<std::string>(json, "quoteCoin", quoteCoin);
    readValue<std::string>(json, "settleCoin", settleCoin);
    readValue<bool>(json, "apiAllowed", apiAllowed);
    readValue<std::int32_t>(json, "automaticDelivery", automaticDelivery);

    if (json.contains("state") && json["state"].is_number()) {
        state = static_cast<ContractState>(json["state"].get<std::int32_t>());
    }

    if (json.contains("conceptPlate") && json["conceptPlate"].is_array()) {
        for (const auto &item : json["conceptPlate"]) {
            if (item.is_string()) {
                conceptPlate.push_back(item.get<std::string>());
            }
        }
    }
}

nlohmann::json ContractDetails::toJson() const {
    throw std::runtime_error("Unimplemented: ContractDetails::toJson()");
}

void ContractDetails::fromJson(const nlohmann::json &json) {
    Response::fromJson(json);

    if (data.is_array()) {
        for (const auto &item : data) {
            ContractDetail detail;
            detail.fromJson(item);
            contractDetails.push_back(detail);
        }
    }
}
}
