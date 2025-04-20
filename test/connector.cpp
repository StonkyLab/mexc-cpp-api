#include <vk/mexc/mexc_exchange_connector.h>

using namespace vk;

void testTickerInfo() {
    const auto connector = std::make_unique<MEXCFuturesExchangeConnector>();

    for (auto ticker : connector->getTickerInfo("BTCUSDT")) {
        spdlog::info("Symbol: {}", ticker.symbol);
    }
}

int main() {
    testTickerInfo();
    return getchar();
}
