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
	
	ofxBlackMagic cam1, cam2;

	RateTimer timer;

	void setup() {
		ofSetLogLevel(OF_LOG_VERBOSE);
		cam1.setup(bmdModeHD1080i6000, 0);
        cam2.setup(bmdModeHD1080i6000, 1);

	}
	void exit() {
		cam1.close();
          cam2.close();
	}
	void update() {
		if(cam1.update()) {
			timer.tick();
		}
          if(cam2.update()) {
			timer.tick();
		}
	}
	void draw() {
          
		cam1.drawColor(0,0,960,540);
        cam2.drawColor(960,0,960,540);
         
          
		ofDrawBitmapStringHighlight(ofToString((int) timer.getFramerate()), 10, 20);
	}
	void keyPressed(int key) {
		if(key == 'f') {
			ofToggleFullscreen();
		}
	}
};

int main() {
	ofSetupOpenGL(1920, 540, OF_WINDOW);
	ofRunApp(new ofApp());
}
