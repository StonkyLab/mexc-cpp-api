/**
MEXC Data Models

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#ifndef INCLUDE_VK_MEXC_MODELS_H
#define INCLUDE_VK_MEXC_MODELS_H

#include <nlohmann/json.hpp>
#include "vk/tools/i_json.h"
#include <boost/multiprecision/cpp_dec_float.hpp>

namespace vk::mexc {
struct ServerTime final : IJson {
    std::int64_t m_serverTime{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json& json) override;
};

struct Candle final : IJson {
    std::int64_t m_openTime{};
    std::int64_t m_closeTime{};
    boost::multiprecision::cpp_dec_float_50 m_open{};
    boost::multiprecision::cpp_dec_float_50 m_high{};
    boost::multiprecision::cpp_dec_float_50 m_low{};
    boost::multiprecision::cpp_dec_float_50 m_close{};
    boost::multiprecision::cpp_dec_float_50 m_volume{};
    boost::multiprecision::cpp_dec_float_50 m_quoteAssetVolume{};

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json& json) override;
};

struct Candles final : IJson {
    std::vector<Candle> m_candles;

    [[nodiscard]] nlohmann::json toJson() const override;

    void fromJson(const nlohmann::json& json) override;
};
}

#endif // INCLUDE_VK_MEXC_MODELS_H
