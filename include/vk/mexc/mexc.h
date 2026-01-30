/**
MEXC Common Stuff

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#ifndef INCLUDE_VK_MEXC_API_H
#define INCLUDE_VK_MEXC_API_H

#include "vk/mexc/mexc_models.h"
#include "vk/mexc/mexc_enums.h"

namespace vk::mexc {
enum class CandleInterval;

class MEXC {
public:
    static int64_t numberOfMsForCandleInterval(CandleInterval candleInterval);

    /// Convert CandleInterval to spot API format (e.g., "1m", "5m", "1h")
    static std::string candleIntervalToSpotString(CandleInterval candleInterval);
};
}
#endif // INCLUDE_VK_MEXC_API_H
