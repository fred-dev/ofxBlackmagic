#pragma once

#include "ofMain.h"
#include "ofxBlackMagic.h"

// Two inputs side by side, e.g. two UltraStudio devices or a two-input DeckLink card.
class ofApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void exit();
	void keyPressed(int key);

	ofxBlackMagic cam1, cam2;
};
