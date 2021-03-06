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

#include "EncryptedBuffer.h"
#include "GstBufferView.h"
#include "IExchange.h"

#include <gst/gstbuffer.h>
#include <gst/gstevent.h>
#include <memory>

namespace WPEFramework {
namespace CENCDecryptor {
    /**
     * @brief Integration layer between a gstreamer element and CDMi implementation.
     * Provides the means to decrypt content and redirect license acquisition requests
     * to the IExhchange.
     */
    class IGstDecryptor {
    public:
        static std::unique_ptr<IGstDecryptor> Create();

        /**
         * @brief Provides source content / stream metadata to the CDMi.
         * 
         * @param factory IExchange used for license acquisition.
         * @param keysystem Keysytem uuid using which content was encrypted. 
         * @param origin TODO
         * @param initData Content metadata contatining E.g. a PSSH box.
         * @return gboolean Whether intialization of CDMi has succeeded.
         */
        virtual gboolean Initialize(std::unique_ptr<CENCDecryptor::IExchange> exchange,
            const std::string& keysystem,
            const std::string& origin,
            BufferView& initData) = 0;

        /**
         * @brief Decrypts the specified EncryptedBuffer in-place.
         * 
         * @param buffer EncryptedBuffer initialized with a GstBuffer containing
         * encryption metadata.
         * @return GstFlowReturn GST_FLOW_OK / GST_FLOW_NOT_SUPPORTED
         */
        virtual GstFlowReturn Decrypt(std::shared_ptr<EncryptedBuffer> buffer) = 0;

        virtual ~IGstDecryptor(){};
    };
}
}
