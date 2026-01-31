# MEXC C++ API

C++ library for interacting with MEXC Exchange API (Spot and Futures).

## 🚫 Important Limitations

> **This library is for MARKET DATA ONLY - NO TRADING SUPPORT!**

- **Spot API** - Trading endpoints exist but are NOT implemented in this library
- **Futures API** - All trading endpoints are permanently **"Under Maintenance"** on MEXC side. MEXC does not allow API trading for futures.

## Features

- **Spot API** - REST client for spot market data
- **Futures API** - REST client for futures market data
- Historical candlestick (OHLCV) data download
- Rate limiting support

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

## API Rate Limits

- Spot API: Rate limited (automatic handling)
- Futures API: Rate limited (automatic handling)

## Building

```bash
mkdir build && cd build
cmake ..
make
```

## Dependencies

- Boost (Beast, Asio, Multiprecision)
- nlohmann/json
- OpenSSL
- spdlog

## License

MIT License