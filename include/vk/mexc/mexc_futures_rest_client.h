/**
MEXC Futures REST Client

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/


#ifndef INCLUDE_VK_MEXC_FUTURES_REST_CLIENT_H
#define INCLUDE_VK_MEXC_FUTURES_REST_CLIENT_H

#include <string>
#include <memory>
#include <functional>
#include "mexc_models.h"
#include "mexc_enums.h"
#include "mexc_http_futures_session.h"

namespace vk::mexc::futures {

using onCandlesDownloaded = std::function<void(const std::vector<Candle>&)>;

class RESTClient {
	struct P;
	std::unique_ptr<P> m_p{};

public:
	/// Construct with API key + secret (OpenAPI auth)
	RESTClient(const std::string &apiKey, const std::string &apiSecret);

	/// Construct with WEB session token
	explicit RESTClient(const std::string &webToken, AuthSource source);

	~RESTClient();

	void setCredentials(const std::string &apiKey, const std::string &apiSecret) const;

	void setWebToken(const std::string &webToken) const;

	/**
	 * Returns server time in ms
	 * @return timestamp in ms
	 * @see https://www.mexc.com/api-docs/futures/market-endpoints#get-the-server-time
	*/
	[[nodiscard]] std::int64_t getServerTime() const;

	/**
	 * Returns contract funding rate
	 * @return FundingRate
	 * @see https://www.mexc.com/api-docs/futures/market-endpoints#get-contract-funding-rate
	*/
	[[nodiscard]] FundingRate getContractFundingRate(const std::string &contract) const;

	/**
	 * Returns funding rates for all contracts
	 * @see https://www.mexc.com/api-docs/futures/market-endpoints#get-contract-funding-rate
	 * @return vector of FundingRate structures
	 */
	[[nodiscard]] std::vector<FundingRate> getContractFundingRates() const;

	/**
	 * Returns historical funding rates for a contract with pagination
	 * @param symbol contract symbol (e.g., BTC_USDT)
	 * @param pageNum page number (starting from 1)
	 * @param pageSize number of results per page (max 1000)
	 * @see https://www.mexc.com/api-docs/futures/market-endpoints#get-contract-funding-rate-history
	 * @return HistoricalFundingRates structure with pagination info
	 */
	[[nodiscard]] HistoricalFundingRates getContractFundingRateHistory(const std::string &symbol,
	                                                                     std::int32_t pageNum = 1,
	                                                                     std::int32_t pageSize = 1000) const;

	/**
	 * Get the user's single currency asset information
	 * @see https://www.mexc.com/api-docs/futures/account-and-trading-endpoints#get-the-users-single-currency-asset-information
	 * @param currency
	 * @return WalletBalance
	 */
	[[nodiscard]] WalletBalance getWalletBalance(const std::string &currency) const;

	/**
	 * Returns contract ticker data (bid/ask prices, volume, etc.)
	 * @param symbol contract symbol (e.g., BTC_USDT)
	 * @return Ticker
	 * @see https://mexcdevelop.github.io/apidocs/contract_v1_en/#get-contract-ticker
	 */
	[[nodiscard]] Ticker getContractTicker(const std::string &symbol) const;

	/**
	 * Download historical candles
	 * @param symbol e.g. BTC_USDT
	 * @param interval candle interval
	 * @param startTime timestamp in seconds
	 * @param endTime timestamp in seconds
	 * @param writer optional callback called after each batch of candles is downloaded for progressive saving
	 * @return vector of Candle structures
	 * @throws std::exception
	 * @see https://www.mexc.com/api-docs/futures/market-endpoints#get-contract-kline
	 */
	[[nodiscard]] std::vector<Candle>
	getHistoricalPrices(const std::string &symbol, CandleInterval interval, std::int64_t startTime,
	                    std::int64_t endTime, const onCandlesDownloaded &writer = {}) const;
};
}

#endif //INCLUDE_VK_MEXC_FUTURES_REST_CLIENT_H
