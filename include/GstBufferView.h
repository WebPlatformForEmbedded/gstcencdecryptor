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

namespace WPEFramework {
namespace CENCDecryptor {
    /**
     * @brief RAII wrapper for the GstBuffer read/write operations.
     */
    class BufferView {
    public:
        BufferView() = delete;
        BufferView(const BufferView&) = delete;
        BufferView& operator=(const BufferView&) = delete;

        explicit BufferView(GstBuffer* buffer, GstMapFlags flags)
            : _buffer(buffer)
        {
            gst_buffer_map(_buffer, &_dataView, flags);
        }

        /**
         * @brief 
         * 
         * @return gsize The size of the mapped buffer.
         */
        gsize Size() { return _dataView.size; }

        /**
         * @brief TODO
         * 
         * @return guint8* Raw data buffer.
         */
        guint8* Raw() { return _dataView.data; }

        ~BufferView()
        {
            gst_buffer_unmap(_buffer, &_dataView);
        }

    private:
        GstBuffer* _buffer;
        GstMapInfo _dataView;
    };
}
}
