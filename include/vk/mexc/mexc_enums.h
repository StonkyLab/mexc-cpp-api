/**
MEXC Enums

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#ifndef INCLUDE_VK_MEXC_ENUMS_H
#define INCLUDE_VK_MEXC_ENUMS_H

#include "magic_enum.hpp"

namespace vk::mexc {
enum class CandleInterval : std::int32_t {
    _1m,
    _5m,
    _15m,
    _30m,
    _60m,
    _4h,
    _1d,
    _1W,
    _1M
};
}

template <>
constexpr magic_enum::customize::customize_t magic_enum::customize::enum_name<vk::mexc::CandleInterval>(
    const vk::mexc::CandleInterval value) noexcept {
    switch (value) {
    case vk::mexc::CandleInterval::_1m:
        return "1m";
    case vk::mexc::CandleInterval::_5m:
        return "5m";
    case vk::mexc::CandleInterval::_15m:
        return "15m";
    case vk::mexc::CandleInterval::_30m:
        return "30m";
    case vk::mexc::CandleInterval::_60m:
        return "60m";
    case vk::mexc::CandleInterval::_4h:
        return "4h";
    case vk::mexc::CandleInterval::_1d:
        return "1d";
    case vk::mexc::CandleInterval::_1W:
        return "1W";
    case vk::mexc::CandleInterval::_1M:
        return "1M";
    }

    return default_tag;
}

#endif // INCLUDE_VK_MEXC_ENUMS_H
