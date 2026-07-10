/**
MEXC futures position-mode reset tool.

Takes the API key + secret as command-line arguments, FLATTENS every open
position on the futures account with reduce-only MARKET orders, then reports the
account's trading mode (Hedge or One-way) and offers to keep it or switch to the
other. MEXC only permits a mode switch when the book is flat with no open orders,
which is why positions are closed first. After applying the choice the mode is
re-read from the venue and printed, so the reported value is the real account
state — not just what we asked for.

Usage:  mexc_set_position_mode <API_KEY> <API_SECRET>

NOTE: this hits MEXC production (futures has no testnet host) and closes REAL
positions. There is no undo.
*/

#include "stonky/mexc/mexc_futures_rest_client.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

using namespace stonky::mexc::futures;

namespace {

constexpr const char *modeName(const PositionMode mode) {
    return mode == PositionMode::Hedge ? "Hedge" : "One-way";
}

/// Flatten every open position with reduce-only MARKET orders, then poll until
/// the venue reports the book flat (the position query lags the fills by a
/// moment, and a mode switch requires a genuinely flat book).
void closeAllPositions(const RESTClient &rest) {
    const auto positions = rest.getOpenPositions();
    spdlog::info("open positions: {}", positions.size());

    for (const auto &pos: positions) {
        const double holdVol = pos.holdVol.convert_to<double>();
        if (holdVol <= 0.0) continue;

        OrderRequest req;
        req.symbol = pos.symbol;
        req.price = 0.0;
        req.vol = holdVol;
        // positionType 1=long → CloseLong (sell); 2=short → CloseShort (buy).
        req.side = pos.positionType == 1 ? OrderSide::CloseLong : OrderSide::CloseShort;
        req.type = OrderType::Market;
        // Match the position's own margin mode, and set positionId so the long
        // vs short leg is unambiguous when the account is in hedge mode.
        req.openType = static_cast<MarginType>(pos.openType);
        req.positionId = pos.positionId;

        try {
            const auto resp = rest.submitOrder(req);
            spdlog::info("closed {} {} vol={} -> orderId={}", pos.symbol,
                         pos.positionType == 1 ? "LONG" : "SHORT", holdVol, resp.orderId);
        } catch (std::exception &e) {
            spdlog::error("close {} failed: {}", pos.symbol, e.what());
        }
    }

    // Wait for the venue to register the flat book before offering the switch.
    constexpr int maxChecks = 10;
    for (int attempt = 0; attempt < maxChecks; ++attempt) {
        const auto remaining = rest.getOpenPositions();
        if (remaining.empty()) {
            spdlog::info("account is flat");
            return;
        }
        if (attempt + 1 == maxChecks) {
            spdlog::warn("still {} open position(s) after closing — a mode switch may be rejected:", remaining.size());
            for (const auto &p: remaining) {
                spdlog::warn("  STILL OPEN: {} type={} vol={}", p.symbol, p.positionType, p.holdVol.convert_to<double>());
            }
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

/// Read a 1/2 choice from stdin. On EOF (non-interactive stdin) default to 1
/// (keep) — the safe no-op that never touches the account's mode.
int readChoice() {
    const auto trim = [](std::string s) {
        s.erase(0, s.find_first_not_of(" \t\r\n"));
        s.erase(s.find_last_not_of(" \t\r\n") + 1);
        return s;
    };

    for (;;) {
        std::cout << "Select [1/2]: " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) {
            std::cout << "\nNo input (EOF) — keeping current mode.\n";
            return 1;
        }
        line = trim(line);
        if (line == "1") return 1;
        if (line == "2") return 2;
        std::cout << "Please enter 1 or 2.\n";
    }
}

} // namespace

int main(int argc, char **argv) {
    spdlog::set_level(spdlog::level::info);

    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <API_KEY> <API_SECRET>\n";
        return 1;
    }

    const std::string apiKey = argv[1];
    const std::string apiSecret = argv[2];

    try {
        const RESTClient rest(apiKey, apiSecret);

        // 1) Flatten everything — a mode switch requires a flat book.
        spdlog::info("MEXC futures position-mode reset — closing all positions first");
        closeAllPositions(rest);

        // 2) Report the current trading mode; build the menu around it.
        const PositionMode current = rest.getPositionMode();
        const PositionMode other = current == PositionMode::Hedge ? PositionMode::OneWay : PositionMode::Hedge;

        std::cout << "\nCurrent futures trading mode: " << modeName(current) << "\n\n"
                  << "  1 - Keep " << modeName(current) << "\n"
                  << "  2 - Set "  << modeName(other) << "\n\n";

        // 3) Ask, then keep or switch.
        if (readChoice() == 2) {
            spdlog::info("switching position mode {} -> {}", modeName(current), modeName(other));
            try {
                rest.changePositionMode(other);
                std::this_thread::sleep_for(std::chrono::milliseconds(500)); // let the switch propagate
            } catch (std::exception &e) {
                spdlog::error("changePositionMode failed: {}", e.what());
            }
        } else {
            spdlog::info("keeping position mode {}", modeName(current));
        }

        // 4) Re-read from the venue so the printed mode is the real account state.
        const PositionMode confirmed = rest.getPositionMode();
        std::cout << "\nActive futures trading mode is now: " << modeName(confirmed) << "\n";
    } catch (std::exception &e) {
        spdlog::critical("exception: {}", e.what());
        return 1;
    }

    return 0;
}
