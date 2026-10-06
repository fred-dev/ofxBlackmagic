#pragma once

// ofxBlackMagic device settings: input connection (SDI / HDMI...), capture mode and
// pixel format, GPU or CPU colour conversion, profiles of multi-input cards, and
// what is actually arriving at the input.
//
// The capture uses exactly the mode and pixel format you set. When the signal is
// different, the warning says what it is: press m to set the capture to it.
//
// keys:  d next device   i next input connection   t next pixel format
//        m set the capture to the signal   g GPU / CPU colour
//        p next profile (multi-input cards)   f fullscreen

#include "ofMain.h"
#include "ofxBlackMagic.h"

class ofApp : public ofBaseApp {
public:
	void setup() override;
	void update() override;
	void draw() override;
	void exit() override;
	void keyPressed(int key) override;

	void open();

	ofxBlackMagic cam;
	vector<DeckLinkDeviceInfo> devices;
	vector<DeckLinkProfileInfo> profiles; // of the current device
	int device = 0;
	BMDDisplayMode mode = bmdModeUnknown; // unknown: 1920x1080 at the highest progressive rate
	BMDPixelFormat pixelFormat = bmdFormat8BitYUV;
	bool gpu = true;
	BMDVideoConnection connection = bmdVideoConnectionUnspecified;
	string message;
	float captureFps = 0, lastFrameTime = 0;
};
