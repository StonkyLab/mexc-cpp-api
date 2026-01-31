/**
MEXC Event Data Models

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#ifndef INCLUDE_VK_MEXC_EVENT_MODELS_V5_H
#define INCLUDE_VK_MEXC_EVENT_MODELS_V5_H

#include "mexc_enums.h"
#include "vk/interface/i_json.h"
#include <nlohmann/json.hpp>

namespace vk::mexc::futures {
struct WSSubscriptionParameters final : IJson {
	std::string symbol{};
	std::string interval{};
	std::int32_t limit{-1};
	bool compress{false};

	[[nodiscard]] nlohmann::json toJson() const override;

	void fromJson(const nlohmann::json &json) override;
};

struct WSSubscription final : IJson {
	std::string method{};
	WSSubscriptionParameters parameters{};

	[[nodiscard]] nlohmann::json toJson() const override;

	void fromJson(const nlohmann::json &json) override;
};

struct Event final : IJson {
	std::string channel{};
	std::string symbol{};
	std::int64_t ts{};
	nlohmann::json data{};

	~Event() override = default;

	[[nodiscard]] nlohmann::json toJson() const override;

	void fromJson(const nlohmann::json &json) override;
};

struct EventTicker final : IJson {
	std::string symbol{};
	double bid1{};
	double ask1{};
	double volume24{};
	double holdVol{};
	double lower24Price{};
	double high24Price{};
	double riseFallRate{};
	double riseFallValue{};
	double indexPrice{};
	double fairPrice{};
	double fundingRate{};
	std::int64_t timestamp{};

	[[nodiscard]] nlohmann::json toJson() const override;

	void fromJson(const nlohmann::json &json) override;
};

struct EventCandlestick final : IJson {
	std::string symbol{};
	double amount{};
	CandleInterval interval{};
	double open{};
	double high{};
	double low{};
	double close{};
	double volume{};
	std::int64_t start{};

	[[nodiscard]] nlohmann::json toJson() const override;

	void fromJson(const nlohmann::json &json) override;
};
}
#endif //INCLUDE_VK_MEXC_EVENT_MODELS_V5_H
