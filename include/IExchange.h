/*
 * Copyright RDK Management
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation, version 2
 * of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#pragma once

#include <websocket/WebLink.h>
#include <websocket/WebTransfer.h>

namespace WPEFramework {
namespace CENCDecryptor{
/**
 * @brief Stream socket helper, which provides the means to submit & revoke 
 * http requests.
 */
class IExchange {
public:
    /**
     * @brief Callback used for handling responses of requests submitted through IExchange.
     */
    class ICallback {
    public:
        /**
         * @brief Called after a response (or timeout) arrives (occurs).
         * 
         * @param req Request submitted for execution.
         * @param res Response received from the server. 
         * If a request timed out, @param res is an invalid ProxyType.
         */
        virtual void Response(Core::ProxyType<Web::Request> req,
            Core::ProxyType<Web::Response> res) = 0;

        virtual ~ICallback(){};
    };

    static std::unique_ptr<IExchange> Create();

    /**
     * @brief Schedules the specified request for execution 
     * via the means of a Queue.
     * 
     * @param req Request to be scheduled.
     * @param next Callback fired on success and failure.
     * @param waitTime Time period [ms] within which, a response from the server has to arrive. 
     * Note: this does not include the time it takes to schedule a request.
     * @return uint32_t ERROR_UNAVAILABLE if queue is overfilled, ERROR_NONE otherwise.
     */
    virtual uint32_t Submit(Core::ProxyType<Web::Request> req,
        Core::ProxyType<IExchange::ICallback> next, 
        uint32_t waitTime) = 0;

    /**
     * @brief Revokes a previously scheduled request. Requests in the following states are revocable:
     *  - Scheduled
     *  - During execution
     *  - After execution and before response handler callback
     * 
     * @return uint32_t ERROR_NONE / ERROR_UNAVAILABLE
     */
    virtual uint32_t Revoke(Core::ProxyType<IExchange::ICallback>) = 0;

    virtual ~IExchange(){};
};
}
}
