/*
 Everything in ofxBlackMagic is lazy, and only gets allocated or converted when
 you ask for it. "yuv" really means "cb y0 cr y1", i.e., two pixels stored in
 four bytes.
 */



//#define NORDWR

#pragma once

#include "ofMain.h"

#include "DeckLinkController.h"



class ofxBlackMagic {
private:

     
     bool hasHardware;
     int width, height;
     ofVideoGrabber grabber;
     
     bool setup(int device, int displayModeSelect);
	void close(); // should call this in ofApp::exit()
	bool update(); // returns true if there is a new frame
     int getWidth();
     int getHeight();
     
     vector<string> listDevices();
	
     ofPixels& getGrayPixels(); // fast
	ofPixels& getColorPixels(); // slow
	
	ofTexture& getYuvTexture(); // fastest
	ofTexture& getGrayTexture(); // fast
	ofTexture& getColorTexture(); // slower
     
     
	void drawGray(float x, float y); // fast
     void drawGray(float x, float y, float w, float h); // fast
	
     void drawColor(float x, float y); // slower
     void drawColor(float x, float y, float w, float h); // slower

     

     
	
     
     
     
     
     
};