/**
MEXC Enums

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#ifndef INCLUDE_VK_MEXC_ENUMS_H
#define INCLUDE_VK_MEXC_ENUMS_H

#ifdef _WIN32
#include "magic_enum/magic_enum.hpp"
#else
#include "magic_enum.hpp"
#endif

namespace vk::mexc {
enum class CandleInterval : std::int32_t {
	_1m,
	_5m,
	_15m,
	_30m,
	_60m,
	_4h,
	_8h,
	_1d,
	_1W,
	_1M
};
}

template<>
constexpr magic_enum::customize::customize_t magic_enum::customize::enum_name<vk::mexc::CandleInterval>(
	const vk::mexc::CandleInterval value) noexcept {
	switch (value) {
		case vk::mexc::CandleInterval::_1m:
			return "Min1";
		case vk::mexc::CandleInterval::_5m:
			return "Min5";
		case vk::mexc::CandleInterval::_15m:
			return "Min15";
		case vk::mexc::CandleInterval::_30m:
			return "Min30";
		case vk::mexc::CandleInterval::_60m:
			return "Min60";
		case vk::mexc::CandleInterval::_4h:
			return "Hour4";
		case vk::mexc::CandleInterval::_8h:
			return "Hour8";
		case vk::mexc::CandleInterval::_1d:
			return "Day1";
		case vk::mexc::CandleInterval::_1W:
			return "Week1";
		case vk::mexc::CandleInterval::_1M:
			return "Month1";
		default: ;
	}

	return default_tag;
}

#endif // INCLUDE_VK_MEXC_ENUMS_H
