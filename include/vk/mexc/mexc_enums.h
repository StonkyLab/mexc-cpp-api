/**
MEXC Enums

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#ifndef INCLUDE_VK_MEXC_ENUMS_H
#define INCLUDE_VK_MEXC_ENUMS_H

#include "vk/utils/magic_enum_wrapper.hpp"

namespace vk::mexc {

/// Contract state: 0=enabled, 1=delivery, 2=completed, 3=offline, 4=paused
enum class ContractState : std::int32_t {
	Enabled = 0,
	Delivery = 1,
	Completed = 2,
	Offline = 3,
	Paused = 4
};

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

namespace vk::mexc::futures {
/// Order side: 1=open long, 2=close short, 3=open short, 4=close long
enum class OrderSide : std::int32_t { OpenLong = 1, CloseShort = 2, OpenShort = 3, CloseLong = 4 };

/// Order type: 1=limit, 2=post-only, 3=IOC, 4=FOK, 5=market, 6=market-to-limit
enum class OrderType : std::int32_t { Limit = 1, PostOnly = 2, IOC = 3, FOK = 4, Market = 5, MarketToLimit = 6 };

/// Margin type: 1=isolated, 2=cross
enum class MarginType : std::int32_t { Isolated = 1, Cross = 2 };
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
