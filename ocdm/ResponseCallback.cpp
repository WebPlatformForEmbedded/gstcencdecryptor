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

#include "ResponseCallback.h"

namespace WPEFramework {
namespace CENCDecryptor {
    namespace OCDM {

        ResponseCallback::ResponseCallback(OpenCDMSession*& session, Core::CriticalSection& sessionLock)
            : _session(session)
            , _sessionLock(sessionLock)
        {
        }

        void ResponseCallback::Response(Core::ProxyType<Web::Request> req,
            Core::ProxyType<Web::Response> res)
        {
            ASSERT(_session != nullptr)
            if (req.IsValid() && res->HasBody()) {
                Core::ProxyType<Web::TextBody> body = res->Body<Web::TextBody>();
                auto keyResponse = reinterpret_cast<const uint8_t*>(body->c_str());

                _sessionLock.Lock();

                opencdm_session_update(_session, keyResponse, body->length());

                _sessionLock.Unlock();
            } else {
                TRACE_L1("Challenge response without a body");
            }
        }
    }
}
}
