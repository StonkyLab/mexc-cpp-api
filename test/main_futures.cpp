#include "vk/mexc/mexc.h"
#include "vk/mexc/mexc_spot_rest_client.h"
#include "vk/utils/json_utils.h"
#include <memory>
#include <fstream>
#include <thread>
#include <spdlog/spdlog.h>
#include <future>
#include <iostream>

#include "vk/mexc/mexc_futures_rest_client.h"
#include "vk/mexc/mexc_enums.h"
#include "vk/utils/utils.h"

using namespace vk::mexc;
using namespace vk::mexc::futures;
using namespace std::chrono_literals;

constexpr int HISTORY_LENGTH_IN_S = 86400 * 7; // 7 days

std::pair<std::string, std::string> readCredentials(const char *path) {
	std::filesystem::path pathToCfg{path};
	std::ifstream ifs(pathToCfg.string());

	if (!ifs.is_open()) {
		throw std::runtime_error("Couldn't open config file: " + pathToCfg.string());
	}

	try {
		std::string apiKey;
		std::string apiSecret;
		std::string subAccountName;

		nlohmann::json json = nlohmann::json::parse(ifs);
		vk::readValue<std::string>(json, "ApiKey", apiKey);
		vk::readValue<std::string>(json, "ApiSecret", apiSecret);

		std::pair retVal(apiKey, apiSecret);
		return retVal;
	} catch (const std::exception &e) {
		std::cerr << e.what();
		ifs.close();
	}

	return {};
}

void testServerTime() {
	const auto restClient = std::make_unique<RESTClient>("", "");
	const auto time = restClient->getServerTime();
	spdlog::info("Server time: {}", time);
}

void testFundingRates() {
	const auto restClient = std::make_unique<RESTClient>("", "");
	const auto fr = restClient->getContractFundingRate("ETH_USDT");
	//spdlog::info("Funding rate: {}", fr.m_fundingRate.convert_to<std::string>());

	const auto frs = restClient->getContractFundingRates();
}

void testWalletBalance(const std::pair<std::string, std::string> &credentials) {
	const auto restClient = std::make_unique<RESTClient>(credentials.first, credentials.second);
	const auto balance = restClient->getWalletBalance("USDT");
}

bool checkCandles(const std::vector<Candle> &candles, const CandleInterval interval) {
	const auto mSecs = MEXC::numberOfMsForCandleInterval(interval);

	if (candles.empty()) {
		return false;
	}

	for (size_t i = 0; i < candles.size() - 1; i++) {
		if (const auto timeDiff = candles[i + 1].openTime - candles[i].openTime; timeDiff != mSecs) {
			spdlog::warn("Gap at index {}: expected {} ms, got {} ms", i, mSecs, timeDiff);
			return false;
		}
	}

	return true;
}

void saveCandles(const std::vector<Candle> &candles, const std::string &filePath) {
	if (std::ofstream file(filePath); !file.is_open()) {
		spdlog::error("Failed to open file {}", filePath);
	} else {
		file << "OpenTime,Open,High,Low,Close,Volume,Amount" << std::endl;
		for (const auto &c: candles) {
			file << c.openTime << "," << c.open.str(8, std::ios_base::fixed) << ","
			     << c.high.str(8, std::ios_base::fixed) << "," << c.low.str(8, std::ios_base::fixed) << ","
			     << c.close.str(8, std::ios_base::fixed) << "," << c.volume.str(10, std::ios_base::fixed) << ","
			     << c.amount.str(10, std::ios_base::fixed) << std::endl;
		}
	}
}

void testHistory() {
	try {
		const auto restClient = std::make_unique<RESTClient>("", "");

		// Futures API uses timestamps in SECONDS
		const auto from = std::chrono::seconds(std::time(nullptr)).count() - HISTORY_LENGTH_IN_S;
		const auto to = std::chrono::seconds(std::time(nullptr)).count();

		spdlog::info("Fetching candles from {} to {}...", from, to);

		const auto candles = restClient->getHistoricalPrices("BTC_USDT", CandleInterval::_1m, from, to);

		spdlog::info("Received {} candles", candles.size());

		if (candles.empty()) {
			spdlog::error("No candles received!");
			return;
		}

		if (const auto isOK = checkCandles(candles, CandleInterval::_1m); !isOK) {
			spdlog::error("Gaps in candles!");
		} else {
			saveCandles(candles, "BTC_USDT_futures.csv");
			spdlog::info("Candles OK, saved to BTC_USDT_futures.csv");
		}
	} catch (std::exception &e) {
		spdlog::error("{}", e.what());
	}
}

int main(int argc, char **argv) {
	try {
		// Test history doesn't require credentials
		testHistory();
		testServerTime();
		testFundingRates();

		if (argc > 1) {
			const auto credentials = readCredentials(argv[1]);
			testWalletBalance(credentials);
		}
	} catch (const std::exception &e) {
		spdlog::error("Exception: {}", e.what());
	}

	return getchar();
}
