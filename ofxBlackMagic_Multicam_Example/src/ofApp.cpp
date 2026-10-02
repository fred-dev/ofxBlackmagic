#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
	ofSetLogLevel(OF_LOG_VERBOSE);
	cam1.listDevices();
	cam1.setup(1920, 1080, 30, 0);
	cam2.setup(1920, 1080, 30, 1);
}

//--------------------------------------------------------------
void ofApp::update() {
	cam1.update();
	cam2.update();
}

//--------------------------------------------------------------
void ofApp::draw() {
	ofBackground(0);
	float w = ofGetWidth() / 2;
	float h = w * 9 / 16;
	cam1.draw(0, 0, w, h);
	cam2.draw(w, 0, w, h);
	ofDrawBitmapStringHighlight(cam1.getDeviceName() + (cam1.hasSignal() ? "" : " NO SIGNAL"), 10, 20);
	ofDrawBitmapStringHighlight(cam2.getDeviceName() + (cam2.hasSignal() ? "" : " NO SIGNAL"), w + 10, 20);
}

//--------------------------------------------------------------
void ofApp::exit() {
	cam1.close();
	cam2.close();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	if(key == 'f') {
		ofToggleFullscreen();
	}
}
