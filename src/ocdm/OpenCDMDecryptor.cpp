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

#include "OpenCDMDecryptor.h"
#include "Constants.h"
#include "Tracing.h"

#include <gst/gstbuffer.h>
#include <gst/gstevent.h>
#include <thread>

namespace CENCDecryptor {
    namespace OCDM {

        using namespace WPEFramework;

        Decryptor::Decryptor()
            : _system(nullptr)
            , _session(nullptr)
            , _keyReceived(false, true)
            , _sessionMutex()
        {
            auto processChallengeCallback = [](OpenCDMSession* session VARIABLE_IS_NOT_USED, void* userData, const char url[], const uint8_t challenge[], const uint16_t challengeLength) {
                Decryptor* decryptor = reinterpret_cast<Decryptor*>(userData);
                string challengeData(reinterpret_cast<const char*>(challenge), challengeLength);
                decryptor->_licenseRequest = std::move(decryptor->CreateLicenseRequest(challengeData, url));
                if(decryptor->_licenseRequest != nullptr) {
                    decryptor->_licenseRequest->Submit();
                } else {
                    Trace::error("Could not create a license acquisition request");
                }
            };
            auto keyUpdateCallback = [](OpenCDMSession* session VARIABLE_IS_NOT_USED, void* userData VARIABLE_IS_NOT_USED, const uint8_t keyId[] VARIABLE_IS_NOT_USED, const uint8_t length VARIABLE_IS_NOT_USED) {};
            auto errorMessageCallback = [](OpenCDMSession* session VARIABLE_IS_NOT_USED, void* userData VARIABLE_IS_NOT_USED, const char message[] VARIABLE_IS_NOT_USED) {};
            auto keysUpdatedCallback = [](const OpenCDMSession* session VARIABLE_IS_NOT_USED, void* userData) {
                Decryptor* decryptor = reinterpret_cast<Decryptor*>(userData);
                decryptor->_keyReceived.SetEvent();
            };

            _callbacks = { processChallengeCallback, keyUpdateCallback, errorMessageCallback, keysUpdatedCallback };
        }

        IGstDecryptor::Status Decryptor::Initialize(const std::string& keysystem,
            const std::string& origin,
            const std::string& initDataType,
            BufferView& initData)
        {
            if (!opencdm_is_type_supported(GetDomainName(keysystem).c_str(), "")) {

                return InitializeOpenCDM(keysystem, origin, initDataType, initData) ? IGstDecryptor::Status::SUCCESS : IGstDecryptor::Status::ERROR_INITIALIZE_FAILURE;

            } else {
                return IGstDecryptor::Status::ERROR_KEYSYSTEM_NOT_SUPPORTED;
            }
        }

        bool Decryptor::InitializeOpenCDM(const std::string& keysystem,
            const std::string& origin VARIABLE_IS_NOT_USED,
            const std::string& initDataType,
            BufferView& initData)
        {
            auto domainName = GetDomainName(keysystem.c_str());
            _system = opencdm_create_system(domainName.c_str());
            std::lock_guard<std::mutex> lk(_sessionMutex);

            OpenCDMError ocdmResult = opencdm_construct_session(_system,
                LicenseType::Temporary,
                initDataType.c_str(),
                initData.Raw(),
                static_cast<uint16_t>(initData.Size()),
                nullptr,
                0,
                &_callbacks,
                this,
                &_session);

            if (ocdmResult != OpenCDMError::ERROR_NONE) {
                Trace::error("Failed to construct session with error: ", ocdmResult);
                return false;
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

        IGstDecryptor::Status Decryptor::Decrypt(std::shared_ptr<EncryptedBuffer> buffer)
        {
            if (buffer->IsClear()) {
                return IGstDecryptor::Status::SUCCESS;
            }

            BufferView keyIdView(buffer->KeyId(), GST_MAP_READ);
            uint32_t waitResult = WaitForKeyId(keyIdView, Constants::WaitForKeyTimeInMs);
            if (waitResult == Core::ERROR_NONE) {

                std::lock_guard<std::mutex> lk(_sessionMutex);

                OpenCDMError result = opencdm_gstreamer_session_decrypt(_session,
                    buffer->Buffer(),
                    buffer->SubSample(),
                    buffer->SubSampleCount(),
                    buffer->IV(),
                    buffer->KeyId(),
                    0);

                buffer->StripProtection();
                return result != OpenCDMError::ERROR_NONE ? IGstDecryptor::Status::ERROR_GENERAL : IGstDecryptor::Status::SUCCESS;
            } else {
                Trace::error("Error while waiting for a decryption key, code: ", waitResult);
                buffer->StripProtection();
                return IGstDecryptor::Status::ERROR_TIMED_OUT_WAITING_FOR_KEY;
            }
        }

        uint32_t Decryptor::WaitForKeyId(BufferView& keyId, uint32_t timeout)
        {
            std::unique_lock<std::mutex> lk(_sessionMutex);
            KeyStatus keyStatus = opencdm_session_status(_session, keyId.Raw(), keyId.Size());
            lk.unlock();

            uint32_t result = Core::ERROR_NONE;

            while (keyStatus != KeyStatus::Usable && result == Core::ERROR_NONE) {
                Trace::log("Waiting for key: ", Trace::uint8_to_string(keyId.Raw(), keyId.Size()));
                result = _keyReceived.Lock(timeout);
                _keyReceived.ResetEvent();
                std::lock_guard<std::mutex> lk(_sessionMutex);
                keyStatus = opencdm_session_status(_session, keyId.Raw(), keyId.Size());
            }
            return result;
        }

        std::unique_ptr<LicenseRequest> Decryptor::CreateLicenseRequest(const string& challenge, const std::string& rawUrl)
        {
            size_t index = challenge.find(":Type:");
            size_t offset = (index != std::string::npos) ? index + strlen(":Type:") : 0;

            std::string requestBody(challenge.substr(offset));
            std::vector<uint8_t> bodyBytes(requestBody.begin(), requestBody.end());

            const char* overrideUrl = std::getenv("OVERRIDE_LA_URL");
            std::string url(overrideUrl == nullptr ? rawUrl : overrideUrl);

            std::vector<std::string> headers = { "Content-Type: text/xml", "Connection: CLOSE" };
            auto responseCallback = [&](uint32_t code, const std::string& response) {
                this->ProcessLicenseResponse(code, response);
            };

            if (url.empty()) {
                Trace::error("No license server URL provided, aborting request");
                return nullptr;
            } else {
                return std::unique_ptr<LicenseRequest>(new LicenseRequest(url, bodyBytes, headers, responseCallback));
            }
        }

        void Decryptor::ProcessLicenseResponse(uint32_t code, const std::string& response)
        {
            if (code == 200) {
                // Some keysystems (usually WV) add additional information about the keyId's
                // in the beggining of the body. Let's skip past that bit if it's detected:
                std::string drmHeadAnchor = "\r\n\r\n";
                size_t wvDrmHeadPos = response.find(drmHeadAnchor);
                auto newIndex = (wvDrmHeadPos != std::string::npos) ? (wvDrmHeadPos + drmHeadAnchor.length()) : 0;

                std::vector<uint8_t> bytes(response.begin(), response.end());

                std::lock_guard<std::mutex> lk(_sessionMutex);
                VARIABLE_IS_NOT_USED OpenCDMError result = opencdm_session_update(_session, bytes.data() + newIndex, bytes.size() - newIndex);
            } else {
                Trace::error("Error response from LA server, code: ", code);
                Trace::log("Server response: ", response);
            }
        }

        Decryptor::~Decryptor()
        {
            std::unique_lock<std::mutex> lk(_sessionMutex);
            if (_session != nullptr) {
                opencdm_destruct_session(_session);
            }
            lk.unlock();

            if (_system != nullptr) {
                opencdm_destruct_system(_system);
            }

            Core::Singleton::Dispose();
        }
    }

    std::unique_ptr<IGstDecryptor> IGstDecryptor::Create()
    {
        return std::unique_ptr<IGstDecryptor>(new OCDM::Decryptor());
    }
}
