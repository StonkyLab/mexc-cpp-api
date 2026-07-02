/**
MEXC Futures OpenAPI auth smoke test.

Validates the OpenAPI HMAC-SHA256 signing (contract.mexc.com) end to end after
the webToken/Web path was removed:
  - public GET  (getServerTime)                 — connectivity
  - private GET (getWalletBalance/openPositions) — GET signature
  - private POST (cancelOrders with a bogus id)  — POST signature

The POST probe cancels a non-existent order id: it exercises the signature
without placing or modifying any real position. A business error ("order does
not exist") means the signature was ACCEPTED; a signature/auth error means POST
signing is broken. NOTE: MEXC futures has no testnet host here — everything
hits production, so the test never places a real order.

Credentials: ~/.config/crypto-portfolio/mexc/testing.env (API_KEY / API_SECRET).
*/

#include "stonky/mexc/mexc_futures_rest_client.h"
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>

using namespace stonky::mexc::futures;

namespace {
std::map<std::string, std::string> readEnvFile(const std::filesystem::path &path) {
    std::map<std::string, std::string> env;
    std::ifstream ifs(path.string());

    if (!ifs.is_open()) {
        spdlog::error("Couldn't open env file: {}", path.string());
        return env;
    }

    const auto trim = [](std::string s) {
        s.erase(0, s.find_first_not_of(" \t\r\n"));
        s.erase(s.find_last_not_of(" \t\r\n") + 1);

        if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') || (s.front() == '\'' && s.back() == '\''))) {
            s = s.substr(1, s.size() - 2);
        }

        return s;
    };

    std::string line;

    while (std::getline(ifs, line)) {
        line = trim(line);

        if (line.empty() || line.front() == '#') {
            continue;
        }

        if (line.starts_with("export ")) {
            line = line.substr(7);
        }

        if (const auto pos = line.find('='); pos != std::string::npos) {
            env[trim(line.substr(0, pos))] = trim(line.substr(pos + 1));
        }
    }

    return env;
}
} // namespace

int main() {
    spdlog::set_level(spdlog::level::debug);

    const auto *home = std::getenv("HOME");
    const auto env = readEnvFile(std::filesystem::path(home ? home : "") / ".config/crypto-portfolio/mexc/testing.env");
    const auto apiKey = env.contains("API_KEY") ? env.at("API_KEY") : "";
    const auto apiSecret = env.contains("API_SECRET") ? env.at("API_SECRET") : "";

    if (apiKey.empty() || apiSecret.empty()) {
        spdlog::critical("Missing API_KEY / API_SECRET in testing.env");
        return 1;
    }

    const auto restClient = std::make_unique<RESTClient>(apiKey, apiSecret);

    // 1) Public GET — connectivity.
    try {
        spdlog::info("server time: {}", restClient->getServerTime());
    } catch (std::exception &e) {
        spdlog::error("getServerTime failed: {}", e.what());
    }

    // 2) Private GET — validates the OpenAPI GET signature.
    try {
        const auto balance = restClient->getWalletBalance("USDT");
        spdlog::info("[GET auth OK] USDT wallet: available={} equity={}", balance.availableBalance.str(), balance.equity.str());
    } catch (std::exception &e) {
        spdlog::error("[GET auth] getWalletBalance failed: {}", e.what());
    }

    try {
        const auto positions = restClient->getOpenPositions();
        spdlog::info("[GET auth OK] open positions: {}", positions.size());
    } catch (std::exception &e) {
        spdlog::error("[GET auth] getOpenPositions failed: {}", e.what());
    }

    // 3) Private POST — validates the OpenAPI POST signature via a harmless
    // cancel of a non-existent order. Success (or a business "order not
    // exists" error) proves the signature was accepted.
    try {
        const auto response = restClient->cancelOrders({999999999999LL});
        spdlog::info("[POST auth OK] cancelOrders returned success (bogus id accepted for signing)");
    } catch (std::exception &e) {
        const std::string what = e.what();
        const bool authProblem = what.find("sign") != std::string::npos || what.find("Sign") != std::string::npos || what.find("602") != std::string::npos ||
                                  what.find("401") != std::string::npos;

        if (authProblem) {
            spdlog::error("[POST auth FAILED] signature rejected: {}", what);
        } else {
            spdlog::info("[POST auth OK] venue reached signing stage, rejected on business grounds: {}", what);
        }
    }

    return 0;
}
