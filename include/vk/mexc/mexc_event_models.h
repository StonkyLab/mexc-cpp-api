/**
MEXC Event Data Models

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#ifndef INCLUDE_VK_MEXC_EVENT_MODELS_V5_H
#define INCLUDE_VK_MEXC_EVENT_MODELS_V5_H

#include "vk/interface/i_json.h"
#include <nlohmann/json.hpp>
#include <boost/multiprecision/cpp_dec_float.hpp>

#include "mexc_enums.h"

namespace vk::mexc::futures {
struct WSSubscriptionParameters final : IJson {
	std::string m_symbol{};
	std::string m_interval{};
	std::int32_t m_limit{-1};
	bool m_compress{false};

	[[nodiscard]] nlohmann::json toJson() const override;

	void fromJson(const nlohmann::json &json) override;
};

struct WSSubscription final : IJson {
	std::string m_method{};
	WSSubscriptionParameters m_parameters{};

	[[nodiscard]] nlohmann::json toJson() const override;

	void fromJson(const nlohmann::json &json) override;
};

struct Event final : IJson {
	std::string m_channel{};
	std::string m_symbol{};
	std::int64_t m_ts{};
	nlohmann::json m_data{};

	~Event() override = default;

	[[nodiscard]] nlohmann::json toJson() const override;

	void fromJson(const nlohmann::json &json) override;
};

struct EventTicker final : IJson {
	std::string m_symbol{};
	double m_bid1{};
	double m_ask1{};
	double m_volume24{};
	double m_holdVol{};
	double m_lower24Price{};
	double m_high24Price{};
	double m_riseFallRate{};
	double m_riseFallValue{};
	double m_indexPrice{};
	double m_fairPrice{};
	double m_fundingRate{};
	std::int64_t m_timestamp{};

	[[nodiscard]] nlohmann::json toJson() const override;

	void fromJson(const nlohmann::json &json) override;
};

struct EventCandlestick final : IJson {
	std::string m_symbol{};
	double m_amount{};
	CandleInterval m_interval{};
	double m_open{};
	double m_high{};
	double m_low{};
	double m_close{};
	double m_volume{};
	std::int64_t m_start{};

	[[nodiscard]] nlohmann::json toJson() const override;

	void fromJson(const nlohmann::json &json) override;
};
}
#endif //INCLUDE_VK_MEXC_EVENT_MODELS_V5_H
