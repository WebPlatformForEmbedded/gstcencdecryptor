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

#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>

#include <memory>

G_BEGIN_DECLS

#define GST_TYPE_CENCDECRYPT (gst_cencdecrypt_get_type())
#define GST_CENCDECRYPT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_CENCDECRYPT, GstCencDecrypt))
#define GST_CENCDECRYPT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_CENCDECRYPT, GstCencDecryptClass))
#define GST_IS_CENCDECRYPT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_CENCDECRYPT))
#define GST_IS_GST_TYPE_CENCDECRYPT_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_CENCDECRYPT))

struct GstCencDecryptImpl;

struct GstCencDecrypt {
    GstBaseTransform base_cencdecrypt;
    std::unique_ptr<GstCencDecryptImpl> _impl;
    gboolean _dispose_instance;
};

struct GstCencDecryptClass {
    GstBaseTransformClass base_cencdecrypt_class;
};

GType gst_cencdecrypt_get_type(void);

G_END_DECLS
