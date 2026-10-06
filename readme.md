# ofxBlackMagic is an addon for [openFrameworks](http://openframeworks.cc/)

> **About this fork:** Fork of [kylemcdonald/ofxBlackmagic](https://github.com/kylemcdonald/ofxBlackmagic) (2014-2018). Adds an `ofVideoGrabber`-style interface, input format detection, device information, Linux and Windows support, and is updated to the **DeckLink SDK 16.0** on every platform.

Capture from Blackmagic DeckLink cards and UltraStudio devices.

* All DeckLink specific functionality is placed in `DeckLinkController.h`, which can be extended if you're interested in getting minimum latency by overloading the `VideoInputFrameArrived()` callback.
* Frames are triple buffered: minimum delay to the DeckLink device and no tearing on the display side.
* Nothing is converted unless you ask for it; grayscale is provided for situations where colour is unnecessary.
* You set the capture format (resolution, frame rate, pixel format) and it stays that way. With format detection (most devices) `getSignal()` tells what is actually arriving, and when it doesn't match the settings `getSignalWarning()` (also logged) says what the signal is.

### What's in it

* **GPU colour conversion**: `getTexture()` / `draw()` convert the raw frame to RGB in a shader (8 bit YUV, 10 bit YUV v210, 8 bit ARGB, 10 bit RGB r210; Rec.709 for HD and up, Rec.601 for SD). `getPixels()` still gives CPU pixels (the SDK's converter) when you need them, only then. `setUseGpuConversion(false)` uses the CPU for the texture too.
* **Pixel format**: `setPixelFormat()` before `setup()`: `bmdFormat8BitYUV` (default), `bmdFormat10BitYUV`, `bmdFormat8BitARGB` (RGB 4:4:4) or `bmdFormat10BitRGB`. With 10 bit formats the colour texture is 16 bit per channel and `getColorPixels16()` reads it back as `ofShortPixels`. If the signal is a different bit depth or colour format the device converts it and you get a warning.
* **Signal information**: `getSignal()` (mode, size, frame rate, YUV / RGB, bit depth of what is arriving) and `getSignalWarning()` (empty when it matches the capture settings). The capture never switches by itself: set it to the signal yourself (see the settings example's `m` key).
* **Input connection and profiles**: `getInputConnections()` / `setInputConnection(bmdVideoConnectionSDI / HDMI / OpticalSDI...)` (also while capturing), `listProfiles()` / `setProfile()` for multi-input cards (e.g. DeckLink Duo 2: two full duplex or four half duplex inputs; list the devices again afterwards).
* **No copies on arrival**: the SDK's frame is kept (reference counted) until the app takes a newer one, and is read in place. The raw frame goes to the GPU through two alternating pixel buffer objects, so uploads don't stall. `getRawTexture()` gives it to your own shaders.
* **Device information**: `listDevices()` (name, model, persistent id in `serialID`) and `listDeviceInfo()` (input number, interface, inputs, capture / playback, format detection).
* **Counters**: `getFramesArrived()`, `getFramesSkipped()` (frames that arrived faster than the app took them), `getFrameNumber()`.

`ofxBlackMagic_Settings_Example` shows all of it (keys: d device, i input, t pixel format, m set the capture to the signal, g GPU/CPU, p profile).

```cpp
ofxBlackMagic cam;          // follows ofVideoGrabber's interface
cam.listDevices();          // name, model, persistent id (serialID), interface
cam.setDeviceID(0);
cam.setPixelFormat(bmdFormat8BitYUV);
cam.setup(1920, 1080);      // 1080 at the highest progressive rate (setDesiredFrameRate to choose)
...
cam.update();
if (cam.isFrameNew()) { cam.getGrayPixels(); cam.getPixels(); }
cam.draw(0, 0);
cam.getSignal().describe(); // what is arriving, e.g. "1080p25 1920x1080, 10 bit YUV 4:2:2"
cam.getSignalWarning();     // not empty when it doesn't match the capture settings
cam.hasSignal(); cam.getTimecode(); cam.getFramesSkipped();
```

## Installation

1. Install **Blackmagic Desktop Video 16.0 or newer** (the drivers) from [Blackmagic support](https://www.blackmagicdesign.com/support). The addon is built against DeckLink SDK 16.0 and the driver must be at least that version. Check that Blackmagic's own capture app (Media Express or the SDK's CapturePreview sample) shows the signal.
2. Generate your project with the projectGenerator, with `ofxBlackmagic` in `addons.make`.

The SDK headers are in `libs/DeckLink` (Mac, Linux and Win), so the SDK itself doesn't need to be installed.

### macOS

Nothing else. On Desktop Video's settings, un-check "1080PsF" for 1080p cameras on older devices.

### Linux

Links `-ldl -lpthread` (set in `addon_config.mk`). Tested on Ubuntu with the Desktop Video packages installed.

### Windows

On Windows the SDK only ships interface definitions (`.idl`), which Microsoft's MIDL compiler turns into the C++ header. Do this **once** (and again after updating the SDK):

1. Open the **x64 Native Tools Command Prompt for VS** (it comes with Visual Studio's C++ workload).
2. Run `scripts\generate_windows_headers.bat` from the addon folder.

It writes `libs/DeckLink/Win/include/DeckLinkAPI.h` and `libs/DeckLink/Win/src/DeckLinkAPI_i.c`; commit them so other machines don't need this step. Then generate the project with the projectGenerator as usual. COM is initialised by the addon; `ole32` / `oleaut32` are linked automatically.

## Updating the SDK

Copy a new SDK's `Mac/include/*.h`, `Linux/include/*.h` and `Win/include/*.idl` + `DeckLinkAPIVersion.h` into `libs/DeckLink/Mac/include`, `Linux/include`, `Win/idl` + `Win/include`, the two `DeckLinkAPIDispatch.cpp` into `Mac/src` and `Linux/src` (not the versioned `_vXX` ones), and run the Windows script again.

## Supported systems

Originally checked on macOS with an UltraStudio Mini Recorder and an UltraStudio 4K, and on Ubuntu 18.04. The SDK 16 update is written for macOS, Linux and Windows; please report devices you have tested.
