## CENC decryptor
Plugin used for integrating OpenCDM's decryption capabilities into gstreamer.

### Dependencies
In order to build and succesfully run the plugin, you'll need the following:
- gstreamer,
- curl, 
- DRM library (widevine, playready),
- RDKServices, ThunderClientLibraries - if you decide to use the ocdm implementation.

There's a possibility to extend this decryptor with your own implementation via the `IGstDecryptor.h` interface. You'll only have to implement the `Initialize` and `Decrypt` calls. A challenge request will be handled by the CURL library.

### Test content
Depending on the keysystem, there are two reliable sources for encrypted assets:
- *Playready* - [It's best to use the official Microsoft's test site.](https://testweb.playready.microsoft.com/Content/Content3X). Keep in mind that (at the time of writing), not all of the assets are supported. OpenCDM does not implement parsing for playready's WRMHEADER version > 4.0.
- *Widevine* - [It's best to use the Youtube certification test asset package.](https://ytlr-cert.appspot.com/latest/main.html?&test_type=encryptedmedia-test#1634555875127) You'll be able to download them from the **Download-Media-files** section. You'll also need to piece together a correct license URL. The app from *Download-source* section will contain a base URL in `lib/eme/licenseManager.js` and asset specific query parameters in `yts.js`.

Since there's no application layer that provides a license URL for specific assets, you'll have to set it yourself via the `OVERRIDE_LA_URL` environment variable.

You can also create your own encrypted sample files, using tools like [shaka-packager](https://github.com/google/shaka-packager).

### Capabilities
The included gstreamer plugin has fixed capabilities of processing `application/x-cenc` & `application/x-webm-enc` assets. The `application/x-webm-enc` assets do not have specific keysystem information attached to them. Due to this, Widevine will be selected as the preferable option for `webm` files. You can override this behavior by setting the `OVERRIDE_WEBM_KEYSYSTEM_UUID` environment variable.