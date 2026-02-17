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
#include "mexc_enums.h"

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

struct Ticker final : Response {
    std::string symbol{};
    boost::multiprecision::cpp_dec_float_50 lastPrice{};
    boost::multiprecision::cpp_dec_float_50 bid1{};
    boost::multiprecision::cpp_dec_float_50 ask1{};
    boost::multiprecision::cpp_dec_float_50 volume24{};
    boost::multiprecision::cpp_dec_float_50 amount24{};
    boost::multiprecision::cpp_dec_float_50 holdVol{};
    std::int64_t timestamp{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json &json) override;
};

struct OpenPosition final : IJson {
    std::int64_t positionId{};
    std::string symbol{};
    std::int32_t positionType{};  ///< 1=long, 2=short
    std::int32_t openType{};      ///< 1=isolated, 2=cross
    std::int32_t state{};
    boost::multiprecision::cpp_dec_float_50 holdVol{};
    boost::multiprecision::cpp_dec_float_50 frozenVol{};
    boost::multiprecision::cpp_dec_float_50 holdAvgPrice{};
    boost::multiprecision::cpp_dec_float_50 openAvgPrice{};
    boost::multiprecision::cpp_dec_float_50 liquidatePrice{};
    boost::multiprecision::cpp_dec_float_50 oim{};
    boost::multiprecision::cpp_dec_float_50 im{};
    boost::multiprecision::cpp_dec_float_50 holdFee{};
    boost::multiprecision::cpp_dec_float_50 realised{};
    std::int32_t leverage{};
    std::int64_t createTime{};
    std::int64_t updateTime{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json &json) override;
};

struct OpenPositions final : Response {
    std::vector<OpenPosition> positions{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json &json) override;
};

struct OrderRequest final : IJson {
    std::string symbol{};
    double price{};
    double vol{};
    std::int32_t leverage{};
    OrderSide side{};
    OrderType type{};
    MarginType openType{};
    std::int64_t positionId{};         ///< Optional, recommended for close orders
    std::string externalOid{};         ///< Optional external order ID
    double stopLossPrice{};
    double takeProfitPrice{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json &json) override;
};

struct OrderResponse final : Response {
    std::int64_t orderId{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json &json) override;
};

struct CancelOrderResponse final : Response {
    struct Result {
        std::int64_t orderId{};
        std::int32_t errorCode{};
        std::string errorMsg{};
    };

    std::vector<Result> results{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json &json) override;
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

struct ContractDetail final : IJson {
    std::string symbol{};
    std::string displayNameEn{};
    std::string baseCoin{};
    std::string quoteCoin{};
    std::string settleCoin{};
    ContractState state{ContractState::Enabled};
    bool apiAllowed{true};
    std::int32_t automaticDelivery{};
    std::vector<std::string> conceptPlate{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json &json) override;
};

struct ContractDetails final : Response {
    std::vector<ContractDetail> contractDetails{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json &json) override;
};
}

#endif // INCLUDE_VK_MEXC_MODELS_H
