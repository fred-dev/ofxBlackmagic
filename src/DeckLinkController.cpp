
#include "DeckLinkController.h"

namespace {
#ifdef TARGET_OSX
	// CFStringGetCStringPtr may return NULL, so copy into a buffer instead
	string toString(CFStringRef cfString) {
		char buffer[512];
		if (cfString != NULL && CFStringGetCString(cfString, buffer, sizeof(buffer), kCFStringEncodingUTF8)) {
			return string(buffer);
		}
		return "";
	}
#endif
}

DeckLinkController::DeckLinkController()
: selectedDevice(NULL)
, deckLinkInput(NULL)
, supportFormatDetection(false)
, currentlyCapturing(false)
, selectedIndex(-1)
, videoConverter(NULL)
, colorConversionTimeout(75)
, frameWidth(0)
, frameHeight(0)
, signalPresent(false)
, rgbaFrame(NULL)  {
}

DeckLinkController::~DeckLinkController()  {
	if (currentlyCapturing) {
		stopCapture();
	}
	while (modeList.size() > 0) {
		modeList.back()->Release();
		modeList.pop_back();
	}
	if (deckLinkInput != NULL) {
		deckLinkInput->Release();
	}
	
	// Release the IDeckLink list
	for (auto device : deviceList) {
		device->Release();
	}
    
    // Release the IDeckLinkVideoConversion
    if (this->videoConverter) {
        this->videoConverter->Release();
    }
    
    delete rgbaFrame;
}

bool DeckLinkController::init()  {
	IDeckLinkIterator* deckLinkIterator = NULL;
	IDeckLink* deckLink = NULL;
	bool result = false;
	
	// Create an iterator
	deckLinkIterator = CreateDeckLinkIteratorInstance();
	if (deckLinkIterator == NULL) {
		ofLogError("DeckLinkController") << "Please install the Blackmagic Desktop Video drivers to use the features of this application.";
		goto bail;
	}
	
	// List all DeckLink devices (init() may be called again, e.g. by listDevices())
	for (auto device : deviceList) {
		device->Release();
	}
	deviceList.clear();
	while (deckLinkIterator->Next(&deckLink) == S_OK) {
		// Add device to the device list
		deviceList.push_back(deckLink);
	}
	
	if (deviceList.size() == 0) {
		ofLogError("DeckLinkController") << "You will not be able to use the features of this application until a Blackmagic device is installed.";
		goto bail;
	}
    
    // Create a video converter (the instance starts with one reference)
    videoConverter = CreateVideoConversionInstance();
	
	result = true;
	
bail:
	if (deckLinkIterator != NULL) {
		deckLinkIterator->Release();
		deckLinkIterator = NULL;
	}
	
	return result;
}


int DeckLinkController::getDeviceCount()  {
	return deviceList.size();
}


#ifdef TARGET_OSX
vector<string> DeckLinkController::getDeviceNameList()  {
    vector<string> nameList;
    int deviceIndex = 0;
    
    while (deviceIndex < deviceList.size()) {
        CFStringRef cfStrName;
        
        // Get the name of this device
        if (deviceList[deviceIndex]->GetDisplayName(&cfStrName) == S_OK) {
            nameList.push_back(toString(cfStrName));
            CFRelease(cfStrName);
        }
        else {
            nameList.push_back("DeckLink");
        }
        
        deviceIndex++;
    }
    
    return nameList;
}
#else
vector<string> DeckLinkController::getDeviceNameList()  {
    vector<string> nameList;
    int deviceIndex = 0;
    
    while (deviceIndex < deviceList.size()) {
        char* cfStrName;
        // Get the name of this device
        if (deviceList[deviceIndex]->GetDisplayName((const char**)&cfStrName) == S_OK) {
            nameList.push_back(string(cfStrName));
            
        }
        else {
            nameList.push_back("DeckLink");
        }
        
        deviceIndex++;
    }
    
    return nameList;
}
#endif





bool DeckLinkController::selectDevice(int index)  {
	DeckLinkAttributesInterface* deckLinkAttributes = NULL;
	IDeckLinkDisplayModeIterator* displayModeIterator = NULL;
	IDeckLinkDisplayMode* displayMode = NULL;
	bool result = false;
	
	// Check index
	if (index < 0 || index >= deviceList.size()) {
		ofLogError("DeckLinkController") << "This application was unable to select the device.";
		goto bail;
	}
	
	// A new device has been selected.
	// Release the previous selected device and mode list
	if (currentlyCapturing)
		stopCapture();
	if (deckLinkInput != NULL) {
		deckLinkInput->Release();
		deckLinkInput = NULL;
	}
	
	while(modeList.size() > 0) {
		modeList.back()->Release();
		modeList.pop_back();
	}
	
	
	// Get the IDeckLinkInput for the selected device
	if ((deviceList[index]->QueryInterface(IID_IDeckLinkInput, (void**)&deckLinkInput) != S_OK)) {
		ofLogError("DeckLinkController") << "This application was unable to obtain IDeckLinkInput for the selected device.";
		deckLinkInput = NULL;
		goto bail;
	}
	
	//
	// Retrieve and cache mode list
	if (deckLinkInput->GetDisplayModeIterator(&displayModeIterator) == S_OK) {
		while (displayModeIterator->Next(&displayMode) == S_OK)
			modeList.push_back(displayMode);
		
		displayModeIterator->Release();
	}
	
	//
	// Check if input mode detection format is supported.
	
	supportFormatDetection = false; // assume unsupported until told otherwise
	if (deviceList[index]->QueryInterface(DECKLINK_ATTRIBUTES_IID, (void**) &deckLinkAttributes) == S_OK) {
		if (deckLinkAttributes->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &supportFormatDetection) != S_OK)
			supportFormatDetection = false;
		
		deckLinkAttributes->Release();
	}
	
	selectedIndex = index;
	result = true;
	
bail:
	return result;
}

void DeckLinkController::setColorConversionTimeout(int ms)  {
    colorConversionTimeout = ms;
}
#ifdef TARGET_OSX
vector<string> DeckLinkController::getDisplayModeNames()  {
    vector<string> modeNames;
    int modeIndex;
    CFStringRef modeName;
    
    for (modeIndex = 0; modeIndex < modeList.size(); modeIndex++) {
        if (modeList[modeIndex]->GetName(&modeName) == S_OK) {
            modeNames.push_back(toString(modeName));
            CFRelease(modeName);
        }
        else {
            modeNames.push_back("Unknown mode");
        }
    }
    
    return modeNames;
}
#else
vector<string> DeckLinkController::getDisplayModeNames()  {
    vector<string> modeNames;
    int modeIndex;
    char* modeName;
    
    for (modeIndex = 0; modeIndex < modeList.size(); modeIndex++) {
        if (modeList[modeIndex]->GetName((const char**)&modeName) == S_OK) {
            modeNames.push_back(string(modeName));
            
        }
        else {
            modeNames.push_back("Unknown mode");
        }
    }
    
    return modeNames;
}
#endif


bool DeckLinkController::isFormatDetectionEnabled()  {
	return supportFormatDetection;
}

bool DeckLinkController::isCapturing()  {
	return currentlyCapturing;
}

IDeckLinkDisplayMode* DeckLinkController::findMode(BMDDisplayMode mode) {
	for (auto m : modeList) {
		if (m->GetDisplayMode() == mode) {
			return m;
		}
	}
	return NULL;
}

unsigned long DeckLinkController::getDisplayModeBufferSize(BMDDisplayMode mode) {
	// 8 bit YUV (2vuy): two bytes per pixel
	IDeckLinkDisplayMode* m = findMode(mode);
	return m == NULL ? 0 : (unsigned long)m->GetWidth() * m->GetHeight() * 2;
}

bool DeckLinkController::getDisplayModeInfo(BMDDisplayMode mode, int& w, int& h, float& framerate, string& name) {
	IDeckLinkDisplayMode* m = findMode(mode);
	if (m == NULL) {
		return false;
	}
	w = m->GetWidth();
	h = m->GetHeight();
	BMDTimeValue duration;
	BMDTimeScale scale;
	m->GetFrameRate(&duration, &scale);
	framerate = duration > 0 ? float(scale) / float(duration) : 0;
#ifdef TARGET_OSX
	CFStringRef cfName;
	if (m->GetName(&cfName) == S_OK) {
		name = toString(cfName);
		CFRelease(cfName);
	}
#else
	const char* cName;
	if (m->GetName(&cName) == S_OK) {
		name = cName;
	}
#endif
	return true;
}

bool DeckLinkController::startCaptureWithIndex(int videoModeIndex)  {
	// Get the IDeckLinkDisplayMode from the given index
	if ((videoModeIndex < 0) || (videoModeIndex >= modeList.size())) {
		ofLogError("DeckLinkController") << "An invalid display mode was selected.";
		return false;
	}
	
	return startCaptureWithMode(modeList[videoModeIndex]->GetDisplayMode());
}

bool DeckLinkController::startCaptureWithMode(BMDDisplayMode videoMode) {
    int bufferSize = getDisplayModeBufferSize(videoMode);
    
	if (deckLinkInput == NULL) {
		ofLogError("DeckLinkController") << "No device selected.";
		return false;
	}
	if(bufferSize != 0) {
		vector<unsigned char> prototype(bufferSize);
		buffer.setup(prototype);
		IDeckLinkDisplayMode* m = findMode(videoMode);
		frameWidth = m->GetWidth();
		frameHeight = m->GetHeight();
	} else{
		ofLogError("DeckLinkController") << "The selected device does not support that display mode.";
		return false;
	}
	
	BMDVideoInputFlags videoInputFlags;
	
	// Enable input video mode detection if the device supports it
	videoInputFlags = supportFormatDetection ? bmdVideoInputEnableFormatDetection : bmdVideoInputFlagDefault;
	
	// Set capture callback
	deckLinkInput->SetCallback(this);
	
	// Set the video input mode
	if (deckLinkInput->EnableVideoInput(videoMode, bmdFormat8BitYUV, videoInputFlags) != S_OK) {
		ofLogError("DeckLinkController") << "This application was unable to select the chosen video mode. Perhaps, the selected device is currently in-use.";
		return false;
	}
	
	// Start the capture
	if (deckLinkInput->StartStreams() != S_OK) {
		ofLogError("DeckLinkController") << "This application was unable to start the capture. Perhaps, the selected device is currently in-use.";
		return false;
	}
	
	currentlyCapturing = true;
	
	return true;
}

void DeckLinkController::stopCapture()  {
	if (deckLinkInput == NULL) {
		return;
	}
	// Stop the capture
	deckLinkInput->StopStreams();
	deckLinkInput->DisableVideoInput();
	
	// Delete capture callback
	deckLinkInput->SetCallback(NULL);
	
	currentlyCapturing = false;
}


HRESULT DeckLinkController::VideoInputFormatChanged (/* in */ BMDVideoInputFormatChangedEvents notificationEvents, /* in */ IDeckLinkDisplayMode *newMode, /* in */ BMDDetectedVideoInputFormatFlags detectedSignalFlags)  {
	bool shouldRestartCaptureWithNewVideoMode = true;
	
	// Restart capture with the new video mode if told to. The frame buffers are
	// resized as the first frames of the new mode arrive.
	if (shouldRestartCaptureWithNewVideoMode) {
		ofLogNotice("DeckLinkController") << "Input format changed to " << newMode->GetWidth() << "x" << newMode->GetHeight();
		// Stop the capture
		deckLinkInput->StopStreams();
		
		// Set the video input mode
		if (deckLinkInput->EnableVideoInput(newMode->GetDisplayMode(), bmdFormat8BitYUV, bmdVideoInputEnableFormatDetection) != S_OK) {
			ofLogError("DeckLinkController") << "This application was unable to select the new video mode.";
			goto bail;
		}
		
		// Start the capture
		if (deckLinkInput->StartStreams() != S_OK) {
			ofLogError("DeckLinkController") << "This application was unable to start the capture on the selected device.";
			goto bail;
		}
	}
	
bail:
	return S_OK;
}

typedef struct {
	// VITC timecodes and user bits for field 1 & 2
	string vitcF1Timecode;
	string vitcF1UserBits;
	string vitcF2Timecode;
	string vitcF2UserBits;
	
	// RP188 timecodes and user bits (VITC1, VITC2 and LTC)
	string rp188vitc1Timecode;
	string rp188vitc1UserBits;
	string rp188vitc2Timecode;
	string rp188vitc2UserBits;
	string rp188ltcTimecode;
	string rp188ltcUserBits;
} AncillaryDataStruct;

HRESULT DeckLinkController::VideoInputFrameArrived (/* in */ IDeckLinkVideoInputFrame* videoFrame, /* in */ IDeckLinkAudioInputPacket* audioPacket)  {
	if (videoFrame == NULL) {
		return S_OK;
	}
	signalPresent = (videoFrame->GetFlags() & bmdFrameHasNoInputSource) == 0;

	// Timecode, if the source sends any (RP188 first, then VITC)
	string tc, userBits;
	getAncillaryDataFromFrame(videoFrame, bmdTimecodeRP188Any, tc, userBits);
	if (tc.empty()) {
		getAncillaryDataFromFrame(videoFrame, bmdTimecodeVITC, tc, userBits);
	}
	{
		std::lock_guard<std::mutex> guard(timecodeMutex);
		timecode = tc;
	}

	long w = videoFrame->GetWidth();
	long h = videoFrame->GetHeight();
	frameWidth = w;
	frameHeight = h;

    // Using DeckLink SDK for colour conversion
    if (rgbaFrame == NULL) {
        rgbaFrame = new VideoFrame(w, h);
    }
    
    if (rgbaFrame->lock.try_lock_for(std::chrono::milliseconds(colorConversionTimeout))) {
		if (rgbaFrame->getWidth() != w || rgbaFrame->getHeight() != h) {
			rgbaFrame->allocate(w, h); // input format changed
		}
        videoConverter->ConvertFrame(videoFrame, rgbaFrame);
        rgbaFrame->lock.unlock();
    }
    else {
        ofLogVerbose("DeckLinkController") << "Skipped colour conversion, the RGBA frame is still locked";
    }

    // Raw data, sized from the frame itself so a format change can't overrun it
	void* bytes;
	videoFrame->GetBytes(&bytes);
	unsigned char* raw = (unsigned char*) bytes;
	size_t size = (size_t)videoFrame->GetRowBytes() * h;
	vector<unsigned char>& back = buffer.getBack();
	back.assign(raw, raw + size);
	buffer.swapBack();
	
	return S_OK;
}

string DeckLinkController::getTimecode() {
	std::lock_guard<std::mutex> guard(timecodeMutex);
	return timecode;
}

#ifdef TARGET_OSX
void DeckLinkController::getAncillaryDataFromFrame(IDeckLinkVideoInputFrame* videoFrame, BMDTimecodeFormat timecodeFormat, string& timecodeString, string& userBitsString)  {
    IDeckLinkTimecode* timecode = NULL;
    CFStringRef timecodeCFString;
    BMDTimecodeUserBits userBits = 0;
    
    if ((videoFrame != NULL)
        && (videoFrame->GetTimecode(timecodeFormat, &timecode) == S_OK)) {
        if (timecode->GetString(&timecodeCFString) == S_OK) {
            timecodeString = toString(timecodeCFString);
            CFRelease(timecodeCFString);
        }
        else {
            timecodeString = "";
        }
        
        timecode->GetTimecodeUserBits(&userBits);
        userBitsString = "0x" + ofToHex(userBits);
        
        timecode->Release();
    }
    else {
        timecodeString = "";
        userBitsString = "";
    }
}

#else
void DeckLinkController::getAncillaryDataFromFrame(IDeckLinkVideoInputFrame* videoFrame, BMDTimecodeFormat timecodeFormat, string& timecodeString, string& userBitsString)  {
    IDeckLinkTimecode* timecode = NULL;
    char * timecodeCFString;
    BMDTimecodeUserBits userBits = 0;
    
    if ((videoFrame != NULL)
        && (videoFrame->GetTimecode(timecodeFormat, &timecode) == S_OK)) {
        if (timecode->GetString((const char**)&timecodeCFString) == S_OK) {
            timecodeString = string(timecodeCFString);
            
        }
        else {
            timecodeString = "";
        }
        
        timecode->GetTimecodeUserBits(&userBits);
        userBitsString = "0x" + ofToHex(userBits);
        
        timecode->Release();
    }
    else {
        timecodeString = "";
        userBitsString = "";
    }
}
#endif


// picks the mode with matching resolution, with highest available framerate
// and a preference for progressive over interlaced
BMDDisplayMode DeckLinkController::getDisplayMode(int w, int h) {
	return getDisplayMode(w, h, -1);
}

BMDDisplayMode DeckLinkController::getDisplayMode(int w, int h, float framerate) {
	IDeckLinkDisplayMode* best = NULL;
	float bestRate = 0;
	bool bestProgressive = false;
	string available;
	for (auto m : modeList) {
		if (m->GetWidth() != w || m->GetHeight() != h) {
			continue;
		}
		BMDTimeValue duration;
		BMDTimeScale scale;
		m->GetFrameRate(&duration, &scale);
		float rate = duration > 0 ? float(scale) / float(duration) : 0;
		bool progressive = m->GetFieldDominance() == bmdProgressiveFrame || m->GetFieldDominance() == bmdProgressiveSegmentedFrame;
		available += ofToString(rate, 2) + (progressive ? "p " : "i ");
		if (framerate > 0) {
			// 23.98 / 29.97 / 59.94 are matched with a small tolerance
			if (fabs(rate - framerate) < 0.02f && (best == NULL || (progressive && !bestProgressive))) {
				best = m;
				bestProgressive = progressive;
			}
		}
		else if (best == NULL || (progressive && !bestProgressive) || (progressive == bestProgressive && rate > bestRate)) {
			best = m;
			bestRate = rate;
			bestProgressive = progressive;
		}
	}
	if (best == NULL) {
		if (available.empty()) {
			ofLogError("DeckLinkController") << "The device has no " << w << "x" << h << " mode";
		} else {
			ofLogError("DeckLinkController") << "No " << w << "x" << h << " mode at " << framerate << " fps, available: " << available;
		}
		return bmdModeUnknown;
	}
	return best->GetDisplayMode();
}
