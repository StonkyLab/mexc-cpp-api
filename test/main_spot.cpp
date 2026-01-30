#include "vk/mexc/mexc.h"
#include "vk/mexc/mexc_spot_rest_client.h"
#include "vk/utils/json_utils.h"
#include <memory>
#include <fstream>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ranges.h>
#include <future>
#include "vk/mexc/mexc_enums.h"
#include "vk/utils/utils.h"

using namespace vk::mexc;
using namespace std::chrono_literals;

constexpr int HISTORY_LENGTH_IN_S = 86400 * 7; // 7 days

std::pair<std::string, std::string> readCredentials(const char *path) {
    std::filesystem::path pathToCfg{path};
    std::ifstream ifs(pathToCfg.string());

    if (!ifs.is_open()) {
        throw std::runtime_error("Couldn't open config file: " + pathToCfg.string());
    }

    std::string apiKey;
    std::string apiSecret;
    std::string subAccountName;

    nlohmann::json json = nlohmann::json::parse(ifs);
    vk::readValue<std::string>(json, "ApiKey", apiKey);
    vk::readValue<std::string>(json, "ApiSecret", apiSecret);

    std::pair retVal(apiKey, apiSecret);

    return retVal;
}

bool checkCandles(const std::vector<spot::Candle> &candles, const CandleInterval interval) {
    const auto mSecs = MEXC::numberOfMsForCandleInterval(interval);

    for (auto i = 0; i < candles.size() - 1; i++) {
        if (const auto timeDiff = candles[i + 1].openTime - candles[i].openTime; timeDiff != mSecs) {
            return false;
        }
    }

    return true;
}

void saveCandles(const std::vector<spot::Candle> &candles, const std::string &filePath) {
    if (std::ofstream file(filePath); !file.is_open()) {
        spdlog::error("Failed to open file {}", filePath);
    } else {
        file << "OpenTime" << "," << "CloseTime" << "," << "Open" << "," << "High" << "," << "Low" << "," << "Close" <<
                "," << "Volume" << std::endl;
        for (const auto &c: candles) {
            file << c.openTime << "," << c.closeTime << "," << c.open.str(8, std::ios_base::fixed) << "," << c.
                    high.str(8, std::ios_base::fixed) << "," << c.low.str(8, std::ios_base::fixed) << "," << c.
                    close.
                    str(8, std::ios_base::fixed) << "," << c.volume.str(10, std::ios_base::fixed) << std::endl;
        }
    }
}

void testHistory() {
    try {
        const auto restClient = std::make_unique<spot::RESTClient>("", "");

        const auto from = std::chrono::seconds(std::time(nullptr)).count() - HISTORY_LENGTH_IN_S;
        const auto to = std::chrono::seconds(std::time(nullptr)).count();

        const auto candles = restClient->getHistoricalPrices("BTCUSDT", CandleInterval::_1m, from * 1000, to * 1000);

        if (const auto isOK = checkCandles(candles, CandleInterval::_1m); !isOK) {
            spdlog::error("Gaps in candles!");
        } else {
            saveCandles(candles, "BTCUSDT.csv");
            spdlog::info("Candles OK");
        }
    } catch (std::exception &e) {
        spdlog::error("{}", e.what());
    }
}

void testTickerPrice() {
    try {
        const auto restClient = std::make_unique<spot::RESTClient>("", "");

        if (const auto tickerPrices = restClient->getTickerPrice("BTCUSDT"); tickerPrices.size() == 1) {
            spdlog::info("Ticker price = {}", tickerPrices[0].price.convert_to<std::string>());
        }
    } catch (std::exception &e) {
        spdlog::error("{}", e.what());
    }
}

void testServerTime() {
    const auto restClient = std::make_unique<spot::RESTClient>("", "");
    const auto time = restClient->getServerTime();
    spdlog::info("Server time: {}", time);
}

void testListenKey(const std::pair<std::string, std::string> &credentials) {
    const auto restClient = std::make_unique<spot::RESTClient>(credentials.first, credentials.second);
    //const auto listenKey = restClient->getListenKey();

    // const auto listenKey = restClient->renewListenKey("ef6cda06a8b3bed264bd3359ab11038b7225d7ad1ec59449563fea71b0af3e75");
    restClient->closeListenKey("ef6cda06a8b3bed264bd3359ab11038b7225d7ad1ec59449563fea71b0af3e75");
    //spdlog::info("Listen key: {}", listenKey);
}

void testListenKeys(const std::pair<std::string, std::string> &credentials) {
    const auto restClient = std::make_unique<spot::RESTClient>(credentials.first, credentials.second);
    const auto listenKeys = restClient->getListenKeys();
    spdlog::info("Listen keys: {}", listenKeys.listenKeys);
}

int main(int argc, char **argv) {
    try {
        // These tests don't require credentials
        testHistory();
        testServerTime();
        testTickerPrice();

        if (argc > 1) {
            const auto credentials = readCredentials(argv[1]);
            testListenKeys(credentials);
            testListenKey(credentials);
        }
    } catch (const std::exception &e) {
        spdlog::error("Exception: {}", e.what());
    }

    return getchar();
}
