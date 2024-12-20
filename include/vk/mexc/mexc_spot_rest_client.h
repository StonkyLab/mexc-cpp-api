/**
MEXC Spot REST Client

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#ifndef INCLUDE_VK_MEXC_REST_SPOT_CLIENT_H
#define INCLUDE_VK_MEXC_REST_SPOT_CLIENT_H

#include <string>
#include <memory>
#include "mexc_models.h"
#include "mexc_enums.h"

namespace vk::mexc::spot {
class RESTClient {
    struct P;
    std::unique_ptr<P> m_p{};

public:
    RESTClient(const std::string& apiKey, const std::string& apiSecret);

    ~RESTClient();

    /**
     * Set credentials to the RESTClient instance, it will reset the underlying HTTP Session
     * @param apiKey
     * @param apiSecret
     */
    void setCredentials(const std::string& apiKey, const std::string& apiSecret) const;

    /**
     * Download historical candles
     * @param symbol e,g BTCUSDT
     * @param interval
     * @param startTime timestamp in ms, must be smaller than "to"
     * @param endTime timestamp in ms
     * @param limit maximum number of returned candles, default values is 500
     * @return vector of Candle structures
     * @throws nlohmann::json::exception, std::exception
     * @see https://mexcdevelop.github.io/apidocs/spot_v3_en/#kline-candlestick-data
     */
    [[nodiscard]] std::vector<Candle>
    getHistoricalPrices(const std::string& symbol, CandleInterval interval, std::int64_t startTime,
                        std::int64_t endTime,
                        std::int32_t limit = 500) const;

    /**
     * Returns server time in ms
     * @return timestamp in ms
     * @throws nlohmann::json::exception, std::exception
     * @see https://mexcdevelop.github.io/apidocs/spot_v3_en/#check-server-time
    */
    [[nodiscard]] std::int64_t getServerTime() const;

    /**
     * Returns Ticker price for a specified symbol
     * @param symbol Prices of all symbols will be sent if symbol was not given
     * @return Filled TickerPrice structure
     * @throws nlohmann::json::exception, std::exception
     * @see https://mexcdevelop.github.io/apidocs/spot_v3_en/#symbol-price-ticker
     */
    std::vector<TickerPrice> getTickerPrice(const std::string& symbol) const;
};
}

#endif // INCLUDE_VK_MEXC_REST_SPOT_CLIENT_H
