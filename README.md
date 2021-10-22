# Common encryption multimedia decryptor
Plugin used for integrating OpenCDM's decryption capabilities into gstreamer.

## Dependencies
In order to build and succesfully run the plugin, you'll need the following:
- Gstreamer > 1.10,
- CURL > 7.61, 
- DRM library (widevine, playready),
- RDKServices (OpenCDM), ThunderClientLibraries (ocdm) - if you decide to use the ocdm implementation.

## Building
There are already premade build recipes for [Yocto](https://github.com/WebPlatformForEmbedded/meta-wpe/blob/main/recipes-multimedia/cenc/gstreamer1.0-plugins-cencdecrypt_git.bb) and [Buildroot](https://github.com/WebPlatformForEmbedded/buildroot/blob/babcca844750b9b74ca9571bbf6d6c8d9cb7fff8/package/gstreamer1/gst1-cencdecrypt/gst1-cencdecrypt.mk), that will allow you to integrate this package onto your platform. 

When building in a different environment, rules for a normal cmake package apply. If you're extending the `IGstDecryptor.h` interface, make sure to set the cmake flag `GSTCENCDECRYPT_IMPLEMENTATION`, to an approriate value.

## Capabilities
The included gstreamer plugin has fixed capabilities of processing `application/x-cenc` and `application/x-webm-enc` assets. It'll handle `GstCaps` negotiation and in-place transformation of the incoming `GstBuffer` structures. 

Since there's no application layer that provides a license URL for the content, you'll have to set it yourself via the `OVERRIDE_LA_URL` environment variable. 

## Test content
Depending on the keysystem, there are two reliable sources for encrypted assets:
- *Playready* - [It's best to use the official Microsoft's test site.](https://testweb.playready.microsoft.com/Content/Content3X) Keep in mind that (at the time of writing), not all of the assets are supported, specifically with WRMHEADER version > 4.0.
- *Widevine* - [It's best to use the Youtube certification test asset package.](https://ytlr-cert.appspot.com/latest/main.html?&test_type=encryptedmedia-test#1634555875127) You'll be able to download them from the **Download-Media-files** section. You'll also need to piece together a correct license URL. The app from *Download-source* section will contain a base URL in `lib/eme/licenseManager.js` and asset specific query parameters in `yts.js`.

You can also create your own encrypted sample files, using tools like [shaka-packager](https://github.com/google/shaka-packager).

## Quirks

The `application/x-webm-enc` samples do not have specific keysystem information attached to them. Due to this, Widevine will be selected as the preferable option for `webm` files. You can override this behavior by setting the `OVERRIDE_WEBM_KEYSYSTEM_UUID` environment variable.