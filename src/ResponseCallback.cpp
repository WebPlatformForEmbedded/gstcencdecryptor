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
#include <string>

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
            if (req.IsValid() && res->HasBody()) {
                Core::ProxyType<Web::TextBody> body = res->Body<Web::TextBody>();

                std::string drmHeadAnchor = "\r\n\r\n";
                size_t wvDrmHeadPos = body->find(drmHeadAnchor);

                // Some keysystems (usually WV) add additional information about the keyId's
                // in the beggining of the body. Let's skip past that bit if it's detected:
                auto newIndex = (wvDrmHeadPos != std::string::npos) ? (wvDrmHeadPos + drmHeadAnchor.length()) : 0;
                auto keyResponse = reinterpret_cast<const uint8_t*>(body->substr(newIndex).c_str());

                _sessionLock.Lock();
                OpenCDMError result = opencdm_session_update(_session, keyResponse, body->length() - newIndex);
                _sessionLock.Unlock();
                } else {
                    fprintf(stderr, "Challenge response without a body");
                }
            }
        }
}
}
