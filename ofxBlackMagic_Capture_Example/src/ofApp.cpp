#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
	ofSetVerticalSync(true);
	ofSetLogLevel(OF_LOG_VERBOSE);

	cam.listDevices();
	cam.setDeviceID(0);
	cam.setDesiredFrameRate(30);     // leave out for the highest progressive rate
	cam.setup(1920, 1080);
}

//--------------------------------------------------------------
void ofApp::update() {
	cam.update();
	if(cam.isFrameNew()) {
		float now = ofGetElapsedTimef();
		if(lastFrameTime > 0) {
			frameRate = ofLerp(1.0 / (now - lastFrameTime), frameRate, 0.9);
		}
		lastFrameTime = now;
	}
}

//--------------------------------------------------------------
void ofApp::draw() {
	ofBackground(0);
	cam.draw(0, 0, ofGetWidth(), ofGetHeight());

	std::stringstream info;
	info << "capture fps: " << int(frameRate) << "   " << cam.getWidth() << "x" << cam.getHeight();
#ifdef USE_BLACKMAGIC
	info << "\n" << cam.getDeviceName() << "  " << cam.getDisplayModeName()
		 << (cam.hasSignal() ? "" : "   NO SIGNAL");
	if(!cam.getTimecode().empty()) {
		info << "\ntimecode: " << cam.getTimecode();
	}
#endif
	ofDrawBitmapStringHighlight(info.str(), 10, 20);
}

//--------------------------------------------------------------
void ofApp::exit() {
	cam.close();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	if(key == 'f') {
		ofToggleFullscreen();
	}
}
