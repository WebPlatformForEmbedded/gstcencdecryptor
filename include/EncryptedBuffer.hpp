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

#include <gst/gstbuffer.h>
#include <gst/gstprotection.h>

namespace WPEFramework {
namespace CENCDecryptor {
    class EncryptedBuffer {
    public:
        EncryptedBuffer() = delete;
        EncryptedBuffer(GstBuffer*);

        EncryptedBuffer(const EncryptedBuffer&) = default;
        EncryptedBuffer& operator=(const EncryptedBuffer&) = default;

        GstBuffer* Buffer();
        GstBuffer* SubSample();
        size_t SubSampleCount();
        GstBuffer* IV();
        GstBuffer* KeyId();

        bool IsClear();
        bool IsValid(); // TODO: name?

        void StripProtection();

    protected:
        virtual void ExtractDecryptMeta(GstBuffer*);

        GstBuffer* _buffer;
        GstBuffer* _subSample;
        size_t _subSampleSize;
        GstBuffer* _initialVec;
        GstBuffer* _keyId;

        bool _isClear;
        GstProtectionMeta* _protectionMeta;
    };
}
}
