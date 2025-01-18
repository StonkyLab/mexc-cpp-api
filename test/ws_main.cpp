#include <spdlog/spdlog.h>

#include "vk/mexc/mexc.h"
#include "vk/mexc/mexc_futures_ws_client.h"
#include "vk/utils/json_utils.h"

using namespace vk::mexc;

int main() {
    const auto wsClient = futures::WSClient();

    wsClient.setDataEventCallback([&](const futures::Event &event) {
        if (event.m_channel == "push.ticker") {
            defaultLogFunction(vk::LogSeverity::Info, event.m_data.dump());
        }
    });

    {
        futures::WSSubscription subscriptionRequest;
        subscriptionRequest.m_method = "sub.ticker";
        subscriptionRequest.m_parameters.m_symbol = "ETH_USDT";
        const auto subscriptionStr = subscriptionRequest.toJson().dump();
        wsClient.subscribe(subscriptionRequest.toJson());
    }

    {
        futures::WSSubscription subscriptionRequest;
        subscriptionRequest.m_method = "sub.ticker";
        subscriptionRequest.m_parameters.m_symbol = "BTC_USDT";
        const auto subscriptionStr = subscriptionRequest.toJson().dump();
        wsClient.subscribe(subscriptionRequest.toJson());
    }
    return getchar();
}
