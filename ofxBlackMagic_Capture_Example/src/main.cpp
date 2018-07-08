#include "ofMain.h"
#include "ofxBlackMagic.h"

class RateTimer {
protected:
	float lastTick, averagePeriod, smoothing;
	bool secondTick;
public:
	RateTimer() :
	smoothing(.9) {
		reset();
	}
	void reset() {
		lastTick = 0, averagePeriod = 0, secondTick = false;
	}
	void setSmoothing(float smoothing) {
		this->smoothing = smoothing;
	}
	float getFramerate() {
		return averagePeriod == 0 ? 0 : 1 / averagePeriod;
	}
	void tick() {
		float curTick = ofGetElapsedTimef();
		if(lastTick == 0) {
			secondTick = true;
		} else {
			float curDiff = curTick - lastTick;;
			if(secondTick) {
				averagePeriod = curDiff;
				secondTick = false;
			} else {
				averagePeriod = ofLerp(curDiff, averagePeriod, smoothing);
			}
		}
		lastTick = curTick;
	}
};

class ofApp : public ofBaseApp {
public:
	
	ofxBlackMagic cam;
	RateTimer timer;
	
	void setup() {
		ofSetLogLevel(OF_LOG_VERBOSE);
    cam.setup(bmdModeHD1080i6000);
    }
	void exit() {
		cam.close();
	}
	void update() {
		if(cam.update()) {
			timer.tick();
		}
	}
	void draw() {
        cam.drawColor(0,0,1920,1080);
		ofDrawBitmapStringHighlight(ofToString((int) timer.getFramerate()), 10, 20);
	}
	void keyPressed(int key) {
		if(key == 'f') {
			ofToggleFullscreen();
		}
	}
};

int main() {
    ofSetupOpenGL(1920, 1080, OF_WINDOW);
	ofRunApp(new ofApp());
}
