/*
 Everything in ofxBlackMagic is lazy, and only gets allocated or converted when
 you ask for it. "yuv" really means "cb y0 cr y1", i.e., two pixels stored in
 four bytes.

 ofxBlackMagic follows ofVideoGrabber's interface (setDeviceID, setDesiredFrameRate,
 setup, update, isFrameNew, getPixels, getTexture, draw...) so the two can be swapped:

	ofxBlackMagic cam;      // or: ofVideoGrabber cam;
	cam.setDeviceID(0);
	cam.setup(1920, 1080);
	...
	cam.update();
	if(cam.isFrameNew()) { ... cam.getPixels() ... }
	cam.draw(0, 0);

 It is also an ofBaseDraws, ofBaseHasTexture and ofBaseHasPixels, so it can be
 passed anywhere those are accepted (e.g. ofxCv::toCv(cam)).
 */

#pragma once

#include "ofMain.h"

#include "DeckLinkController.h"

class ofxBlackMagic : public ofBaseDraws, public ofBaseHasTexture, public ofBaseHasPixels {
public:
    // Color Frame Capture Mode
    // A lock is necessary to prevent reading from the VideoFrame whilst writing to it.
    // A value 500 milliseconds would be for a 'frame critical' application, as a value of 75 milliseconds would be for a 'latency critical' application.
    // See PR #8 - https://github.com/kylemcdonald/ofxBlackmagic/pull/8
    enum ColorFrameCaptureMode {
        LOW_LATENCY = 75,
        NO_FRAME_DROPS = 500
    };

    ofxBlackMagic();
    virtual ~ofxBlackMagic();

    // ofVideoGrabber-style setup: uses setDeviceID() and setDesiredFrameRate()
    // (highest progressive frame rate if none was set)
    bool setup(int width, int height);
    bool setup(int width, int height, float framerate, int deviceId = 0, ColorFrameCaptureMode colorFrameCaptureMode = LOW_LATENCY);
    bool setup(BMDDisplayMode displayMode, int deviceId = 0, ColorFrameCaptureMode colorFrameCaptureMode = LOW_LATENCY);
    void setColorFrameCaptureMode(ColorFrameCaptureMode colorFrameCaptureMode); // If you want to set a custom value, you just need to cast it as ofxBlackMagic::ColorFrameCaptureMode
    ColorFrameCaptureMode getColorFrameCaptureMode();

    // ofVideoGrabber-style device selection
    vector<ofVideoDevice> listDevices();
    void setDeviceID(int deviceId);
    void setDesiredFrameRate(float framerate);
    vector<string> getDisplayModeNames();   // modes the selected device supports

    void close(); // should call this in ofApp::exit()
    bool update(); // returns true if there is a new frame
    bool isFrameNew() const;
    bool isInitialized() const;

    bool hasSignal();        // false when nothing is connected to the input
    string getTimecode();    // RP188 or VITC timecode from the source, if any
    string getDeviceName();
    string getDisplayModeName();
    float getFrameRate();

    vector<unsigned char>& getYuvRaw(); // fastest
    ofPixels& getGrayPixels(); // fast
    ofPixels& getColorPixels(); // slow

    ofTexture& getYuvTexture(); // fastest
    ofTexture& getGrayTexture(); // fast
    ofTexture& getColorTexture(); // slower

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

    void drawYuv(float x, float y); // fastest
    void drawYuv(float x, float y, float w, float h); // fastest

    void drawGray(float x, float y); // fast
    void drawGray(float x, float y, float w, float h); // fast

    void drawColor(float x, float y); // slower
    void drawColor(float x, float y, float w, float h); // slower

    float getWidth() const override;
    float getHeight() const override;

private:
	bool startCapture(BMDDisplayMode displayMode);
	void checkFrameSize();

	DeckLinkController controller;

	bool grayPixOld, colorPixOld;
	ofPixels yuvPix, grayPix, colorPix;
	bool yuvTexOld, grayTexOld, colorTexOld;
	ofTexture yuvTex, grayTex, colorTex;

	int width, height;
	int deviceId;
	float desiredFrameRate;
	bool bFrameNew;
	bool bInitialized;
	bool bUseTexture;
	BMDDisplayMode currentMode;
    ColorFrameCaptureMode colorFrameCaptureMode;
};
