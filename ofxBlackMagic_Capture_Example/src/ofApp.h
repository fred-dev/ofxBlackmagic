#pragma once

#include "ofMain.h"
#include "ofxBlackMagic.h"

// Comment this out to run the same app with a webcam: ofxBlackMagic follows
// ofVideoGrabber's interface, so nothing else changes.
#define USE_BLACKMAGIC

class ofApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void exit();
	void keyPressed(int key);

#ifdef USE_BLACKMAGIC
	ofxBlackMagic cam;
#else
	ofVideoGrabber cam;
#endif
	float frameRate = 0;
	float lastFrameTime = 0;
};
