# gstcencdecryptor
:fireworks: &nbsp; Gstreamer plugin to satisfy all your basic cenc decryption needs. :fireworks:
## Milestones
- [x] Clear content passthrough
- [x] Decryption of content with a single encrypted stream 
- [x] Useable as a standalone gstreamer plugin (with gst-* tools)
- [ ] Support the VP9 encoding
- [ ] Decryption of sources with multiple encrypted streams


## Overview
### Usage
In order to give it a spin, you'll need encrypted content. [You can find some here.](#content-cheatsheet)

As with any other gstreamer plugin, you can jump straight into playback with *gst-launch*. 
```Shell
$ gst-launch-1.0 playbin uri=encrypted-content-uri
```
You can peek the element's capabilities with:
```Shell
$ gst-inspect-1.0 cencdecrypt 
```
### Capabilities

:warning: &nbsp; Note: The OCDM implementation requires of you to have a running [OpenCDM server instance](https://github.com/rdkcentral/ThunderNanoServices/blob/master/OpenCDMi/doc/OpenCDMiPlugin.md). By association, you'll probably need to run [Thunder](https://github.com/rdkcentral/Thunder) as well.

The *cencdecrypt* element provides the means for testing encrypted content playback, without the need for a full-fledged browser. In it's current state, the element will accept **h.264** files encrypted with the following keysystems:
- `com.widevine.alpha` - `edef8ba9-79d6-4ace-a3c8-27dcd51d21ed`
- `com.microsoft.playready` - `9a04f079-9840-4286-ab92-e65be0885f95`
- `org.w3.clearkey` - `1077efec-c0b2-4d02-ace3-3c1e52e2fb4b`

## Content cheatsheet
:warning: &nbsp; The element does not support handling manifest files, so you'll have to use packaged .mp4 files with complete PSSH boxes. :warning:
### Playready
Since this plugin heavily relies on the [*OpenCDM server*](https://github.com/rdkcentral/ThunderNanoServices/blob/master/OpenCDMi/doc/OpenCDMiPlugin.md), it's worth mentioning that it doesn't support parsing playready WRMHEADER version > 4.0. Other than that, you should be all set with the [playready test server](https://testweb.playready.microsoft.com/).

- Single encrypted stream: 
```
http://profficialsite.origin.mediaservices.windows.net/4e8b9b4a-ef12-4822-91cd-bb49fb8ad3c9/tears_of_steel.60s.high41.30fps.idr2.8slice.8000kbps.1920x1080.h264.cenc.unaligned.sliceheadersclear.uvu
```
- Multiple encrypted streams(not supported yet):
```
http://profficialsite.origin.mediaservices.windows.net/baa427a0-0716-4c9c-9da0-985a9899fac4/tears_of_steel.60s.high41.30fps.idr2.8slice.8000kbps.1920x1080.h264.2ch.320kbps.aac.cenc.unaligned.sliceheadersclear.uvu
```
### Widevine
Since (at the time of writing) there's no trusted source, which provides widevine content in the desirable format, it's best if you encrypt it yourself. You can do that with the [shaka-packager](https://github.com/google/shaka-packager/releases)(walkthrough [available here](https://google.github.io/shaka-packager/html/tutorials/widevine.html)) or [mp4dash](https://www.bento4.com/). 
