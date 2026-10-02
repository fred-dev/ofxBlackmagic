#include "ofxBlackMagic.h"

#include "ColorConversion.h"

ofxBlackMagic::ofxBlackMagic()
:grayPixOld(true)
,colorPixOld(true)
,yuvTexOld(true)
,grayTexOld(true)
,colorTexOld(true)
,width(0)
,height(0)
,deviceId(0)
,desiredFrameRate(-1)
,bFrameNew(false)
,bInitialized(false)
,bUseTexture(true)
,currentMode(bmdModeUnknown)
,colorFrameCaptureMode(LOW_LATENCY) {
}

ofxBlackMagic::~ofxBlackMagic() {
	close();
}

bool ofxBlackMagic::startCapture(BMDDisplayMode displayMode) {
    if(displayMode == bmdModeUnknown) {
        ofLogError("ofxBlackMagic") << "Resolution and framerate combination not supported.";
        return false;
    }
    if(!controller.startCaptureWithMode(displayMode)) {
        return false;
    }
    currentMode = displayMode;
    width = controller.getFrameWidth();
    height = controller.getFrameHeight();
    controller.setColorConversionTimeout(this->colorFrameCaptureMode);
    bInitialized = true;
    return true;
}

bool ofxBlackMagic::setup(int width, int height) {
    return setup(width, height, desiredFrameRate, deviceId, colorFrameCaptureMode);
}

bool ofxBlackMagic::setup(int width, int height, float framerate, int deviceId, ColorFrameCaptureMode colorFrameCaptureMode) {
    if(!controller.init()) {
        return false;
    }
    this->deviceId = deviceId;
    if(!controller.selectDevice(deviceId)) {
        return false;
    }
    ofLogVerbose("ofxBlackMagic") << "Available display modes: " << ofToString(controller.getDisplayModeNames());
    this->colorFrameCaptureMode = colorFrameCaptureMode;
    return startCapture(controller.getDisplayMode(width, height, framerate));
}

bool ofxBlackMagic::setup(BMDDisplayMode displayMode, int deviceId, ColorFrameCaptureMode colorFrameCaptureMode) {
    if(!controller.init()) {
        return false;
    }
    this->deviceId = deviceId;
    if(!controller.selectDevice(deviceId)) {
        return false;
    }
    ofLogVerbose("ofxBlackMagic") << "Available display modes: " << ofToString(controller.getDisplayModeNames());
    this->colorFrameCaptureMode = colorFrameCaptureMode;
    // The size comes from the device's own description of the mode
    return startCapture(displayMode);
}


void ofxBlackMagic::setColorFrameCaptureMode(ColorFrameCaptureMode colorFrameCaptureMode) {
    this->colorFrameCaptureMode = colorFrameCaptureMode;
    controller.setColorConversionTimeout(this->colorFrameCaptureMode);
}

ofxBlackMagic::ColorFrameCaptureMode ofxBlackMagic::getColorFrameCaptureMode() {
    return colorFrameCaptureMode;
}

vector<ofVideoDevice> ofxBlackMagic::listDevices() {
    vector<ofVideoDevice> devices;
    if(controller.getDeviceCount() == 0 && !controller.init()) {
        return devices;
    }
    vector<string> names = controller.getDeviceNameList();
    for(size_t i = 0; i < names.size(); i++) {
        ofVideoDevice device;
        device.id = i;
        device.deviceName = names[i];
        device.hardwareName = "Blackmagic DeckLink";
        device.serialID = "";
        device.bAvailable = true;
        devices.push_back(device);
        ofLogNotice("ofxBlackMagic") << i << ": " << names[i];
    }
    return devices;
}

void ofxBlackMagic::setDeviceID(int deviceId) {
    this->deviceId = deviceId;
}

void ofxBlackMagic::setDesiredFrameRate(float framerate) {
    desiredFrameRate = framerate;
}

vector<string> ofxBlackMagic::getDisplayModeNames() {
    return controller.getDisplayModeNames();
}

void ofxBlackMagic::close() {
	if(controller.isCapturing()) {
		controller.stopCapture();
	}
	bInitialized = false;
	bFrameNew = false;
}

void ofxBlackMagic::checkFrameSize() {
	// The input format can change while capturing (format detection)
	int w = controller.getFrameWidth(), h = controller.getFrameHeight();
	if(w > 0 && h > 0 && (w != width || h != height)) {
		ofLogNotice("ofxBlackMagic") << "Input is now " << w << "x" << h;
		width = w;
		height = h;
	}
}

bool ofxBlackMagic::update() {
	if(controller.buffer.swapFront()) {
		checkFrameSize();
		grayPixOld = true, colorPixOld = true;
		yuvTexOld = true, grayTexOld = true, colorTexOld = true;
		bFrameNew = true;
	} else {
		bFrameNew = false;
	}
	return bFrameNew;
}

bool ofxBlackMagic::isFrameNew() const {
	return bFrameNew;
}

bool ofxBlackMagic::isInitialized() const {
	return bInitialized;
}

bool ofxBlackMagic::hasSignal() {
	return controller.hasSignal();
}

string ofxBlackMagic::getTimecode() {
	return controller.getTimecode();
}

string ofxBlackMagic::getDeviceName() {
	vector<string> names = controller.getDeviceNameList();
	int index = controller.getSelectedDeviceIndex();
	return index >= 0 && index < (int)names.size() ? names[index] : "";
}

string ofxBlackMagic::getDisplayModeName() {
	int w, h;
	float rate;
	string name;
	return controller.getDisplayModeInfo(currentMode, w, h, rate, name) ? name : "";
}

float ofxBlackMagic::getFrameRate() {
	int w, h;
	float rate = 0;
	string name;
	controller.getDisplayModeInfo(currentMode, w, h, rate, name);
	return rate;
}

vector<unsigned char>& ofxBlackMagic::getYuvRaw() {
	return controller.buffer.getFront();
}

ofPixels& ofxBlackMagic::getGrayPixels() {
	if(grayPixOld) {
		grayPix.allocate(width, height, OF_IMAGE_GRAYSCALE);
		vector<unsigned char>& raw = getYuvRaw();
		// never read past the frame we actually have (format changes)
		unsigned int n = MIN((size_t)width * height, raw.size() / 2);
		if(n > 0) {
			cby0cry1_to_y(&raw[0], grayPix.getData(), n);
		}
		grayPixOld = false;
	}
	return grayPix;
}

ofPixels& ofxBlackMagic::getColorPixels() {
	if(colorPixOld) {
        if (controller.rgbaFrame) {
            if (controller.rgbaFrame->lock.try_lock_for(std::chrono::milliseconds(colorFrameCaptureMode))) {
                colorPix = controller.rgbaFrame->getPixels();
                controller.rgbaFrame->lock.unlock();
                colorPixOld = false;
            }
        }
	}
	return colorPix;
}

ofTexture& ofxBlackMagic::getYuvTexture() {
	if(yuvTexOld) {
		vector<unsigned char>& raw = getYuvRaw();
		if(raw.size() >= (size_t)width * height * 2) {
			yuvTex.loadData(&raw[0], width / 2, height, GL_RGBA);
		}
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
		ofPixels& pix = getColorPixels();
		if(pix.isAllocated()) {
			colorTex.loadData(pix);
		}
		colorTexOld = false;
	}
	return colorTex;
}

ofPixels& ofxBlackMagic::getPixels() {
	return getColorPixels();
}

const ofPixels& ofxBlackMagic::getPixels() const {
	return const_cast<ofxBlackMagic*>(this)->getColorPixels();
}

ofTexture& ofxBlackMagic::getTexture() {
	return getColorTexture();
}

const ofTexture& ofxBlackMagic::getTexture() const {
	return const_cast<ofxBlackMagic*>(this)->getColorTexture();
}

void ofxBlackMagic::setUseTexture(bool bUseTex) {
	bUseTexture = bUseTex;
}

bool ofxBlackMagic::isUsingTexture() const {
	return bUseTexture;
}

void ofxBlackMagic::draw(float x, float y) const {
    draw(x, y, getWidth(), getHeight());
}
void ofxBlackMagic::draw(float x, float y, float w, float h) const {
    if(!bUseTexture) {
        return;
    }
    const ofTexture& tex = getTexture();
    if(tex.isAllocated()) {
        tex.draw(x, y, w, h);
    }
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
float ofxBlackMagic::getWidth() const {
    return this->width;
}
float ofxBlackMagic::getHeight() const {
    return this->height;
}
