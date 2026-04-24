/**
MEXC Common Stuff

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@stonky.cz>, Stonky s.r.o.
*/

#ifndef INCLUDE_STONKY_MEXC_API_H
#define INCLUDE_STONKY_MEXC_API_H

#include "stonky/mexc/mexc_models.h"
#include "stonky/mexc/mexc_enums.h"

namespace stonky::mexc {
enum class CandleInterval;

class MEXC {
public:
    static int64_t numberOfMsForCandleInterval(CandleInterval candleInterval);

    /// Convert CandleInterval to spot API format (e.g., "1m", "5m", "1h")
    static std::string candleIntervalToSpotString(CandleInterval candleInterval);
};
}
#endif // INCLUDE_STONKY_MEXC_API_H
