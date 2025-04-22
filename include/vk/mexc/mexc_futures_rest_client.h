/**
MEXC Futures REST Client

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/


#ifndef INCLUDE_VK_MEXC_FUTURES_REST_CLIENT_H
#define INCLUDE_VK_MEXC_FUTURES_REST_CLIENT_H

#include <string>
#include <memory>
#include "mexc_models.h"

namespace vk::mexc::futures {
class RESTClient {
    struct P;
    std::unique_ptr<P> m_p{};

public:
    RESTClient(const std::string &apiKey, const std::string &apiSecret);

    ~RESTClient();

    void setCredentials(const std::string &apiKey, const std::string &apiSecret) const;

    /**
     * Returns server time in ms
     * @return timestamp in ms
     * @see https://mexcdevelop.github.io/apidocs/contract_v1_en/#get-the-server-time
    */
    [[nodiscard]] std::int64_t getServerTime() const;

    //[[nodiscard]] std::vector<Contract> getContracts() const;
};
}

#endif //INCLUDE_VK_MEXC_FUTURES_REST_CLIENT_H
