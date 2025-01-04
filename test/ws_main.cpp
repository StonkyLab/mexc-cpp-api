#include <spdlog/spdlog.h>

#include "vk/mexc/mexc.h"
#include "vk/mexc/mexc_futures_ws_client.h"
#include "vk/tools/json_utils.h"

using namespace vk::mexc;

void logFunction(const vk::LogSeverity severity, const std::string& errmsg) {
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

int main() {
    const auto wsClient = futures::WSClient();

    futures::WSSubscription subscription;
    subscription.m_method = "sub.ticker";
    subscription.m_parameters.m_symbol = "ETH_USDT";

    const auto subscriptionStr = subscription.toJson().dump();
    wsClient.subscribe(subscriptionStr);
    wsClient.run();
    return getchar();
}
