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

#include "Module.h"

#include "EncryptedBuffer.h"
#include "GstBufferView.h"
#include "IGstDecryptor.h"
#include "LicenseRequest.h"

#include <ocdm/open_cdm.h>
#include <ocdm/open_cdm_adapter.h>

#include <memory>
#include <mutex>

namespace WPEFramework {
namespace CENCDecryptor {
    namespace OCDM {

        class Decryptor : public IGstDecryptor {
        public:
            Decryptor();
            Decryptor(const Decryptor&) = delete;
            Decryptor& operator=(const Decryptor&) = delete;

            ~Decryptor() override;

            IGstDecryptor::Status Initialize(const std::string& keysystem,
                const std::string& origin,
                BufferView& initData) override;

            GstFlowReturn Decrypt(std::shared_ptr<EncryptedBuffer>) override;

        private:
            bool SetupOCDM(const std::string& keysystem, const std::string& origin, BufferView& initData);

            std::string GetDomainName(const std::string& guid);

            uint32_t WaitForKeyId(BufferView& keyId, uint32_t timeout);

            std::unique_ptr<LicenseRequest> CreateLicenseRequest(const string& challenge, const std::string& url);
            void ProcessResponse(uint32_t code, const std::string& response);

            OpenCDMSystem* _system;
            OpenCDMSession* _session;
            OpenCDMSessionCallbacks _callbacks;

            Core::Event _keyReceived;
            std::unique_ptr<LicenseRequest> _licenseRequest;
            std::mutex _sessionMutex;

        private:
            static void process_challenge_callback(OpenCDMSession* session,
                void* userData,
                const char url[],
                const uint8_t challenge[],
                const uint16_t challengeLength)
            {
                Decryptor* comm = reinterpret_cast<Decryptor*>(userData);
                string challengeData(reinterpret_cast<const char*>(challenge), challengeLength);
                comm->ProcessChallengeCallback(session, std::string(url), challengeData);
            }

            static void key_update_callback(OpenCDMSession* session,
                void* userData,
                const uint8_t keyId[],
                const uint8_t length)
            {
                Decryptor* comm = reinterpret_cast<Decryptor*>(userData);
                comm->KeyUpdateCallback(session, userData, keyId, length);
            }

            static void error_message_callback(OpenCDMSession* session,
                void* userData,
                const char message[])
            {
                Decryptor* comm = reinterpret_cast<Decryptor*>(userData);
                comm->ErrorMessageCallback();
            }

            static void keys_updated_callback(const OpenCDMSession* session, void* userData)
            {
                Decryptor* comm = reinterpret_cast<Decryptor*>(userData);
                comm->KeysUpdatedCallback();
            }

            void ProcessChallengeCallback(OpenCDMSession* session,
                const string& url,
                const string& challenge);
            void KeyUpdateCallback(OpenCDMSession* session,
                void* userData,
                const uint8_t keyId[],
                const uint8_t length);
            void ErrorMessageCallback();
            void KeysUpdatedCallback();
        };
    }
}
}
