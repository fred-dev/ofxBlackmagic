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
	
	ofxBlackMagic cam1;
     ofxBlackMagic cam2;
	RateTimer timer;

     
	
	void setup() {
          
//          bmdModeNTSC                                                  = 'ntsc',
//          bmdModeNTSC2398                                              = 'nt23',	// 3:2 pulldown
//          bmdModePAL                                                   = 'pal ',
//          bmdModeNTSCp                                                 = 'ntsp',
//          bmdModePALp                                                  = 'palp',
//          
//          /* HD 1080 Modes */
//          
//          bmdModeHD1080p2398                                           = '23ps',
//          bmdModeHD1080p24                                             = '24ps',
//          bmdModeHD1080p25                                             = 'Hp25',
//          bmdModeHD1080p2997                                           = 'Hp29',
//          bmdModeHD1080p30                                             = 'Hp30',
//          bmdModeHD1080i50                                             = 'Hi50',
//          bmdModeHD1080i5994                                           = 'Hi59',
//          bmdModeHD1080i6000                                           = 'Hi60',	// N.B. This _really_ is 60.00 Hz.
//          bmdModeHD1080p50                                             = 'Hp50',
//          bmdModeHD1080p5994                                           = 'Hp59',
//          bmdModeHD1080p6000                                           = 'Hp60',	// N.B. This _really_ is 60.00 Hz.
//          
//          /* HD 720 Modes */
//          
//          bmdModeHD720p50                                              = 'hp50',
//          bmdModeHD720p5994                                            = 'hp59',
//          bmdModeHD720p60                                              = 'hp60',
//          
//          /* 2k Modes */
//          
//          bmdMode2k2398                                                = '2k23',
//          bmdMode2k24                                                  = '2k24',
//          bmdMode2k25                                                  = '2k25',
//          

		ofSetLogLevel(OF_LOG_VERBOSE);
		cam1.setup(0,'Hp25' );
          cam2.setup(1,'Hp25');

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