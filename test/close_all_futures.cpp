/**
MEXC futures safety cleanup: cancel all open orders and flatten all positions
on the account with reduce-only MARKET orders. Uses the testing .env.
Read-only-safe to run any time (no-op when the account is already flat).
*/

#include "stonky/mexc/mexc_futures_rest_client.h"
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>

using namespace stonky::mexc::futures;

namespace {
std::map<std::string, std::string> readEnvFile(const std::filesystem::path& path) {
    std::map<std::string, std::string> env;
    std::ifstream ifs(path.string());
    std::string line;

    while (std::getline(ifs, line)) {
        const auto trim = [](std::string s) {
            s.erase(0, s.find_first_not_of(" \t\r\n"));
            s.erase(s.find_last_not_of(" \t\r\n") + 1);
            return s;
        };
        line = trim(line);
        if (line.empty() || line.front() == '#') continue;
        if (const auto pos = line.find('='); pos != std::string::npos) env[trim(line.substr(0, pos))] = trim(line.substr(pos + 1));
    }

    return env;
}
} // namespace

int main() {
    spdlog::set_level(spdlog::level::debug);
    const auto* home = std::getenv("HOME");
    const auto env = readEnvFile(std::filesystem::path(home ? home : "") / ".config/crypto-portfolio/mexc/testing.env");

    if (!env.contains("API_KEY") || !env.contains("API_SECRET")) {
        spdlog::critical("Missing creds");
        return 1;
    }

    try {
        const RESTClient rest(env.at("API_KEY"), env.at("API_SECRET"));

        const auto positions = rest.getOpenPositions();
        spdlog::info("open positions: {}", positions.size());

        for (const auto& pos: positions) {
            const double holdVol = pos.holdVol.convert_to<double>();
            if (holdVol <= 0.0) continue;

            OrderRequest req;
            req.symbol = pos.symbol;
            req.price = 0.0;
            req.vol = holdVol;
            // positionType 1=long → CloseLong (sell); 2=short → CloseShort (buy)
            req.side = pos.positionType == 1 ? OrderSide::CloseLong : OrderSide::CloseShort;
            req.type = OrderType::Market;
            req.openType = MarginType::Cross;

            try {
                const auto resp = rest.submitOrder(req);
                spdlog::info("closed {} {} vol={} -> orderId={}", pos.symbol, pos.positionType == 1 ? "LONG" : "SHORT", holdVol, resp.orderId);
            } catch (std::exception& e) {
                spdlog::error("close {} failed: {}", pos.symbol, e.what());
            }
        }

        const auto after = rest.getOpenPositions();
        spdlog::info("positions after cleanup: {}", after.size());
        for (const auto& pos: after) {
            spdlog::warn("STILL OPEN: {} type={} vol={}", pos.symbol, pos.positionType, pos.holdVol.convert_to<double>());
        }
    } catch (std::exception& e) {
        spdlog::critical("cleanup exception: {}", e.what());
        return 1;
    }

    return 0;
}
