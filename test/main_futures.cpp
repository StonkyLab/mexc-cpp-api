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
#include "vk/utils/utils.h"

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

int main(int argc, char **argv) {
	if (argc <= 1) {
		spdlog::error("No parameters!");
		return -1;
	}

	try {
		const auto credentials = readCredentials(argv[1]);
		testWalletBalance(credentials);

		//testHistory();
		//testFundingRates();
		//testServerTime();
	} catch (const std::exception &e) {
		spdlog::error("Exception: {}", e.what());
	}

	return getchar();
}
