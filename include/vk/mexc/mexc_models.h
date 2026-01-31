/**
MEXC Data Models

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#ifndef INCLUDE_VK_MEXC_MODELS_H
#define INCLUDE_VK_MEXC_MODELS_H

#include <boost/multiprecision/cpp_dec_float.hpp>
#include <nlohmann/json.hpp>
#include "vk/interface/i_json.h"

namespace vk::mexc::spot {

struct Response : IJson {
    int code{};
    std::string msg{};
    nlohmann::json data{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json &json) override;
};

struct ServerTime final : Response {
    std::int64_t serverTime{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json &json) override;
};

struct Candle final : Response {
    std::int64_t openTime{};
    std::int64_t closeTime{};
     boost::multiprecision::cpp_dec_float_50 open{};
     boost::multiprecision::cpp_dec_float_50 high{};
     boost::multiprecision::cpp_dec_float_50 low{};
     boost::multiprecision::cpp_dec_float_50 close{};
     boost::multiprecision::cpp_dec_float_50 volume{};
     boost::multiprecision::cpp_dec_float_50 quoteAssetVolume{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json &json) override;
};

struct TickerPrice final : Response {
    std::string symbol{};
    boost::multiprecision::cpp_dec_float_50 price{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json &json) override;
};

struct ListenKey final : Response {
    std::string listenKey{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json &json) override;
};

struct ListenKeys final : Response {
    std::int32_t available{};
    std::int32_t total{};
    std::vector<std::string> listenKeys{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json &json) override;
};
}

namespace vk::mexc::futures {
struct Response : IJson {
    int code{};
    bool success{};
    nlohmann::json data{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json &json) override;
};

struct ServerTime final : Response {
    std::int64_t serverTime{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json &json) override;
};

struct FundingRate final : Response {
    std::string symbol{};
    boost::multiprecision::cpp_dec_float_50 fundingRate{};
    boost::multiprecision::cpp_dec_float_50 maxFundingRate{};
    boost::multiprecision::cpp_dec_float_50 minFundingRate{};
    std::int32_t collectCycle{};
    std::int64_t nextSettleTime{};
    std::int64_t timestamp{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json &json) override;
};

struct FundingRates final : Response {
    std::vector<FundingRate> fundingRates{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json &json) override;
};

struct HistoricalFundingRate final : IJson {
    std::string symbol{};
    boost::multiprecision::cpp_dec_float_50 fundingRate{};
    std::int64_t settleTime{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json &json) override;
};

struct HistoricalFundingRates final : Response {
    std::int32_t pageSize{};
    std::int32_t totalCount{};
    std::int32_t totalPage{};
    std::int32_t currentPage{};
    std::vector<HistoricalFundingRate> resultList{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json &json) override;
};

struct WalletBalance final : Response {
    std::string currency{};
    boost::multiprecision::cpp_dec_float_50 positionMargin{};
    boost::multiprecision::cpp_dec_float_50 availableBalance{};
    boost::multiprecision::cpp_dec_float_50 cashBalance{};
    boost::multiprecision::cpp_dec_float_50 frozenBalance{};
    boost::multiprecision::cpp_dec_float_50 equity{};
    boost::multiprecision::cpp_dec_float_50 unrealized{};
    boost::multiprecision::cpp_dec_float_50 bonus{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json& json) override;
};

struct Candle final : IJson {
    std::int64_t openTime{};
    boost::multiprecision::cpp_dec_float_50 open{};
    boost::multiprecision::cpp_dec_float_50 high{};
    boost::multiprecision::cpp_dec_float_50 low{};
    boost::multiprecision::cpp_dec_float_50 close{};
    boost::multiprecision::cpp_dec_float_50 volume{};
    boost::multiprecision::cpp_dec_float_50 amount{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json &json) override;
};

struct Candles final : Response {
    std::vector<Candle> candles{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json &json) override;
};
}

#endif // INCLUDE_VK_MEXC_MODELS_H
