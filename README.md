# mexc_cpp_api

C++ connector library for MEXC cryptocurrency exchange API (Spot v3 and Futures).

## 🚫 Important Limitations

> **This library is for MARKET DATA ONLY - NO TRADING SUPPORT!**

- **Spot API** - Trading endpoints exist but are NOT implemented in this library
- **Futures API** - All trading endpoints are permanently **"Under Maintenance"** on MEXC side. MEXC does not allow API trading for futures.

## Features

- REST API client for Spot market data
- REST API client for Futures market data
- WebSocket client for real-time Futures market data
- Historical candlestick (OHLCV) data download with backward pagination
- Funding rate data (current and historical)
- Rate limiting support

## Requirements

- C++20 compiler
- CMake 3.20+
- Boost 1.83+ (ASIO, Beast)
- OpenSSL
- nlohmann_json
- spdlog
- magic_enum

## Building

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## Usage Examples

### Spot REST Client - Market Data

```cpp
#include "vk/mexc/mexc_spot_rest_client.h"

using namespace vk::mexc::spot;

// Create client (empty credentials for public endpoints)
RESTClient client("", "");

// Get server time
auto serverTime = client.getServerTime();

// Get ticker price
auto tickers = client.getTickerPrice("BTCUSDT");
for (const auto& ticker : tickers) {
    std::cout << ticker.symbol << ": " << ticker.price << std::endl;
}
```

### Spot Historical Candlestick Data

```cpp
#include "vk/mexc/mexc_spot_rest_client.h"

using namespace vk::mexc::spot;

RESTClient client("", "");

// Download historical candles with automatic backward pagination
auto candles = client.getHistoricalPrices(
    "BTCUSDT",
    CandleInterval::_1h,
    startTimestamp,   // ms
    endTimestamp);    // ms

std::cout << "Downloaded " << candles.size() << " candles" << std::endl;

// With callback for streaming large datasets
client.getHistoricalPrices(
    "BTCUSDT",
    CandleInterval::_1h,
    startTimestamp,
    endTimestamp,
    [](const std::vector<Candle>& batch) {
        // Process each batch as it arrives
        std::cout << "Received batch of " << batch.size() << " candles" << std::endl;
    });
```

### Futures REST Client - Market Data

```cpp
#include "vk/mexc/mexc_futures_rest_client.h"

using namespace vk::mexc::futures;

RESTClient client("", "");

// Get server time
auto serverTime = client.getServerTime();

// Get current funding rate for a contract
auto fundingRate = client.getContractFundingRate("BTC_USDT");
std::cout << "Funding rate: " << fundingRate.fundingRate << std::endl;

// Get funding rates for all contracts
auto allRates = client.getContractFundingRates();
```

### Futures Historical Candlestick Data

```cpp
#include "vk/mexc/mexc_futures_rest_client.h"

using namespace vk::mexc::futures;

RESTClient client("", "");

// Note: timestamps are in SECONDS for futures API
auto candles = client.getHistoricalPrices(
    "BTC_USDT",
    CandleInterval::_1h,
    startTimeSec,
    endTimeSec);

// With callback
client.getHistoricalPrices(
    "BTC_USDT",
    CandleInterval::_1h,
    startTimeSec,
    endTimeSec,
    [](const std::vector<Candle>& batch) {
        for (const auto& candle : batch) {
            std::cout << "Time: " << candle.openTime << std::endl;
        }
    });
```

### Funding Rate History

```cpp
#include "vk/mexc/mexc_futures_rest_client.h"

using namespace vk::mexc::futures;

RESTClient client("", "");

// Get historical funding rates with pagination
auto history = client.getContractFundingRateHistory(
    "BTC_USDT",
    1,      // page number
    1000);  // page size (max 1000)

std::cout << "Total records: " << history.totalPage * history.pageSize << std::endl;

for (const auto& fr : history.resultList) {
    std::cout << "Time: " << fr.settleTime 
              << " Rate: " << fr.fundingRate << std::endl;
}
```

### Spot Listen Key (User Data Stream)

```cpp
#include "vk/mexc/mexc_spot_rest_client.h"

using namespace vk::mexc::spot;

RESTClient client("your_api_key", "your_api_secret");

// Get a new listen key
auto listenKey = client.getListenKey();

// Renew listen key (call every 30 minutes)
client.renewListenKey(listenKey);

// Get all active listen keys
auto allKeys = client.getListenKeys();

// Close listen key when done
client.closeListenKey(listenKey);
```

### WebSocket - Real-time Futures Data

```cpp
#include "vk/mexc/mexc_futures_ws_client.h"

using namespace vk::mexc::futures;

WebSocketClient wsClient;

// Subscribe to ticker stream
nlohmann::json subRequest;
subRequest["method"] = "sub.ticker";
subRequest["symbol"] = "BTC_USDT";

wsClient.subscribe(subRequest, [](const Event& event) {
    // Handle real-time ticker updates
    std::cout << "Ticker update received" << std::endl;
});
```

## ⚠️ Historical Data Limits

MEXC API has **undocumented limits** for historical candlestick data. Complete history is NOT available for all intervals!

### Spot API Limits

| Interval | Available History |
|----------|-------------------|
| 1m       | ~30 days          |
| 5m       | ~270 days (~9 months) |
| 15m      | ~270 days (~9 months) |
| 30m      | ~270 days (~9 months) |
| **1h**   | **5+ years (complete)** |
| 4h       | 5+ years (complete) |
| 1d       | Complete history  |

### Futures API Limits

| Interval | Available History |
|----------|-------------------|
| 1m       | ~30 days          |
| 5m       | ~360 days (~1 year) |
| 15m      | ~180-365 days     |
| **30m**  | **5+ years (complete)** |
| **1h**   | **5+ years (complete)** |
| 1d       | Complete history  |

> **Recommendation:** Use **1h (hourly)** or larger intervals for complete historical data.

## Candle Intervals

| Interval | Enum Value |
|----------|------------|
| 1 minute | `CandleInterval::_1m` |
| 5 minutes | `CandleInterval::_5m` |
| 15 minutes | `CandleInterval::_15m` |
| 30 minutes | `CandleInterval::_30m` |
| 1 hour | `CandleInterval::_1h` |
| 4 hours | `CandleInterval::_4h` |
| 1 day | `CandleInterval::_1d` |
| 1 week | `CandleInterval::_1w` |
| 1 month | `CandleInterval::_1M` |

## API Differences: Spot vs Futures

| Feature | Spot API | Futures API |
|---------|----------|-------------|
| Timestamps | Milliseconds | Seconds |
| Symbol format | `BTCUSDT` | `BTC_USDT` |
| Candle limit | 1000 per request | 2000 per request |
| Trading | Not implemented | Under Maintenance |

## Project Structure

```
mexc_cpp_api/
├── include/vk/mexc/
│   ├── mexc_spot_rest_client.h      # Spot REST API client
│   ├── mexc_futures_rest_client.h   # Futures REST API client
│   ├── mexc_futures_ws_client.h     # WebSocket client
│   ├── mexc_futures_ws_session.h    # WebSocket session (PIMPL)
│   ├── mexc_http_spot_session.h     # Spot HTTP session
│   ├── mexc_http_futures_session.h  # Futures HTTP session
│   ├── mexc_models.h                # Data models
│   ├── mexc_enums.h                 # Enumerations
│   └── ...
├── src/
│   ├── mexc_spot_rest_client.cpp
│   ├── mexc_futures_rest_client.cpp
│   ├── mexc_futures_ws_session.cpp
│   └── ...
├── vk_cpp_common/                   # Common utilities submodule
└── test/
    └── ws_main.cpp
```

## API Documentation

- Spot API v3: https://mexcdevelop.github.io/apidocs/spot_v3_en/
- Futures API: https://www.mexc.com/api-docs/futures/

## License

MIT License - see source files for details.

## Author

Vitezslav Kot <vitezslav.kot@gmail.com>