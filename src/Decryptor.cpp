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
            , _keyReceived(false, true)
            , _sessionLock()
        {
        }

        IGstDecryptor::Status Decryptor::Initialize(const std::string& keysystem,
            const std::string& origin,
            BufferView& initData)
        {
            if(!opencdm_is_type_supported(GetDomainName(keysystem).c_str(), "")) {

                return SetupOCDM(keysystem, origin, initData) ? 
                    IGstDecryptor::Status::SUCCESS : IGstDecryptor::Status::ERROR_INITIALIZE_FAILURE;

            } else {
                return IGstDecryptor::Status::ERROR_KEYSYSTEM_NOT_SUPPORTED;
            }
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
                        fprintf(stderr, "Failed to construct session with error: <%d>", ocdmResult);
                        return false;
                    }

                } else {
                    fprintf(stderr, "Cannot construct opencdm_system for <%s> keysystem", keysystem.c_str());
                    return false;
                }
            }
            return true;
        }

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
                fprintf(stderr, "Waiting for key failed with: <%d>", waitResult);
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

        std::unique_ptr<LicenseRequest> Decryptor::CreateLicenseRequest(const string& challenge, const std::string& rawUrl)
        {
            // TODO: This ":Type:" string is an ugly corner case for the wv keysystem.
            // Take this bit of code out into a in/out filter object, that will
            // process in-place requests / responses for specific keysystems.
            // Same goes for the license response "\r\n\r\n" string.
            size_t index = challenge.find(":Type:");
            size_t offset = (index != std::string::npos) ? index + strlen(":Type:") : 0;

            std::string requestBody(challenge.substr(offset));
            std::vector<uint8_t> bodyBytes(requestBody.begin(), requestBody.end());  

            const char* overrideUrl = std::getenv("OVERRIDE_LA_URL");
            std::string url(overrideUrl == nullptr ? rawUrl : overrideUrl);

            std::vector<std::string> headers = {"Content-Type: text/xml", "Connection: CLOSE"};
            auto responseCallback = [&](uint32_t code, const std::string& response) {
                this->ProcessResponse(code, response);
            };
            return std::unique_ptr<LicenseRequest>(new LicenseRequest(url, bodyBytes, headers, responseCallback));
        }

        void Decryptor::ProcessResponse(uint32_t code, const std::string &response)
        {
            if (code == 200)
            {
                // Some keysystems (usually WV) add additional information about the keyId's
                // in the beggining of the body. Let's skip past that bit if it's detected:
                std::string drmHeadAnchor = "\r\n\r\n";
                size_t wvDrmHeadPos = response.find(drmHeadAnchor);
                auto newIndex = (wvDrmHeadPos != std::string::npos) ? (wvDrmHeadPos + drmHeadAnchor.length()) : 0;

                std::vector<uint8_t> bytes(response.begin(), response.end());
                
                _sessionLock.Lock();
                fprintf(stderr, "\n\n cdm session update \n\n");
                OpenCDMError result = opencdm_session_update(_session, bytes.data() + newIndex, bytes.size() - newIndex);
                fprintf(stderr, "\n\n cdm session update --- done\n\n");
                _sessionLock.Unlock();
            }
            else
            {
                fprintf(stderr, "Invalid license response code received: %d \n", code);
            }
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
            _licenseRequest = std::move(CreateLicenseRequest(challenge, url));
            _licenseRequest->Submit();
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
