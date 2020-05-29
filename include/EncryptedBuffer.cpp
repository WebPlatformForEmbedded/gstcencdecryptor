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

#include "EncryptedBuffer.hpp"
#include <stdio.h>
namespace WPEFramework {
namespace CENCDecryptor {
    EncryptedBuffer::EncryptedBuffer(GstBuffer* buffer)
        : _buffer(nullptr)
        , _subSample(nullptr)
        , _subSampleSize(0)
        , _initialVec(nullptr)
        , _keyId(nullptr)
        , _isClear(false)
        , _protectionMeta(nullptr)
    {
        ExtractDecryptMeta(buffer);
    }

    void EncryptedBuffer::ExtractDecryptMeta(GstBuffer* buffer)
    {
        _buffer = buffer;
        _protectionMeta = reinterpret_cast<GstProtectionMeta*>(gst_buffer_get_protection_meta(_buffer));

        if (!_protectionMeta) {
            _isClear = true;
        } else {
            gst_structure_remove_field(_protectionMeta->info, "stream-encryption-events");

            const GValue* value;
            value = gst_structure_get_value(_protectionMeta->info, "kid");

            _keyId = gst_value_get_buffer(value);

            unsigned ivSize;
            gst_structure_get_uint(_protectionMeta->info, "iv_size", &ivSize);

            gboolean encrypted;
            gst_structure_get_boolean(_protectionMeta->info, "encrypted", &encrypted);

            if (!ivSize || !encrypted) {
                StripProtection();
                _isClear = true;
            } else {
                gst_structure_get_uint(_protectionMeta->info, "subsample_count", &_subSampleSize);
                if (_subSampleSize) {
                    const GValue* value2 = gst_structure_get_value(_protectionMeta->info, "subsamples");
                    _subSample = gst_value_get_buffer(value2);
                }

                const GValue* value3;
                value3 = gst_structure_get_value(_protectionMeta->info, "iv");
                _initialVec = gst_value_get_buffer(value3);
            }
        }
    }

    void EncryptedBuffer::StripProtection()
    {
        if (_protectionMeta != nullptr) {
            gst_buffer_remove_meta(_buffer, reinterpret_cast<GstMeta*>(_protectionMeta));
        }
        // gst_unref
        // gst_buffer_unref(_subSample);
        // gst_buffer_unref(_initialVec);
        // gst_buffer_unref(_keyId);
    }

    GstBuffer* EncryptedBuffer::Buffer()
    {
        return _buffer;
    }

    GstBuffer* EncryptedBuffer::SubSample()
    {
        return _subSample;
    }

    size_t EncryptedBuffer::SubSampleCount()
    {
        return _subSampleSize;
    }

    GstBuffer* EncryptedBuffer::IV()
    {
        return _initialVec;
    }
    
    GstBuffer* EncryptedBuffer::KeyId()
    {
        return _keyId;
    }

    bool EncryptedBuffer::IsClear()
    {
        return _isClear;
    }

    bool EncryptedBuffer::IsValid()
    {
        return _buffer != nullptr
            && _subSample != nullptr
            && _subSampleSize != 0
            && _initialVec != nullptr
            && _keyId != nullptr;
    }
}
}
