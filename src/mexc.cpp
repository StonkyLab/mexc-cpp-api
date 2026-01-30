/**
MEXC Common Stuff

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#include "vk/mexc/mexc.h"

namespace vk::mexc {
int64_t MEXC::numberOfMsForCandleInterval(const CandleInterval candleInterval) {
    switch (candleInterval) {
    case CandleInterval::_1m:
        return 60000;
    case CandleInterval::_5m:
        return 60000 * 5;
    case CandleInterval::_15m:
        return 60000 * 15;
    case CandleInterval::_30m:
        return 60000 * 30;
    case CandleInterval::_60m:
        return 60000 * 60;
    case CandleInterval::_4h:
        return 60000 * 240;
    case CandleInterval::_8h:
        return 60000 * 480;
    case CandleInterval::_1d:
        return 86400000;
    case CandleInterval::_1W:
        return 86400000 * 7;
    case CandleInterval::_1M:
        return static_cast<int64_t>(86400000) * 30;
    default:
        return 0;
    }
}

std::string MEXC::candleIntervalToSpotString(const CandleInterval candleInterval) {
    switch (candleInterval) {
    case CandleInterval::_1m:
        return "1m";
    case CandleInterval::_5m:
        return "5m";
    case CandleInterval::_15m:
        return "15m";
    case CandleInterval::_30m:
        return "30m";
    case CandleInterval::_60m:
        return "1h";
    case CandleInterval::_4h:
        return "4h";
    case CandleInterval::_8h:
        return "8h";
    case CandleInterval::_1d:
        return "1d";
    case CandleInterval::_1W:
        return "1W";
    case CandleInterval::_1M:
        return "1M";
    default:
        return "";
    }
}
}
