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

nlohmann::json Ticker::toJson() const {
    throw std::runtime_error("Unimplemented: Ticker::toJson()");
}

void Ticker::fromJson(const nlohmann::json &json) {
    Response::fromJson(json);
    readValue<std::string>(data, "symbol", symbol);

    if (data.contains("lastPrice") && data["lastPrice"].is_number()) {
        lastPrice.assign(std::to_string(data["lastPrice"].get<double>()));
    } else {
        lastPrice = readDecimalValue(data, "lastPrice");
    }

    if (data.contains("bid1") && data["bid1"].is_number()) {
        bid1.assign(std::to_string(data["bid1"].get<double>()));
    } else {
        bid1 = readDecimalValue(data, "bid1");
    }

    if (data.contains("ask1") && data["ask1"].is_number()) {
        ask1.assign(std::to_string(data["ask1"].get<double>()));
    } else {
        ask1 = readDecimalValue(data, "ask1");
    }

    if (data.contains("volume24") && data["volume24"].is_number()) {
        volume24.assign(std::to_string(data["volume24"].get<double>()));
    } else {
        volume24 = readDecimalValue(data, "volume24");
    }

    if (data.contains("amount24") && data["amount24"].is_number()) {
        amount24.assign(std::to_string(data["amount24"].get<double>()));
    } else {
        amount24 = readDecimalValue(data, "amount24");
    }

    if (data.contains("holdVol") && data["holdVol"].is_number()) {
        holdVol.assign(std::to_string(data["holdVol"].get<double>()));
    } else {
        holdVol = readDecimalValue(data, "holdVol");
    }

    readValue<std::int64_t>(data, "timestamp", timestamp);
}

nlohmann::json OpenPosition::toJson() const {
    throw std::runtime_error("Unimplemented: OpenPosition::toJson()");
}

void OpenPosition::fromJson(const nlohmann::json &json) {
    readValue<std::int64_t>(json, "positionId", positionId);
    readValue<std::string>(json, "symbol", symbol);
    readValue<std::int32_t>(json, "positionType", positionType);
    readValue<std::int32_t>(json, "openType", openType);
    readValue<std::int32_t>(json, "state", state);

    if (json.contains("holdVol") && json["holdVol"].is_number()) {
        holdVol.assign(std::to_string(json["holdVol"].get<double>()));
    } else {
        holdVol = readDecimalValue(json, "holdVol");
    }

    if (json.contains("frozenVol") && json["frozenVol"].is_number()) {
        frozenVol.assign(std::to_string(json["frozenVol"].get<double>()));
    } else {
        frozenVol = readDecimalValue(json, "frozenVol");
    }

    if (json.contains("holdAvgPrice") && json["holdAvgPrice"].is_number()) {
        holdAvgPrice.assign(std::to_string(json["holdAvgPrice"].get<double>()));
    } else {
        holdAvgPrice = readDecimalValue(json, "holdAvgPrice");
    }

    if (json.contains("openAvgPrice") && json["openAvgPrice"].is_number()) {
        openAvgPrice.assign(std::to_string(json["openAvgPrice"].get<double>()));
    } else {
        openAvgPrice = readDecimalValue(json, "openAvgPrice");
    }

    if (json.contains("liquidatePrice") && json["liquidatePrice"].is_number()) {
        liquidatePrice.assign(std::to_string(json["liquidatePrice"].get<double>()));
    } else {
        liquidatePrice = readDecimalValue(json, "liquidatePrice");
    }

    if (json.contains("oim") && json["oim"].is_number()) {
        oim.assign(std::to_string(json["oim"].get<double>()));
    } else {
        oim = readDecimalValue(json, "oim");
    }

    if (json.contains("im") && json["im"].is_number()) {
        im.assign(std::to_string(json["im"].get<double>()));
    } else {
        im = readDecimalValue(json, "im");
    }

    if (json.contains("holdFee") && json["holdFee"].is_number()) {
        holdFee.assign(std::to_string(json["holdFee"].get<double>()));
    } else {
        holdFee = readDecimalValue(json, "holdFee");
    }

    if (json.contains("realised") && json["realised"].is_number()) {
        realised.assign(std::to_string(json["realised"].get<double>()));
    } else {
        realised = readDecimalValue(json, "realised");
    }

    readValue<std::int32_t>(json, "leverage", leverage);
    readValue<std::int64_t>(json, "createTime", createTime);
    readValue<std::int64_t>(json, "updateTime", updateTime);
}

nlohmann::json OpenPositions::toJson() const {
    throw std::runtime_error("Unimplemented: OpenPositions::toJson()");
}

void OpenPositions::fromJson(const nlohmann::json &json) {
    Response::fromJson(json);

    if (data.is_array()) {
        for (const auto &item : data) {
            OpenPosition pos;
            pos.fromJson(item);
            positions.push_back(pos);
        }
    }
}

nlohmann::json OrderRequest::toJson() const {
    nlohmann::json j;
    j["symbol"] = symbol;
    j["price"] = price;
    j["vol"] = vol;
    j["side"] = static_cast<std::int32_t>(side);
    j["type"] = static_cast<std::int32_t>(type);
    j["openType"] = static_cast<std::int32_t>(openType);

    if (leverage > 0) {
        j["leverage"] = leverage;
    }
    if (positionId > 0) {
        j["positionId"] = positionId;
    }
    if (!externalOid.empty()) {
        j["externalOid"] = externalOid;
    }
    if (stopLossPrice > 0) {
        j["stopLossPrice"] = stopLossPrice;
    }
    if (takeProfitPrice > 0) {
        j["takeProfitPrice"] = takeProfitPrice;
    }

    return j;
}

void OrderRequest::fromJson(const nlohmann::json &json) {
    throw std::runtime_error("Unimplemented: OrderRequest::fromJson()");
}

nlohmann::json OrderResponse::toJson() const {
    throw std::runtime_error("Unimplemented: OrderResponse::toJson()");
}

void OrderResponse::fromJson(const nlohmann::json &json) {
    Response::fromJson(json);

    if (data.is_number_integer()) {
        orderId = data.get<std::int64_t>();
    }
}

nlohmann::json CancelOrderResponse::toJson() const {
    throw std::runtime_error("Unimplemented: CancelOrderResponse::toJson()");
}

void CancelOrderResponse::fromJson(const nlohmann::json &json) {
    Response::fromJson(json);

    if (data.is_array()) {
        for (const auto &item : data) {
            Result r;
            readValue<std::int64_t>(item, "orderId", r.orderId);
            readValue<std::int32_t>(item, "errorCode", r.errorCode);
            readValue<std::string>(item, "errorMsg", r.errorMsg);
            results.push_back(r);
        }
    }
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
    readValue<double>(json, "contractSize", contractSize);
    readValue<std::int32_t>(json, "minVol", minVol);
    readValue<std::int32_t>(json, "maxVol", maxVol);
    readValue<std::int32_t>(json, "volUnit", volUnit);
    readValue<std::int32_t>(json, "priceUnit", priceUnit);
    readValue<std::int32_t>(json, "pricePrecision", pricePrecision);
    readValue<std::int32_t>(json, "volPrecision", volPrecision);

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
