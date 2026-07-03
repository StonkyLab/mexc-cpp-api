/**
Read-only probe of the MEXC data path the DirtyCarryMexcExecutor depends on:
account balance, funding-rate ranking (the candidate universe), open positions,
and contract sizes. Places NO orders. Uses the testing .env.
*/

#include "stonky/mexc/mexc_futures_rest_client.h"
#include <spdlog/spdlog.h>
#include <algorithm>
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
    const auto trim = [](std::string s) {
        s.erase(0, s.find_first_not_of(" \t\r\n"));
        s.erase(s.find_last_not_of(" \t\r\n") + 1);
        return s;
    };
    while (std::getline(ifs, line)) {
        line = trim(line);
        if (line.empty() || line.front() == '#') continue;
        if (const auto pos = line.find('='); pos != std::string::npos) env[trim(line.substr(0, pos))] = trim(line.substr(pos + 1));
    }
    return env;
}
} // namespace

int main() {
    spdlog::set_level(spdlog::level::info);
    const auto* home = std::getenv("HOME");
    const auto env = readEnvFile(std::filesystem::path(home ? home : "") / ".config/crypto-portfolio/mexc/testing.env");

    if (!env.contains("API_KEY") || !env.contains("API_SECRET")) {
        spdlog::critical("Missing creds");
        return 1;
    }

    try {
        const RESTClient rest(env.at("API_KEY"), env.at("API_SECRET"));

        // Balance (equity) — the sizing base.
        const auto wallet = rest.getWalletBalance("USDT");
        spdlog::info("USDT equity={} available={}", wallet.equity.str(), wallet.availableBalance.str());

        // Contract sizes (needed to convert positions contracts -> base).
        const auto details = rest.getContractDetails();
        spdlog::info("contracts: {}", details.size());

        // Open positions.
        const auto positions = rest.getOpenPositions();
        spdlog::info("open positions: {}", positions.size());
        for (const auto& p: positions) {
            spdlog::info("  pos {} type={} holdVol={}", p.symbol, p.positionType, p.holdVol.convert_to<double>());
        }

        // Funding-rate ranking (the candidate universe), USDT perps only.
        const auto frs = rest.getContractFundingRates();
        std::vector<std::pair<std::string, double>> ranked;
        for (const auto& fr: frs) {
            if (fr.symbol.ends_with("_USDT") && fr.fundingRate != 0) {
                ranked.emplace_back(fr.symbol, fr.fundingRate.convert_to<double>());
            }
        }
        std::ranges::sort(ranked, [](const auto& a, const auto& b) { return a.second < b.second; });
        spdlog::info("USDT funding candidates: {}", ranked.size());

        spdlog::info("--- most NEGATIVE funding (would LONG) ---");
        for (std::size_t i = 0; i < std::min<std::size_t>(5, ranked.size()); ++i) {
            spdlog::info("  {} {:+.4f}%", ranked[i].first, ranked[i].second * 100.0);
        }
        spdlog::info("--- most POSITIVE funding (would SHORT) ---");
        for (std::size_t i = 0; i < std::min<std::size_t>(5, ranked.size()); ++i) {
            const auto& e = ranked[ranked.size() - 1 - i];
            spdlog::info("  {} {:+.4f}%", e.first, e.second * 100.0);
        }
    } catch (std::exception& e) {
        spdlog::critical("probe exception: {}", e.what());
        return 1;
    }

    return 0;
}
