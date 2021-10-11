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
constexpr static auto cencMime = "application/x-cenc";
constexpr static auto webmMime = "application/x-webm-enc";

static std::map<std::string, std::string> keySystems{ { "edef8ba9-79d6-4ace-a3c8-27dcd51d21ed", "com.widevine.alpha" },
    { "9a04f079-9840-4286-ab92-e65be0885f95", "com.microsoft.playready" }, {"1077efec-c0b2-4d02-ace3-3c1e52e2fb4b", "org.w3.clearkey"}};


static GstCaps* TransformCaps(GstBaseTransform* trans, GstPadDirection direction,
    GstCaps* caps, GstCaps* filter);
static gboolean SinkEvent(GstBaseTransform* trans, GstEvent* event);
static GstFlowReturn TransformIp(GstBaseTransform* trans, GstBuffer* buffer);

static GstCaps* SinkCaps(GstCencDecryptClass* klass)
{
    GstCaps* caps = gst_caps_new_empty();
    for (auto& type : clearContentTypes) {
        gst_caps_append_structure(caps,
            gst_structure_new(webmMime,
                "original-media-type", G_TYPE_STRING, type, NULL));
                
        for (auto& system : keySystems) {
            gst_caps_append_structure(caps,
                gst_structure_new(cencMime,
                    "original-media-type", G_TYPE_STRING, type,
                    "protection-system", G_TYPE_STRING, system.first.c_str(), NULL));
        }
    }
    return caps;
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
        "CENC decryptor", GST_ELEMENT_FACTORY_KLASS_DECRYPTOR, "Decrypts content with local instance of OpenCDM",
        "FIXME <k.plata@metrological.com>");

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
    
    if(impl->_keySystem == GST_PROTECTION_UNSPECIFIED_SYSTEM_ID) {
        gst_structure_set_name(structure, webmMime);
    } else {
        gst_structure_set(structure,
            "protection-system", G_TYPE_STRING, impl->_keySystem.c_str(), nullptr);
        gst_structure_set_name(structure, cencMime);
    }

    return TRUE;
}

static gboolean SinkCapsTransform(GstCapsFeatures* features,
    GstStructure* structure,
    gpointer user_data)
{
    gst_structure_set_name(structure, gst_structure_get_string(structure, "original-media-type"));
    gst_structure_remove_field(structure, "protection-system");
    gst_structure_remove_field(structure, "original-media-type");
    gst_structure_remove_field(structure, "encryption-algorithm");
    gst_structure_remove_field(structure, "encoding-scope");
    gst_structure_remove_field(structure, "cipher-mode");
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

static void InitializeDecryptor(GstCencDecrypt* cencdecrypt, 
    const std::string& keySystem, 
    const std::string& origin, 
    GstBuffer* initData)
{
    std::lock_guard<std::mutex> lk(cencdecrypt->_impl->_initLock);
    if(!cencdecrypt->_impl->_isDecryptorInitialized) {

        BufferView initDataView(initData, GST_MAP_READ);
        auto result = cencdecrypt->_impl->_decryptor->Initialize(
                    keySystem,
                    origin,
                    initDataView);

        cencdecrypt->_impl->_isDecryptorInitialized = (result == IGstDecryptor::Status::SUCCESS);

        GST_MEMDUMP_OBJECT(cencdecrypt, "Initializing decryptor with initData", initDataView.Raw(), initDataView.Size());
    }
}

static gboolean SinkEvent(GstBaseTransform* trans, GstEvent* event)
{
    GstCencDecrypt* cencdecrypt = GST_CENCDECRYPT(trans);
    GST_DEBUG_OBJECT(cencdecrypt, "sink_event");
    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_PROTECTION: {
        const char *systemId, *origin;
        GstBuffer* initData;
        gst_event_parse_protection(event, &systemId, &initData, &origin);
        cencdecrypt->_impl->_keySystem = std::string(systemId);
        
        // MP4 parser will raise this event multiple times for earch supported keysystem.
        // In the case of a WebM container, there will be no information about the keysystem 
        // that is meant to be used by the decryptor. Because of this, we're going to try 
        // with WideVine by default and allow for overriding the UUID.

        std::string keySystem;
        if(cencdecrypt->_impl->_keySystem == GST_PROTECTION_UNSPECIFIED_SYSTEM_ID) {
            auto overrideKeySystem = std::getenv("OVERRIDE_WEBM_KEYSYSTEM_UUID");
            keySystem = (overrideKeySystem != nullptr) ? overrideKeySystem : "edef8ba9-79d6-4ace-a3c8-27dcd51d21ed";
        } else {
            keySystem = cencdecrypt->_impl->_keySystem;
        }

        InitializeDecryptor(cencdecrypt, keySystem, std::string(origin ? origin : ""), initData);

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

#ifndef VERSION
#define VERSION "0.2"
#endif
#ifndef PACKAGE
#define PACKAGE "gstcencdecryptor"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "gstcencdecryptor"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "https://github.com/WebPlatformForEmbedded/gstcencdecryptor/"
#endif

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    cencdecrypt,
    "Decryptor plugin using a local instance of OpenCDM",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)
