#include "vk/mexc/mexc.h"
#include "vk/mexc/mexc_spot_rest_client.h"
#include "vk/utils/json_utils.h"
#include <memory>
#include <fstream>
#include <thread>
#include <spdlog/spdlog.h>
#include <future>
#include "vk/mexc/mexc_enums.h"
#include "vk/mexc/mexc_futures_rest_client.h"
#include "vk/utils/utils.h"

using namespace vk::mexc;
using namespace std::chrono_literals;

constexpr int HISTORY_LENGTH_IN_S = 86400 * 7; // 7 days

bool checkCandles(const std::vector<spot::Candle>& candles, const CandleInterval interval) {
    const auto mSecs = MEXC::numberOfMsForCandleInterval(interval);

    for (auto i = 0; i < candles.size() - 1; i++) {
        if (const auto timeDiff = candles[i + 1].m_openTime - candles[i].m_openTime; timeDiff != mSecs) {
            return false;
        }
    }

    return true;
}

void saveCandles(const std::vector<spot::Candle>& candles, const std::string& filePath) {
    if (std::ofstream file(filePath); !file.is_open()) {
        spdlog::error("Failed to open file {}", filePath);
    }
    else {
        file << "OpenTime" << "," << "CloseTime" << "," << "Open" << "," << "High" << "," << "Low" << "," << "Close" <<
            "," << "Volume" << std::endl;
        for (const auto& c : candles) {
            file << c.m_openTime << "," << c.m_closeTime << "," << c.m_open.str(8, std::ios_base::fixed) << "," << c.
                m_high.str(8, std::ios_base::fixed) << "," << c.m_low.str(8, std::ios_base::fixed) << "," << c.m_close.
                str(8, std::ios_base::fixed) << "," << c.m_volume.str(10, std::ios_base::fixed) << std::endl;
        }
    }
}

void testHistory() {
    try {
        const auto restClient = std::make_unique<spot::RESTClient>("", "");

        const auto from = std::chrono::seconds(std::time(nullptr)).count() - HISTORY_LENGTH_IN_S;
        const auto to = std::chrono::seconds(std::time(nullptr)).count();

        const auto candles = restClient->getHistoricalPrices("PLSUSDT", CandleInterval::_1m, from * 1000, to * 1000);

        if (const auto isOK = checkCandles(candles, CandleInterval::_1m); !isOK) {
            spdlog::error("Gaps in candles!");
        }
        else {
            saveCandles(candles, "PLSUSDT.csv");
            spdlog::info("Candles OK");
        }
    }
    catch (std::exception& e) {
        spdlog::error("{}", e.what());
    }
}

void testTickerPrice() {
    try {
        const auto restClient = std::make_unique<spot::RESTClient>("", "");

        if (const auto tickerPrices = restClient->getTickerPrice("PLSUSDT"); tickerPrices.size() == 1) {
            spdlog::info("Ticker price = {}", tickerPrices[0].m_price.convert_to<std::string>());
        }
    }
    catch (std::exception& e) {
        spdlog::error("{}", e.what());
    }
}

void testServerTime() {
    const auto restClient = std::make_unique<futures::RESTClient>("", "");
    const auto time = restClient->getServerTime();
    spdlog::info("Server time: {}", time);
}

int main() {
    //testHistory();
    //testTickerPrice();
    testServerTime();
    return getchar();
}
