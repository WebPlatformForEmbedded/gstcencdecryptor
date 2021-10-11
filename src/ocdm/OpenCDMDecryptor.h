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
                const std::string& initDataType,
                BufferView& initData) override;

            IGstDecryptor::Status Decrypt(std::shared_ptr<EncryptedBuffer>) override;

        private:
            bool InitializeOpenCDM(const std::string& keysystem,
                const std::string& origin,
                const std::string& initDataType,
                BufferView& initData);
            std::string GetDomainName(const std::string& guid);
            uint32_t WaitForKeyId(BufferView& keyId, uint32_t timeout);
            std::unique_ptr<LicenseRequest> CreateLicenseRequest(const string& challenge, const std::string& url);
            void ProcessLicenseResponse(uint32_t code, const std::string& response);

            OpenCDMSystem* _system;
            OpenCDMSession* _session;
            OpenCDMSessionCallbacks _callbacks;

            Core::Event _keyReceived;
            std::unique_ptr<LicenseRequest> _licenseRequest;
            std::mutex _sessionMutex;
        };
    }
}
}
