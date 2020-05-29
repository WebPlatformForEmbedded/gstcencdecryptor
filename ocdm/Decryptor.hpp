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

#include "EncryptedBuffer.hpp"
#include "GstBufferView.hpp"
#include "IExchangeFactory.hpp"
#include "IGstDecryptor.hpp"
#include "ResponseCallback.hpp"
#include <ocdm/open_cdm.h>

namespace WPEFramework {
namespace CENCDecryptor {
    class OCDMDecryptor : public IGstDecryptor {
    public:
        OCDMDecryptor();
        OCDMDecryptor(const OCDMDecryptor&) = delete;
        OCDMDecryptor& operator=(const OCDMDecryptor&) = delete;

        ~OCDMDecryptor() override;

        gboolean Initialize(std::unique_ptr<IExchangeFactory>,
            const std::string& keysystem,
            const std::string& origin,
            BufferView& initData) override;

        GstFlowReturn Decrypt(std::shared_ptr<EncryptedBuffer>) override;

    private:
        bool SetupOCDM(const std::string& keysystem, const std::string& origin, BufferView& initData);

        std::string GetDomainName(const std::string& guid);

        uint32_t WaitForKeyId(BufferView& keyId, uint32_t timeout);

        Core::ProxyType<Web::Request> PrepareChallenge(const string& challenge);

        OpenCDMSystem* _system;
        OpenCDMSession* _session;
        std::unique_ptr<IExchange> _exchanger;
        std::unique_ptr<IExchangeFactory> _factory;
        OpenCDMSessionCallbacks _callbacks;

        Core::Event _keyReceived;
        mutable Core::CriticalSection _sessionLock;

    private:
        static void process_challenge_callback(OpenCDMSession* session,
            void* userData,
            const char url[],
            const uint8_t challenge[],
            const uint16_t challengeLength)
        {
            OCDMDecryptor* comm = reinterpret_cast<OCDMDecryptor*>(userData);
            string challengeData(reinterpret_cast<const char*>(challenge), challengeLength);
            comm->ProcessChallengeCallback(session, std::string(url), challengeData);
        }

        static void key_update_callback(OpenCDMSession* session,
            void* userData,
            const uint8_t keyId[],
            const uint8_t length)
        {
            OCDMDecryptor* comm = reinterpret_cast<OCDMDecryptor*>(userData);
            comm->KeyUpdateCallback(session, userData, keyId, length);
        }

        static void error_message_callback(OpenCDMSession* session,
            void* userData,
            const char message[])
        {
            OCDMDecryptor* comm = reinterpret_cast<OCDMDecryptor*>(userData);
            comm->ErrorMessageCallback();
        }

        static void keys_updated_callback(const OpenCDMSession* session, void* userData)
        {
            OCDMDecryptor* comm = reinterpret_cast<OCDMDecryptor*>(userData);
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
