#include "ofxBlackMagic.h"

#include "ColorConversion.h"

ofxBlackMagic::ofxBlackMagic()
:grayPixOld(true)
,colorPixOld(true)
,yuvTexOld(true)
,grayTexOld(true)
,colorTexOld(true) {
}

bool ofxBlackMagic::setup(int width, int height, float framerate, int deviceId, ColorFrameCaptureMode colorFrameCaptureMode) {
    if(!controller.init()) {
        return false;
    }
    controller.selectDevice(deviceId);
    vector<string> displayModes = controller.getDisplayModeNames();
    ofLogVerbose("ofxBlackMagic") << "Available display modes: " << ofToString(displayModes);
    BMDDisplayMode displayMode = controller.getDisplayMode(width, height, framerate);

    if(displayMode == bmdModeUnknown) {
        ofLogError("ofxBlackMagic") << "Resolution and framerate combination not supported.";
        return false;
    }
    if(!controller.startCaptureWithMode(displayMode)) {
        return false;
    }

    this->colorFrameCaptureMode = colorFrameCaptureMode;
    controller.setColorConversionTimeout(this->colorFrameCaptureMode);

    this->width = width, this->height = height;

    return true;
}

bool ofxBlackMagic::setup(BMDDisplayMode displayMode, int deviceId, ColorFrameCaptureMode colorFrameCaptureMode) {
    if(!controller.init()) {
        return false;
    }
    controller.selectDevice(deviceId);
    vector<string> displayModes = controller.getDisplayModeNames();
    ofLogVerbose("ofxBlackMagic") << "Available display modes: " << ofToString(displayModes);

    if(displayMode == bmdModeUnknown) {
        ofLogError("ofxBlackMagic") << "Resolution and framerate combination not supported.";
        return false;
    }
    if(!controller.startCaptureWithMode(displayMode)) {
        return false;
    }
    if(displayMode == bmdModeNTSC2398
       || displayMode == bmdModeNTSC
       || displayMode == bmdModeNTSCp) {
        this->width = 720;
        this->height = 486;
    } else if( displayMode == bmdModePAL
              || displayMode == bmdModePALp) {
        this->width = 720;
        this->height = 576;
    } else if( displayMode == bmdModeHD720p50
              || displayMode == bmdModeHD720p5994
              || displayMode == bmdModeHD720p60) {
        this->width = 1280;
        this->height = 720;
    } else if( displayMode == bmdModeHD1080p2398
              || displayMode == bmdModeHD1080p24
              || displayMode == bmdModeHD1080p25
              || displayMode == bmdModeHD1080p2997
              || displayMode == bmdModeHD1080p30
              || displayMode == bmdModeHD1080i50
              || displayMode == bmdModeHD1080i5994
              || displayMode == bmdModeHD1080i6000
              || displayMode == bmdModeHD1080p50
              || displayMode == bmdModeHD1080p5994
              || displayMode == bmdModeHD1080p6000) {
        this->width = 1920;
        this->height = 1080;
    } else if( displayMode == bmdMode2k2398
              || displayMode == bmdMode2k24
              || displayMode == bmdMode2k25) {
        this->width = 2048;
        this->height =1556;
    } else if( displayMode == bmdMode2kDCI2398
              || displayMode == bmdMode2kDCI24
              || displayMode == bmdMode2kDCI25) {
        this->width = 2048;
        this->height =1080;
    } else if( displayMode == bmdMode4K2160p2398
              || displayMode == bmdMode4K2160p24
              || displayMode == bmdMode4K2160p25
              || displayMode == bmdMode4K2160p2997
              || displayMode == bmdMode4K2160p30) {
        this->width = 3840;
        this->height =2160;
    } else if( displayMode == bmdMode4kDCI2398
              || displayMode == bmdMode4kDCI24
              || displayMode == bmdMode4kDCI25) {
        this->width = 4096;
        this->height =2160;
    }
    

    
    this->colorFrameCaptureMode = colorFrameCaptureMode;
    controller.setColorConversionTimeout(this->colorFrameCaptureMode);

    return true;
}


void ofxBlackMagic::setColorFrameCaptureMode(ColorFrameCaptureMode colorFrameCaptureMode) {
    this->colorFrameCaptureMode = colorFrameCaptureMode;
    controller.setColorConversionTimeout(this->colorFrameCaptureMode);
}

ofxBlackMagic::ColorFrameCaptureMode ofxBlackMagic::getColorFrameCaptureMode() {
    return colorFrameCaptureMode;
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
		cby0cry1_to_y(&(getYuvRaw()[0]), grayPix.getData(), n);
		grayPixOld = false;
	}
	return grayPix;
}

ofPixels& ofxBlackMagic::getColorPixels() {
	if(colorPixOld) {
//		colorPix.allocate(width, height, OF_IMAGE_COLOR);
//		unsigned int n = width * height;
//		cby0cry1_to_rgb(&(getYuvRaw()[0]), colorPix.getPixels(), n);
//		colorPixOld = false;
        
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

}
