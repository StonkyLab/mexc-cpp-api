/**
MEXC Futures WebSocket Stream manager v5

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#include "vk/mexc/mexc_futures_rest_client.h"
#include "vk/mexc/mexc_ws_stream_manager.h"
#include "vk/mexc/mexc_futures_ws_client.h"
#include "vk/utils/utils.h"
#include <mutex>
#include <fmt/format.h>
#include <thread>

using namespace std::chrono_literals;

namespace vk::mexc::futures {
struct WSStreamManager::P {
	std::unique_ptr<WSClient> wsClient;
	int timeout{5};
	mutable std::recursive_mutex instrumentInfoLocker;
	mutable std::recursive_mutex candlestickLocker;
	std::map<std::string, EventTicker> tickers;
	std::map<std::string, std::map<CandleInterval, EventCandlestick> > candlesticks;
	onLogMessage logMessageCB;

	explicit P() : wsClient(std::make_unique<WSClient>()) {
		wsClient->setDataEventCallback([&](const Event &event) {
			if (event.channel.find("push.ticker") != std::string::npos) {
				std::lock_guard lk(instrumentInfoLocker);

				try {
					if (const auto it = tickers.find(event.symbol); it == tickers.end()) {
						EventTicker eventTicker;
						eventTicker.fromJson(event.data);
						tickers.insert_or_assign(eventTicker.symbol, eventTicker);
					} else {
						it->second.fromJson(event.data);
					}
				} catch (std::exception &e) {
					logMessageCB(LogSeverity::Error, fmt::format("{}: {}", MAKE_FILELINE, e.what()));
				}
			} else if (event.channel.find("push.kline") != std::string::npos) {
				std::lock_guard lk(candlestickLocker);

				try {
					EventCandlestick eventCandlestick;
					eventCandlestick.fromJson(event.data);

					auto it = candlesticks.find(event.symbol);

					if (it == candlesticks.end()) {
						candlesticks.insert({event.symbol, {}});
					}

					it = candlesticks.find(event.symbol);
					it->second.insert_or_assign(eventCandlestick.interval, eventCandlestick);
				} catch (std::exception &e) {
					logMessageCB(LogSeverity::Error, fmt::format("{}: {}", MAKE_FILELINE, e.what()));
				}
			}
		});
	}
};

WSStreamManager::WSStreamManager() : m_p(std::make_unique<P>()) {
}

WSStreamManager::~WSStreamManager() {
	m_p->wsClient.reset();
	m_p->timeout = 0;
}

void WSStreamManager::subscribeTickerStream(const std::string &pair) const {
	WSSubscription subscriptionRequest;
	subscriptionRequest.method = "sub.ticker";
	subscriptionRequest.parameters.symbol = pair;

	if (!m_p->wsClient->isSubscribed(subscriptionRequest.toJson())) {
		if (m_p->logMessageCB) {
			const auto msgString = fmt::format("subscribing: {}", subscriptionRequest.toJson().dump());
			m_p->logMessageCB(LogSeverity::Info, msgString);
		}

		m_p->wsClient->subscribe(subscriptionRequest.toJson());
	}

	m_p->wsClient->run();
}

void WSStreamManager::subscribeCandlestickStream(const std::string &pair, const CandleInterval interval) const {
	WSSubscription subscriptionRequest;
	subscriptionRequest.method = "sub.kline";
	subscriptionRequest.parameters.symbol = pair;
	subscriptionRequest.parameters.interval = magic_enum::enum_name(interval);

	if (!m_p->wsClient->isSubscribed(subscriptionRequest.toJson())) {
		if (m_p->logMessageCB) {
			const auto msgString = fmt::format("subscribing: {}", subscriptionRequest.toJson().dump());
			m_p->logMessageCB(LogSeverity::Info, msgString);
		}

		m_p->wsClient->subscribe(subscriptionRequest.toJson());
	}

	m_p->wsClient->run();
}

void WSStreamManager::setTimeout(const int seconds) const {
	m_p->timeout = seconds;
}

int WSStreamManager::timeout() const {
	return m_p->timeout;
}

void WSStreamManager::setLoggerCallback(const onLogMessage &onLogMessageCB) const {
	m_p->logMessageCB = onLogMessageCB;
	m_p->wsClient->setLoggerCallback(onLogMessageCB);
}

std::optional<EventTicker> WSStreamManager::readEventTicker(const std::string &pair) const {
	int numTries = 0;
	const int maxNumTries = static_cast<int>(m_p->timeout / 0.01);

	while (numTries <= maxNumTries) {
		if (m_p->timeout == 0) {
			/// No need to wait when destroying object
			break;
		}

		m_p->instrumentInfoLocker.lock();

		if (const auto it = m_p->tickers.find(pair); it != m_p->tickers.end()) {
			auto retVal = it->second;

			m_p->instrumentInfoLocker.unlock();
			return retVal;
		}
		m_p->instrumentInfoLocker.unlock();
		numTries++;
		std::this_thread::sleep_for(3ms);
	}

	return {};
}

std::optional<EventCandlestick>
WSStreamManager::readEventCandlestick(const std::string &pair, const CandleInterval interval) const {
	int numTries = 0;
	const int maxNumTries = static_cast<int>(m_p->timeout / 0.01);

	while (numTries <= maxNumTries) {
		if (m_p->timeout == 0) {
			/// No need to wait when destroying object
			break;
		}

		m_p->candlestickLocker.lock();

		if (const auto it = m_p->candlesticks.find(pair); it != m_p->candlesticks.end()) {
			if (const auto itCandle = it->second.find(interval); itCandle != it->second.end()) {
				auto retVal = itCandle->second;
				m_p->candlestickLocker.unlock();
				return retVal;
			}
		}
		m_p->candlestickLocker.unlock();
		numTries++;
		std::this_thread::sleep_for(3ms);
	}

	return {};
}
}
