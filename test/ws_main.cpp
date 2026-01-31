#include <spdlog/spdlog.h>

#include "vk/mexc/mexc.h"
#include "vk/mexc/mexc_futures_ws_client.h"
#include "vk/mexc/mexc_ws_stream_manager.h"
#include "vk/utils/json_utils.h"

using namespace std::chrono_literals;
using namespace vk::mexc;

void logFunction(const vk::LogSeverity severity, const std::string &errmsg) {
	switch (severity) {
		case vk::LogSeverity::Info:
			spdlog::info(errmsg);
			break;
		case vk::LogSeverity::Warning:
			spdlog::warn(errmsg);
			break;
		case vk::LogSeverity::Critical:
			spdlog::critical(errmsg);
			break;
		case vk::LogSeverity::Error:
			spdlog::error(errmsg);
			break;
		case vk::LogSeverity::Debug:
			spdlog::debug(errmsg);
			break;
		case vk::LogSeverity::Trace:
			spdlog::trace(errmsg);
			break;
	}
}

void testWSClient() {
	const auto wsClient = futures::WSClient();

	wsClient.setDataEventCallback([&](const futures::Event &event) {
		if (event.channel == "push.ticker") {
			defaultLogFunction(vk::LogSeverity::Info, event.data.dump());
		}
	}); {
		futures::WSSubscription subscriptionRequest;
		subscriptionRequest.method = "sub.ticker";
		subscriptionRequest.parameters.symbol = "ETH_USDT";
		const auto subscriptionStr = subscriptionRequest.toJson().dump();
		wsClient.subscribe(subscriptionRequest.toJson());
	} {
		futures::WSSubscription subscriptionRequest;
		subscriptionRequest.method = "sub.ticker";
		subscriptionRequest.parameters.symbol = "BTC_USDT";
		const auto subscriptionStr = subscriptionRequest.toJson().dump();
		wsClient.subscribe(subscriptionRequest.toJson());
	}
}

void testWSManagerTicker() {
	const auto wsManager = futures::WSStreamManager();
	wsManager.subscribeTickerStream("ETH_USDT");
	wsManager.setLoggerCallback(&logFunction);

	while (true) {
		{
			if (const auto ret = wsManager.readEventTicker("ETH_USDT")) {
				spdlog::info("ETH price: {}", ret->fairPrice);
			} else {
				spdlog::error("Error reading event");
			}
		}
		std::this_thread::sleep_for(1000ms);
	}
}

void testWSManagerCandle() {
	const auto wsManager = futures::WSStreamManager();
	wsManager.subscribeCandlestickStream("ETH_USDT", CandleInterval::_1m);
	wsManager.setLoggerCallback(&logFunction);

	while (true) {
		{
			if (const auto ret = wsManager.readEventCandlestick("ETH_USDT", CandleInterval::_1m)) {
				spdlog::info("ETH open price: {}", ret->open);
			} else {
				spdlog::error("Error reading event");
			}
		}
		std::this_thread::sleep_for(1000ms);
	}
}

int main() {
	testWSManagerCandle();
	return getchar();
}
