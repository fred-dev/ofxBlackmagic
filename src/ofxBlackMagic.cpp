#include "ofxBlackMagic.h"
#include "ColorConversion.h"



vector<string> ofxBlackMagic::listDevices(){
     //return grabber.listDevices();
     
}

bool ofxBlackMagic::setup(int device, int displayModeSelect) {
	
	
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
     grabber.setDeviceID(device);
     grabber.initGrabber(width, height);
     
	return true;
}

void ofxBlackMagic::close() {
	grabber.close();
}

bool ofxBlackMagic::update() {
	grabber.update();
}

//vector<unsigned char>& ofxBlackMagic::getYuvRaw() {
//	return controller.buffer.getFront();
//}

ofPixels& ofxBlackMagic::getGrayPixels() {
     ofImage gray;
     gray=grabber.getPixelsRef();
     gray.setImageType(OF_IMAGE_GRAYSCALE);
	return gray;
}

ofPixels& ofxBlackMagic::getColorPixels() {
	return grabber.getPixelsRef();
}

ofTexture& ofxBlackMagic::getYuvTexture() {
	return grabber.getTextureReference();
}

ofTexture& ofxBlackMagic::getGrayTexture() {
	return grabber.getTextureReference();
}

ofTexture& ofxBlackMagic::getColorTexture() {
	return grabber.getTextureReference();
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
     
}

