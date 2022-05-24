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

#include <gst/gstbuffer.h>
#include <gst/gstevent.h>
#include <memory>

namespace CENCDecryptor {
    /**
     * @brief Integration layer between a gstreamer element and CDMi implementation.
     * Provides the means to decrypt content and handle license acquisition requests.
     */
    class IGstDecryptor {
    public:
        enum class Status {
            SUCCESS,
            ERROR_KEYSYSTEM_NOT_SUPPORTED,
            ERROR_INITIALIZE_FAILURE,
            ERROR_TIMED_OUT_WAITING_FOR_KEY,
            ERROR_GENERAL
        };

        static std::unique_ptr<IGstDecryptor> Create(bool disposeInstance);

        /**
         * @brief Initializes MediaSystem and MediaSession objects.
         * 
         * This function should trigger the process of a license acquisition,
         * which will allow for the subsequent Decrypt calls to succeed.
         * The license acquisition does not have to finish before this call ends.
         * 
         * Does not need to be thread safe, the calling context makes sure that
         * only a single thread at a time calls this.
         * 
         * Will be called multiple times if the function returns ERROR_KEYSYSTEM_NOT_SUPPORTED
         * and the asset has multi-DRM support.
         * 
         * @param factory IExchange used for license acquisition.
         * @param keysystem Keysytem uuid using which content was encrypted. 
         * @param origin TODO
         * @param initDataType Type ("webm" / "cenc") of @param initData. 
         * @param initData Content metadata contatining E.g. a PSSH box.
         * @return SUCCESS, ERROR_KEYSYSTEM_NOT_SUPPORTED, ERROR_INITIALIZE_FAILURE.
         */
        virtual IGstDecryptor::Status Initialize(const std::string& keysystem,
            const std::string& origin,
            const std::string& initDataType,
            BufferView& initData)
            = 0;

        /**
         * @brief Decrypts the specified EncryptedBuffer in-place.
         * 
         * @param buffer EncryptedBuffer initialized with a GstBuffer containing
         * the encryption metadata. 
         * 
         * This function might be called before a key for decryption is available.
         * In this case, it should wait until the key becomes available for decryption.
         * The time after which a function should raise ERROR_TIMED_OUT_WAITING_FOR_KEY
         * is decided by the implementation. 
         * 
         * Both ERROR_TIMED_OUT_WAITING_FOR_KEY and ERROR_GENERAL, cause the player
         * to abort playback and exit.
         * 
         * @return SUCCESS, ERROR_TIMED_OUT_WAITING_FOR_KEY, ERROR_GENERAL
         */
        virtual IGstDecryptor::Status Decrypt(std::shared_ptr<EncryptedBuffer> buffer) = 0;

        virtual ~IGstDecryptor(){};
    };
}
