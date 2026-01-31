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
#include <functional>
#include "mexc_models.h"
#include "mexc_enums.h"

namespace vk::mexc::spot {

using onCandlesDownloaded = std::function<void(const std::vector<Candle>&)>;

class RESTClient {
    struct P;
    std::unique_ptr<P> m_p{};

public:
    RESTClient(const std::string &apiKey, const std::string &apiSecret);

    ~RESTClient();

    /**
     * Set credentials to the RESTClient instance, it will reset the underlying HTTP Session
     * @param apiKey
     * @param apiSecret
     */
    void setCredentials(const std::string &apiKey, const std::string &apiSecret) const;

    /**
     * Download historical candles with backward pagination
     * @param symbol e.g. BTCUSDT
     * @param interval candle interval
     * @param startTime timestamp in ms
     * @param endTime timestamp in ms
     * @param writer optional callback called after each batch of candles is downloaded for progressive saving
     * @return vector of Candle structures (last incomplete candle is removed)
     * @throws nlohmann::json::exception, std::exception
     * @see https://mexcdevelop.github.io/apidocs/spot_v3_en/#kline-candlestick-data
     */
    [[nodiscard]] std::vector<Candle>
    getHistoricalPrices(const std::string &symbol, CandleInterval interval, std::int64_t startTime,
                        std::int64_t endTime, const onCandlesDownloaded &writer = {}) const;

    /**
     * Returns server time in ms
     * @return timestamp in ms
     * @throws nlohmann::json::exception, std::exception
     * @see https://mexcdevelop.github.io/apidocs/spot_v3_en/#check-server-time
    */
    [[nodiscard]] std::int64_t getServerTime() const;

    /**
     * Returns Ticker price for a specified symbol
     * @param symbol Prices of all symbols will be sent if the symbol was not given
     * @return Filled TickerPrice structure
     * @throws nlohmann::json::exception, std::exception
     * @see https://mexcdevelop.github.io/apidocs/spot_v3_en/#symbol-price-ticker
     */
    [[nodiscard]] std::vector<TickerPrice> getTickerPrice(const std::string &symbol) const;

    /**
     * Starts a new data stream and return a listenKey for it. The stream will close 60 minutes after creation
     * unless a keepalive is sent.
     * @see https://www.mexc.com/api-docs/spot-v3/websocket-user-data-streams
     * @return valid listen key
     */
    [[nodiscard]] std::string getListenKey() const;

    /**
     * Retrieves all currently valid listen keys.
     * @see https://www.mexc.com/api-docs/spot-v3/websocket-user-data-streams
     * @return ListenKeys structure
     */
    [[nodiscard]] ListenKeys getListenKeys() const;

    /**
     * Extends the validity to 60 minutes from the time of this call. It is recommended to send a request every 30 minutes.
     * @param listenKey
     * @return renewed listen key
     */
    [[nodiscard]] std::string renewListenKey(const std::string &listenKey) const;

    /**
     * Closes the user data stream.
     * @param listenKey
     * @return closed listen key
     */
    [[nodiscard]] std::string closeListenKey(const std::string &listenKey) const;
};
}

#endif // INCLUDE_VK_MEXC_REST_SPOT_CLIENT_H
