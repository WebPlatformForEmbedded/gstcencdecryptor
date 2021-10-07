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

#include "GstCencDecrypt.h"
#include "IGstDecryptor.h"

#include <gst/base/gstbasetransform.h>
#include <gst/gst.h>
#include <gst/gstprotection.h>

#include <core/Singleton.h>

#include <map>
#include <mutex>

using namespace WPEFramework::CENCDecryptor;

GST_DEBUG_CATEGORY_STATIC(gst_cencdecrypt_debug_category);
#define GST_CAT_DEFAULT gst_cencdecrypt_debug_category

G_DEFINE_TYPE_WITH_CODE(GstCencDecrypt, gst_cencdecrypt, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT(gst_cencdecrypt_debug_category, "cencdecrypt", 0,
        "debug category for cencdecrypt element"));

constexpr static auto clearContentTypes = { "video/mp4", "audio/mp4", "audio/mpeg", "video/x-h264", "video/x-h265", "video/x-vp9" };

static std::map<std::string, std::string> keySystems{ { "edef8ba9-79d6-4ace-a3c8-27dcd51d21ed", "com.widevine.alpha" },
    { "9a04f079-9840-4286-ab92-e65be0885f95", "com.microsoft.playready" }, {"1077efec-c0b2-4d02-ace3-3c1e52e2fb4b", "org.w3.clearkey"} };
constexpr static auto cencPrefix = "application/x-cenc";

static GstCaps* TransformCaps(GstBaseTransform* trans, GstPadDirection direction,
    GstCaps* caps, GstCaps* filter);
static gboolean SinkEvent(GstBaseTransform* trans, GstEvent* event);
static GstFlowReturn TransformIp(GstBaseTransform* trans, GstBuffer* buffer);

static void AddCapsForKeysystem(GstCaps*& caps, const std::string& keysystem)
{
    for (auto& type : clearContentTypes) {
        gst_caps_append_structure(caps,
            gst_structure_new(cencPrefix,
                "original-media-type", G_TYPE_STRING, type,
                "protection-system", G_TYPE_STRING, keysystem.c_str(), NULL));
    }
}
static GstCaps* SinkCaps(GstCencDecryptClass* klass)
{
    GstCaps* cencCaps = gst_caps_new_empty();
    for (auto& system : keySystems) {
        AddCapsForKeysystem(cencCaps, system.first);
    }
    return cencCaps;
}

static GstCaps* SrcCaps()
{
    GstCaps* caps = gst_caps_new_empty();
    for (auto& type : clearContentTypes) {
        gst_caps_append_structure(caps, gst_structure_new_from_string(type));
    }
    return caps;
}

struct GstCencDecryptImpl {
    std::unique_ptr<IGstDecryptor> _decryptor;
    std::string _keySystem;
    std::mutex _initLock;
    bool _isDecryptorInitialized;
};

void Finalize(GObject* object)
{
    GstCencDecrypt* cencdecrypt = GST_CENCDECRYPT(object);

    GST_DEBUG_OBJECT(cencdecrypt, "finalize");

    cencdecrypt->_impl->_decryptor.reset();
   
    G_OBJECT_CLASS(gst_cencdecrypt_parent_class)->finalize(object);

    WPEFramework::Core::Singleton::Dispose();
}

static void gst_cencdecrypt_class_init(GstCencDecryptClass* klass)
{
    GstBaseTransformClass* base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);

    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
        gst_pad_template_new("src", GST_PAD_SRC, GST_PAD_ALWAYS, SrcCaps()));

    gst_element_class_add_pad_template(GST_ELEMENT_CLASS(klass),
        gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS, SinkCaps(klass)));

    gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass),
        "CENC decryptor", GST_ELEMENT_FACTORY_KLASS_DECODER"/"GST_ELEMENT_FACTORY_KLASS_DECRYPTOR, "Decrypts content with local instance of OpenCDM",
        "Krystian Plata <k.plata@metrological.com>");

    G_OBJECT_CLASS(klass)->finalize = Finalize;

    base_transform_class->transform_caps = GST_DEBUG_FUNCPTR(TransformCaps);
    base_transform_class->transform_ip_on_passthrough = FALSE;
    base_transform_class->transform_ip = GST_DEBUG_FUNCPTR(TransformIp);
    base_transform_class->sink_event = GST_DEBUG_FUNCPTR(SinkEvent);
}

static void gst_cencdecrypt_init(GstCencDecrypt* cencdecrypt)
{
    GstBaseTransform* base = GST_BASE_TRANSFORM(cencdecrypt);
    gst_base_transform_set_in_place(base, TRUE);
    gst_base_transform_set_passthrough(base, FALSE);
    gst_base_transform_set_gap_aware(base, FALSE);

    cencdecrypt->_impl = std::unique_ptr<GstCencDecryptImpl>(new GstCencDecryptImpl());
    cencdecrypt->_impl->_decryptor = IGstDecryptor::Create();

    GST_FIXME_OBJECT(cencdecrypt, "Caps are constructed based on hard coded keysystem values");
}

static gboolean SrcCapsTransform(GstCapsFeatures* features,
    GstStructure* structure,
    gpointer user_data)
{
    // gst_structure_remove_fields takes care of checking
    // if a field with a corresponding name exists.
    GstCencDecryptImpl* impl = reinterpret_cast<GstCencDecryptImpl*>(user_data);

    gst_structure_remove_fields(structure, "base-profile",
        "codec_data",
        "height",
        "framerate",
        "level",
        "pixel-aspect-ratio",
        "profile",
        "rate",
        "width",
        nullptr);

    gst_structure_set(structure, "original-media-type", G_TYPE_STRING, gst_structure_get_name(structure), nullptr);
    gst_structure_set(structure,
        "protection-system", G_TYPE_STRING, impl->_keySystem.c_str(), nullptr);
    gst_structure_set_name(structure, cencPrefix);
    return TRUE;
}

static gboolean SinkCapsTransform(GstCapsFeatures* features,
    GstStructure* structure,
    gpointer user_data)
{
    gst_structure_set_name(structure, gst_structure_get_string(structure, "original-media-type"));
    gst_structure_remove_field(structure, "protection-system");
    gst_structure_remove_field(structure, "original-media-type");
    return TRUE;
}

static GstCaps* TransformCaps(GstBaseTransform* trans, GstPadDirection direction,
    GstCaps* caps, GstCaps* filter)
{
    GstCencDecrypt* cencdecrypt = GST_CENCDECRYPT(trans);
    GstCaps* othercaps = gst_caps_copy(caps);

    GST_DEBUG_OBJECT(cencdecrypt, "transform_caps");

    if (direction == GST_PAD_SRC) {
        GST_DEBUG_OBJECT(cencdecrypt, "Transforming caps going upstream");
        gst_caps_filter_and_map_in_place(othercaps, SrcCapsTransform, nullptr);

    } else {
        GST_DEBUG_OBJECT(cencdecrypt, "Transforming caps going downstream");
        gst_caps_filter_and_map_in_place(othercaps, SinkCapsTransform, nullptr);
    }

    if (filter) {
        GstCaps* intersect;
        intersect = gst_caps_intersect(othercaps, filter);
        gst_caps_unref(othercaps);
        othercaps = intersect;
    }
    return othercaps;
}

static gboolean SinkEvent(GstBaseTransform* trans, GstEvent* event)
{
    GstCencDecrypt* cencdecrypt = GST_CENCDECRYPT(trans);
    GST_DEBUG_OBJECT(cencdecrypt, "sink_event");
    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_PROTECTION: {
        
        std::lock_guard<std::mutex> lk(cencdecrypt->_impl->_initLock);
        if(!cencdecrypt->_impl->_isDecryptorInitialized) {
            const char *systemId, *origin;
            GstBuffer* initData;

            gst_event_parse_protection(event, &systemId, &initData, &origin);
            cencdecrypt->_impl->_keySystem = std::string(systemId);
            BufferView initDataView(initData, GST_MAP_READ);

            auto result = cencdecrypt->_impl->_decryptor->Initialize(
                IExchange::Create(),
                cencdecrypt->_impl->_keySystem,
                std::string(origin ? origin : ""),
                initDataView);

            cencdecrypt->_impl->_isDecryptorInitialized = (result == IGstDecryptor::Status::SUCCESS);
            GST_INFO_OBJECT(cencdecrypt, 
                "Initialize decryptor with keysystem <%s> and initdata <%"GST_PTR_FORMAT">, result: %d", 
                systemId, initData, result);
        }

        gst_event_unref(event);
        return true;
    }
    default: {
        return GST_BASE_TRANSFORM_CLASS(gst_cencdecrypt_parent_class)->sink_event(trans, event);
    }
    }
}

static GstFlowReturn TransformIp(GstBaseTransform* trans, GstBuffer* buffer)
{
    GstCencDecrypt* cencdecrypt = GST_CENCDECRYPT(trans);

    GST_DEBUG_OBJECT(cencdecrypt, "Processing encrypted buffer %"GST_PTR_FORMAT, buffer);
    auto encryptedBuffer = std::make_shared<EncryptedBuffer>(buffer);
    return cencdecrypt->_impl->_decryptor->Decrypt(encryptedBuffer);
}

static gboolean plugin_init(GstPlugin* plugin)
{
    return gst_element_register(plugin, "cencdecrypt", GST_RANK_PRIMARY,
        GST_TYPE_CENCDECRYPT);
}


GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    cencdecrypt,
    "Decryptor plugin using a local instance of OpenCDM",
    plugin_init, 0.1, "LGPL", "gstcencdecryptor", "https://github.com/WebPlatformForEmbedded/gstcencdecryptor/")
