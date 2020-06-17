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

#include "Decryptor.h"

#include <gst/gstbuffer.h>
#include <gst/gstevent.h>
#include <thread>

#include "open_cdm_adapter.h"

namespace WPEFramework {
namespace CENCDecryptor {
    namespace OCDM {

        Decryptor::Decryptor()
            : _system(nullptr)
            , _session(nullptr)
            , _callbacks({ process_challenge_callback,
                  key_update_callback,
                  error_message_callback,
                  keys_updated_callback })
            , _exchanger(nullptr)
            , _keyReceived(false, true)
            , _sessionLock()
        {
        }

        gboolean Decryptor::Initialize(std::unique_ptr<CENCDecryptor::IExchange> exchange,
            const std::string& keysystem,
            const std::string& origin,
            BufferView& initData)
        {
            _exchanger = std::move(exchange);
            return SetupOCDM(keysystem, origin, initData);
        }

        bool Decryptor::SetupOCDM(const std::string& keysystem,
            const std::string& origin,
            BufferView& initData)
        {
            if (_system == nullptr) {
                auto domainName = GetDomainName(keysystem.c_str());
                _system = opencdm_create_system(domainName.c_str());
                if (_system != nullptr) {

                    _sessionLock.Lock();

                    OpenCDMError ocdmResult = opencdm_construct_session(_system,
                        LicenseType::Temporary,
                        "cenc",
                        initData.Raw(),
                        static_cast<uint16_t>(initData.Size()),
                        nullptr,
                        0,
                        &_callbacks,
                        this,
                        &_session);

                    _sessionLock.Unlock();

                    if (ocdmResult != OpenCDMError::ERROR_NONE) {
                        TRACE_L1("Failed to construct session with error: <%d>", ocdmResult);
                        return false;
                    }

                } else {
                    TRACE_L1("Cannot construct opencdm_system for <%s> keysystem", keysystem.c_str());
                    return false;
                }
            }
            return true;
        }

        // TODO: Should this be provided by the ocdm?
        std::string Decryptor::GetDomainName(const std::string& guid)
        {
            if (guid == "edef8ba9-79d6-4ace-a3c8-27dcd51d21ed")
                return "com.widevine.alpha";
            else if (guid == "9a04f079-9840-4286-ab92-e65be0885f95")
                return "com.microsoft.playready";
            else if (guid == "1077efec-c0b2-4d02-ace3-3c1e52e2fb4b")
                return "org.w3.clearkey";
            else 
                return "";
        }

        GstFlowReturn Decryptor::Decrypt(std::shared_ptr<EncryptedBuffer> buffer)
        {
            if (buffer->IsClear()) {
                return GST_FLOW_OK;
            }

            if (buffer->IsValid()) {
                BufferView keyIdView(buffer->KeyId(), GST_MAP_READ);
                uint32_t waitResult = WaitForKeyId(keyIdView, Core::infinite);
                if (waitResult == Core::ERROR_NONE) {

                    _sessionLock.Lock();

                    OpenCDMError result = opencdm_gstreamer_session_decrypt(_session,
                        buffer->Buffer(),
                        buffer->SubSample(),
                        buffer->SubSampleCount(),
                        buffer->IV(),
                        buffer->KeyId(),
                        0);

                    _sessionLock.Unlock();

                    buffer->StripProtection();

                    return result != OpenCDMError::ERROR_NONE ? GST_FLOW_NOT_SUPPORTED : GST_FLOW_OK;
                } else {
                    TRACE_L1("Waiting for key failed with: <%d>", waitResult);
                    buffer->StripProtection();
                    return GST_FLOW_NOT_SUPPORTED;
                }
            } else {
                TRACE_L1("Invalid decryption metadata.");
                buffer->StripProtection();
                return GST_FLOW_NOT_SUPPORTED;
            }
        }

        uint32_t Decryptor::WaitForKeyId(BufferView& keyId, uint32_t timeout)
        {
            _sessionLock.Lock();

            KeyStatus keyStatus = opencdm_session_status(_session, keyId.Raw(), keyId.Size());

            _sessionLock.Unlock();

            uint32_t result = Core::ERROR_NONE;

            if (keyStatus != KeyStatus::Usable) {
                result = _keyReceived.Lock(Core::infinite);
            }
            return result;
        }

        Core::ProxyType<Web::Request> Decryptor::PrepareRequest(const string& challenge, const std::string& rawUrl)
        {
            size_t index = challenge.find(":Type:");
            size_t offset = 0;

            if (index != std::string::npos)
                offset = index + strlen(":Type:");

            auto request(Core::ProxyType<Web::Request>::Create());
            auto requestBody(Core::ProxyType<Web::TextBody>::Create());
            std::string reqBodyStr(challenge.substr(offset));
            requestBody->assign(reqBodyStr);
            
            Core::URL url(rawUrl);
            request->Host = url.Host().Value();
            request->Path = '/' + url.Path().Value();
            request->Verb = Web::Request::HTTP_POST;
            request->Connection = Web::Request::CONNECTION_CLOSE;
            request->Body<Web::TextBody>(requestBody);
            request->ContentType = Web::MIMETypes::MIME_TEXT_XML;
            request->ContentLength = reqBodyStr.length();

            return request;
        }

        Decryptor::~Decryptor()
        {
            if (_session != nullptr) {
                
                _sessionLock.Lock();

                opencdm_destruct_session(_session);

                _sessionLock.Unlock();
            }

            if (_system != nullptr) {
                opencdm_destruct_system(_system);
            }
        }

        void Decryptor::ProcessChallengeCallback(OpenCDMSession* session,
            const string& url,
            const string& challenge)
        {
            auto callback(Core::ProxyType<IExchange::ICallback>(Core::ProxyType<ResponseCallback>::Create(_session, _sessionLock)));
            auto request = PrepareRequest(challenge, url);
            _exchanger->Submit(request, callback, Core::infinite);
        }

        void Decryptor::KeyUpdateCallback(OpenCDMSession* session,
            void* userData,
            const uint8_t keyId[],
            const uint8_t length)
        {
        }

        void Decryptor::ErrorMessageCallback()
        {
            TRACE_L1("Error message callback not implemented in ocdm decryptor");
        }

        void Decryptor::KeysUpdatedCallback()
        {
            _keyReceived.SetEvent();
        }
    }

    std::unique_ptr<IGstDecryptor> IGstDecryptor::Create()
    {
        return std::unique_ptr<IGstDecryptor>(new OCDM::Decryptor());
    }
}
}
