#include <vk/mexc/mexc_futures_exchange_connector.h>

using namespace vk;

void testTickerInfo() {
    const auto connector = std::make_unique<MEXCFuturesExchangeConnector>();

    for (auto ticker : connector->getSymbolInfo("BTCUSDT")) {
        spdlog::info("Symbol: {}", ticker.symbol);
    }
}

int main() {
    testTickerInfo();
    return getchar();
}
