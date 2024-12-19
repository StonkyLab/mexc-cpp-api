#include "vk/mexc/mexc.h"
#include "vk/mexc/mexc_spot_rest_client.h"
#include "vk/tools/json_utils.h"
#include <memory>
#include <iostream>
#include <fstream>
#include <thread>
#include <spdlog/spdlog.h>
#include <future>
#include "vk/mexc/mexc_enums.h"
#include "vk/tools/utils.h"

using namespace vk::mexc;
using namespace std::chrono_literals;

constexpr int HISTORY_LENGTH_IN_S = 86400; // 1 day

bool checkCandles(const std::vector<Candle> &candles, const CandleInterval interval) {

    const auto mSecs = MEXC::numberOfMsForCandleInterval(interval);

    for (auto i = 0; i < candles.size() - 1; i++) {
        if (const auto timeDiff = candles[i + 1].m_openTime - candles[i].m_openTime; timeDiff != mSecs) {
            return false;
        }
    }

    return true;
}

void testHistory() {

    try {
        const auto restClient = std::make_unique<spot::RESTClient>("", "");

        const auto from = std::chrono::seconds(std::time(nullptr)).count() - HISTORY_LENGTH_IN_S;
        const auto to = std::chrono::seconds(std::time(nullptr)).count();

        const auto candles = restClient->getHistoricalPrices("PLSUSDT", CandleInterval::_1m, from * 1000, to * 1000);

        if (const auto isOK = checkCandles(candles, CandleInterval::_1m); !isOK) {
            spdlog::error("Gaps in candles!");
        }else {
            spdlog::info("Candles OK");
        }
    }
    catch (std::exception &e) {
        spdlog::error("{}", e.what());
    }
}

int main() {
    testHistory();
    return getchar();
}
