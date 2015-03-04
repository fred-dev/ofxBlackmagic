#include "ofxBlackMagic.h"

#include "ColorConversion.h"

ofxBlackMagic::ofxBlackMagic()
:grayPixOld(true)
,colorPixOld(true)
,yuvTexOld(true)
,grayTexOld(true)
,colorTexOld(true) {
}

bool ofxBlackMagic::setup(int device,unsigned int displayModeSelect) {
	if(!controller.init()) {
		return false;
	}
     
	controller.selectDevice(device);
	vector<string> displayModes = controller.getDisplayModeNames();
	ofLogVerbose("ofxBlackMagic") << "Available display modes: " << ofToString(displayModes);
	BMDDisplayMode displayMode = bmdModeUnknown;
     
     
     displayMode = displayModeSelect;
     
	if(!controller.startCaptureWithMode(displayMode)) {
		return false;
	}
     switch (displayModeSelect) {
               
               
          case 'ntsc':
               this->width= 720, this->height=480;
               break;
          case 'nt23':
               this->width= 720, this->height=480;
               break;	// 3:2 pulldown
          case 'pal ':
               this->width= 720, this->height=576;
               break;
          case 'ntsp':
               this->width= 720, this->height=480;
               break;
          case 'palp':
               this->width= 720, this->height=576;
               break;
               
               /* HD 1080 Modes */
               
          case '23ps':
               this->width= 1920, this->height=1080;
               break;
          case '24ps':
               this->width= 1920, this->height=1080;
               break;
          case 'Hp25':
               this->width= 1920, this->height=1080;
               break;
          case 'Hp29':
               this->width= 1920, this->height=1080;
               break;
          case 'Hp30':
               this->width= 1920, this->height=1080;
               break;
          case 'Hi50':
               this->width= 1920, this->height=1080;
               break;
          case 'Hi59':
               this->width= 1920, this->height=1080;
               break;
          case 'Hi60':
               this->width= 1920, this->height=1080;
               break;	// N.B. This _really_ is 60.00 Hz.
          case 'Hp50':
               this->width= 1920, this->height=1080;
               break;
          case 'Hp59':
               this->width= 1920, this->height=1080;
               break;
          case 'Hp60':
               this->width= 1920, this->height=1080;
               break;	// N.B. This _really_ is 60.00 Hz.
               
               /* HD 720 Modes */
               
          case 'hp50':
               this->width= 1280, this->height=720;
               break;
          case 'hp59':
               this->width= 1280, this->height=720;
               break;
          case 'hp60':
               this->width= 1280, this->height=720;
               break;
               
               /* 2k Modes */
               
          case '2k23':
               this->width= 2048, this->height=1080;
               break;
          case '2k24':
               this->width= 2048, this->height=1080;
               break;
          case '2k25':
               this->width= 2048, this->height=1080;
               break;
               
     }
     
	return true;
}

void ofxBlackMagic::close() {
	if(controller.isCapturing()) {
		controller.stopCapture();
	}
}

bool ofxBlackMagic::update() {
	if(controller.buffer.swapFront()) {
		grayPixOld = true, colorPixOld = true;
		yuvTexOld = true, grayTexOld = true, colorTexOld = true;
		return true;
	} else {
		return false;
	}
}

vector<unsigned char>& ofxBlackMagic::getYuvRaw() {
	return controller.buffer.getFront();
}

ofPixels& ofxBlackMagic::getGrayPixels() {
	if(grayPixOld) {
		grayPix.allocate(width, height, OF_IMAGE_GRAYSCALE);
		unsigned int n = width * height;
		cby0cry1_to_y(&(getYuvRaw()[0]), grayPix.getPixels(), n);
		grayPixOld = false;
	}
	return grayPix;
}

ofPixels& ofxBlackMagic::getColorPixels() {
	if(colorPixOld) {
		colorPix.allocate(width, height, OF_IMAGE_COLOR);
		unsigned int n = width * height;
		cby0cry1_to_rgb(&(getYuvRaw()[0]), colorPix.getPixels(), n);
		colorPixOld = false;
	}
	return colorPix;
}

ofTexture& ofxBlackMagic::getYuvTexture() {
	if(yuvTexOld) {
		yuvTex.loadData(&(getYuvRaw()[0]), width / 2, height, GL_RGBA);
		yuvTexOld = false;
	}
	return yuvTex;
}

ofTexture& ofxBlackMagic::getGrayTexture() {
	if(grayTexOld) {
		grayTex.loadData(getGrayPixels());
		grayTexOld = false;
	}
	return grayTex;
}

ofTexture& ofxBlackMagic::getColorTexture() {
	if(colorTexOld) {
		colorTex.loadData(getColorPixels());
		colorTexOld = false;
	}
	return colorTex;
}




void ofxBlackMagic::drawYuv(float x, float y){
	getYuvTexture().draw(x, y);
}
void ofxBlackMagic::drawYuv(float x, float y, float w, float h){
	getYuvTexture().draw(x, y, w, h);
}

void ofxBlackMagic::drawGray(float x, float y) {
	getGrayTexture().draw(x, y);
}
void ofxBlackMagic::drawGray(float x, float y, float w, float h) {
	getGrayTexture().draw(x, y, w, h);
}

void ofxBlackMagic::drawColor(float x, float y) {
	getColorTexture().draw(x, y);
}
void ofxBlackMagic::drawColor(float x, float y, float w, float h) {
	getColorTexture().draw(x, y, w, h);
}

int ofxBlackMagic::getWidth() {
	return this->width;
}
int ofxBlackMagic::getHeight() {
	return this->height;
<<<<<<< HEAD
}
=======
}
>>>>>>> origin/master
