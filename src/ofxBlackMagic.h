/*
 ofxBlackMagic: capture from Blackmagic DeckLink cards and UltraStudio devices
 (DeckLink SDK 16.0, macOS / Windows / Linux, Desktop Video 16.0 or newer).

 It follows ofVideoGrabber's interface (setDeviceID, setDesiredFrameRate, setup,
 update, isFrameNew, getPixels, getTexture, draw...) so the two can be swapped:

	ofxBlackMagic cam;      // or: ofVideoGrabber cam;
	cam.setDeviceID(0);
	cam.setup(1920, 1080);
	...
	cam.update();
	if(cam.isFrameNew()) { ... cam.getPixels() ... }
	cam.draw(0, 0);

 It is also an ofBaseDraws, ofBaseHasTexture and ofBaseHasPixels, so it can be
 passed anywhere those are accepted (e.g. ofxCv::toCv(cam)).

 Everything is lazy: nothing is converted unless you ask for it.
   getTexture() / draw()   colour on the GPU (shader), the fast path. 16 bit per
                           channel texture when capturing 10 bit.
   getPixels()             colour on the CPU (8 bit RGBA, the SDK's converter): slower,
                           only when you need the pixels.
   getColorPixels16()      16 bit RGBA pixels read back from the GPU (10 bit capture).
   getGrayPixels()         8 bit luma on the CPU, fast (no colour conversion).
   getRawTexture()         the frame as it arrives, for your own shaders.
 Frames are not copied on arrival: the SDK's frame is kept until a newer one is taken.

 Capture format: you choose it, like the resolution and frame rate: setup(width,
 height, framerate) and setPixelFormat() (8 / 10 bit, YUV 4:2:2 / RGB 4:4:4). The
 capture never switches by itself. getSignal() tells what is actually arriving
 (format detection) and getSignalWarning() is not empty when it doesn't match the
 settings (the same warning is logged).

 Devices: listDevices() / listDeviceInfo() describe the devices (model, persistent id,
 interface, inputs), setInputConnection() picks SDI / HDMI / optical, and
 listProfiles() / setProfile() switch the profile of multi-input cards.
 */

#pragma once

#include "ofMain.h"

#include "DeckLinkController.h"

class ofxBlackMagic : public ofBaseDraws, public ofBaseHasTexture, public ofBaseHasPixels {
public:
	// Kept for compatibility: colour conversion now happens on demand in the app
	// thread, so there is no lock to wait for any more.
	enum ColorFrameCaptureMode {
		LOW_LATENCY = 75,
		NO_FRAME_DROPS = 500
	};

	ofxBlackMagic();
	virtual ~ofxBlackMagic();

	// ofVideoGrabber-style setup: uses setDeviceID() and setDesiredFrameRate()
	// (highest progressive frame rate if none was set) and setPixelFormat().
	bool setup(int width, int height);
	bool setup(int width, int height, float framerate, int deviceId = 0, ColorFrameCaptureMode colorFrameCaptureMode = LOW_LATENCY);
	bool setup(BMDDisplayMode displayMode, int deviceId = 0, ColorFrameCaptureMode colorFrameCaptureMode = LOW_LATENCY);
	void setColorFrameCaptureMode(ColorFrameCaptureMode colorFrameCaptureMode);
	ColorFrameCaptureMode getColorFrameCaptureMode();

	// Pixel format of the capture, before setup():
	//   bmdFormat8BitYUV (default)  8 bit YUV 4:2:2
	//   bmdFormat10BitYUV           10 bit YUV 4:2:2 (v210)
	//   bmdFormat8BitARGB           8 bit RGB 4:4:4
	//   bmdFormat10BitRGB           10 bit RGB 4:4:4 (r210)
	// 10 bit formats give 16 bit per channel textures.
	void setPixelFormat(BMDPixelFormat format);
	BMDPixelFormat getPixelFormat() const;
	static string getPixelFormatName(BMDPixelFormat format);

	// What is arriving at the input (format detection): mode, size, frame rate, colour
	// format, bit depth. Empty warning when it matches what the capture is set to.
	DeckLinkSignalInfo getSignal();
	string getSignalWarning();
	// Colour texture from the GPU shader (default) or from the CPU pixels.
	void setUseGpuConversion(bool useGpu);

	// ofVideoGrabber-style device selection. ofVideoDevice::serialID is the device's
	// persistent id (hex, stable across reboots), hardwareName its model,
	// bAvailable false for devices without inputs.
	vector<ofVideoDevice> listDevices();
	// Everything the SDK reports about each device (input number, interface, inputs...)
	vector<DeckLinkDeviceInfo> listDeviceInfo();
	void setDeviceID(int deviceId);
	void setDesiredFrameRate(float framerate);
	vector<string> getDisplayModeNames();   // modes the selected device supports

	// Input connection (SDI, HDMI, optical SDI...). Before setup() it is applied when the
	// device opens; while capturing it switches immediately.
	void setInputConnection(BMDVideoConnection connection);
	vector<BMDVideoConnection> getInputConnections(); // of the open device
	BMDVideoConnection getInputConnection();
	static string getConnectionName(BMDVideoConnection connection);

	// Profiles of multi-input cards (e.g. DeckLink Duo 2: four half duplex inputs or two
	// full duplex). Changing it changes the device list: call listDevices() again.
	vector<DeckLinkProfileInfo> listProfiles(int deviceId);
	bool setProfile(int deviceId, BMDProfileID profile);

	void close(); // should call this in ofApp::exit()
	bool update(); // returns true if there is a new frame
	bool isFrameNew() const;
	bool isInitialized() const;

	bool hasSignal();        // false when nothing is connected to the input
	string getTimecode();    // RP188 or VITC timecode from the source, if any
	string getDeviceName();
	string getDisplayModeName(); // the capture's mode, e.g. "1080p25" (see getSignal() for the input)
	float getFrameRate();        // the capture's frame rate
	BMDPixelFormat getRawPixelFormat(); // bmdFormat8BitYUV, 10BitYUV, 8BitARGB or 10BitRGB
	uint64_t getFramesArrived();
	uint64_t getFramesSkipped();       // arrived faster than the app took them
	uint64_t getFrameNumber();         // of the current frame

	const DeckLinkRawFrame& getRawFrame(); // the current frame as delivered (see DeckLinkController::readFrame)
	vector<unsigned char>& getYuvRaw();    // copy of the raw frame bytes (rowBytes * height)
	ofPixels& getGrayPixels();             // fast
	ofPixels& getColorPixels();            // slow (CPU, 8 bit RGBA)
	ofShortPixels& getColorPixels16();     // 16 bit RGBA, read back from the GPU conversion

	ofTexture& getRawTexture();   // the raw frame as an RGBA8 texture (4 bytes per texel), for your own shaders
	ofTexture& getYuvTexture();   // same as getRawTexture()
	ofTexture& getGrayTexture();  // fast
	ofTexture& getColorTexture(); // GPU conversion (or CPU, see setUseGpuConversion)

	// ofBaseHasPixels / ofBaseHasTexture: the colour image, like ofVideoGrabber
	ofPixels& getPixels() override;
	const ofPixels& getPixels() const override;
	ofTexture& getTexture() override;
	const ofTexture& getTexture() const override;
	void setUseTexture(bool bUseTex) override;
	bool isUsingTexture() const override;

	using ofBaseDraws::draw;
	void draw(float x, float y) const override; // colour
	void draw(float x, float y, float w, float h) const override; // colour

	void drawYuv(float x, float y); // raw
	void drawYuv(float x, float y, float w, float h); // raw

	void drawGray(float x, float y); // fast
	void drawGray(float x, float y, float w, float h); // fast

	void drawColor(float x, float y);
	void drawColor(float x, float y, float w, float h);

	float getWidth() const override;
	float getHeight() const override;

private:
	bool startCapture(BMDDisplayMode displayMode);
	bool openDevice(int deviceId);
	void checkFrameSize();
	bool uploadRawTexture();
	bool convertOnGpu();

	DeckLinkController controller;

	bool grayPixOld, colorPixOld, colorPix16Old, rawCopyOld;
	ofPixels grayPix, colorPix;
	ofShortPixels colorPix16;
	vector<unsigned char> rawCopy;
	bool rawTexOld, grayTexOld, colorTexOld;
	ofTexture rawTex, grayTex, colorTex;

	// GPU conversion
	ofBufferObject pbo[2];
	int pboIndex;
	ofShader convertShader;
	ofFbo convertFbo;
	ofMesh quad;
	bool useGpuConversion;

	int width, height;
	int deviceId;
	float desiredFrameRate;
	bool bFrameNew;
	bool bInitialized;
	bool bUseTexture;
	BMDVideoConnection inputConnection;
	ColorFrameCaptureMode colorFrameCaptureMode;
};
