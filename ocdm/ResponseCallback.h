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

#include "IExchange.h"
#include <ocdm/open_cdm.h>

namespace WPEFramework {
namespace CENCDecryptor {
    namespace OCDM {

        class ResponseCallback : public IExchange::ICallback {
        public:
            ResponseCallback() = delete;
            ResponseCallback(const ResponseCallback&) = delete;
            ResponseCallback& operator=(const ResponseCallback&) = delete;

            ResponseCallback(OpenCDMSession*&, Core::CriticalSection& sessionLock);
            ~ResponseCallback() override{};

            void Response(Core::ProxyType<Web::Request>,
                Core::ProxyType<Web::Response>) override;

        private:
            OpenCDMSession*& _session;
            Core::CriticalSection& _sessionLock;
        };
    }
}
}
