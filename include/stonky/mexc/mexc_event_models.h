/**
MEXC Event Data Models

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@stonky.cz>, Stonky s.r.o.
*/

#ifndef INCLUDE_STONKY_MEXC_EVENT_MODELS_V5_H
#define INCLUDE_STONKY_MEXC_EVENT_MODELS_V5_H

#include "mexc_enums.h"
#include "stonky/interface/i_json.h"
#include <nlohmann/json.hpp>

namespace stonky::mexc::futures {
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
	/// Local steady-clock receipt time (ms), stamped by WSStreamManager on every
	/// update. Not part of the wire — lets consumers measure quote age without
	/// trusting exchange clocks.
	std::int64_t receivedTimestamp{};

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

/**
 * One order state change from the private "push.personal.order" channel.
 * Numeric fields arrive as JSON numbers. `vol`/`dealVol` are in CONTRACTS.
 * @see https://mexcdevelop.github.io/apidocs/contract_v1_en/#private-channels
 */
struct EventOrder final : IJson {
	std::int64_t orderId{};
	std::string symbol{};
	std::string externalOid{};   ///< client-set order id (= our clientOrderId)
	OrderSide side{OrderSide::OpenLong};
	OrderType orderType{OrderType::Limit};
	OrderState state{OrderState::New};
	double price{};
	double vol{};                ///< order size in contracts
	double dealVol{};            ///< filled size in contracts
	double dealAvgPrice{};
	std::int32_t errorCode{};    ///< 0 = EC_NoError
	std::int64_t updateTime{};

	[[nodiscard]] nlohmann::json toJson() const override;

	void fromJson(const nlohmann::json &json) override;
};

/**
 * One fill from the private "push.personal.order.deal" channel. Carries the
 * venue orderId but NOT the externalOid — the gateway maps orderId back to the
 * client order id. `vol` is in CONTRACTS.
 * @see https://mexcdevelop.github.io/apidocs/contract_v1_en/#private-channels
 */
struct EventDeal final : IJson {
	std::int64_t id{};           ///< trade/fill id — dedup key
	std::int64_t orderId{};
	std::string symbol{};
	OrderSide side{OrderSide::OpenLong};
	double price{};
	double vol{};                ///< fill size in contracts
	double fee{};
	bool isTaker{false};
	std::int64_t timestamp{};

	[[nodiscard]] nlohmann::json toJson() const override;

	void fromJson(const nlohmann::json &json) override;
};
}
#endif //INCLUDE_STONKY_MEXC_EVENT_MODELS_V5_H
