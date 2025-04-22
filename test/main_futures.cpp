#include "vk/mexc/mexc.h"
#include "vk/mexc/mexc_spot_rest_client.h"
#include "vk/utils/json_utils.h"
#include <memory>
#include <fstream>
#include <thread>
#include <spdlog/spdlog.h>
#include <future>
#include "vk/mexc/mexc_futures_rest_client.h"
#include "vk/utils/utils.h"

using namespace vk::mexc::futures;
using namespace std::chrono_literals;

constexpr int HISTORY_LENGTH_IN_S = 86400 * 7; // 7 days

void testServerTime() {
    const auto restClient = std::make_unique<RESTClient>("", "");
    const auto time = restClient->getServerTime();
    spdlog::info("Server time: {}", time);
}

void testFundingRates() {
    const auto restClient = std::make_unique<RESTClient>("", "");
    const auto fr = restClient->getContractFundingRate("ETH_USDT");
    spdlog::info("Funding rate: {}", fr.m_fundingRate.convert_to<std::string>());

    const auto frs = restClient->getContractFundingRates();
}


int main() {
    //testHistory();
    testFundingRates();
    testServerTime();
    return getchar();
}
