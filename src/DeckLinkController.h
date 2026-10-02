/*
 DeckLinkController is a C++ only port of sample code from the DeckLink SDK
 that demonstrates how to get device info, start and stop a capture stream, and
 get video frame data from an input device. The only addition is a triple
 buffered video data member.
 */

#pragma once

#include "ofMain.h"

#include "DeckLinkAPI.h"
#include "TripleBuffer.h"
#include "VideoFrame.h"

class VideoFrame;

// SDK 11 renamed IDeckLinkAttributes; support both so newer SDK headers can be dropped in.
#if defined(BLACKMAGIC_DECKLINK_API_VERSION) && BLACKMAGIC_DECKLINK_API_VERSION >= 0x0b000000
typedef IDeckLinkProfileAttributes DeckLinkAttributesInterface;
#define DECKLINK_ATTRIBUTES_IID IID_IDeckLinkProfileAttributes
#else
typedef IDeckLinkAttributes DeckLinkAttributesInterface;
#define DECKLINK_ATTRIBUTES_IID IID_IDeckLinkAttributes
#endif

class DeckLinkController : public IDeckLinkInputCallback {
private:
	vector<IDeckLink*> deviceList;
	IDeckLink* selectedDevice;
	IDeckLinkInput* deckLinkInput;
	vector<IDeckLinkDisplayMode*> modeList;
	
	bool supportFormatDetection;
	bool currentlyCapturing;
	int selectedIndex;
    
    IDeckLinkVideoConversion *videoConverter;
    int colorConversionTimeout;

	// Size of the incoming frames; changes when the input format changes
	std::atomic<int> frameWidth, frameHeight;
	std::atomic<bool> signalPresent;
	std::mutex timecodeMutex;
	string timecode;
	
	void getAncillaryDataFromFrame(IDeckLinkVideoInputFrame* frame, BMDTimecodeFormat format, string& timecodeString, string& userBitsString);
	IDeckLinkDisplayMode* findMode(BMDDisplayMode mode);
	
public:
	TripleBuffer< vector<unsigned char> > buffer;
	
	DeckLinkController();
	virtual ~DeckLinkController();
	
	bool init();
	
	int getDeviceCount();
	vector<string> getDeviceNameList();
	
	bool selectDevice(int index);
    void setColorConversionTimeout(int ms);
	
	vector<string> getDisplayModeNames();
	bool isFormatDetectionEnabled();
	bool isCapturing();

	unsigned long getDisplayModeBufferSize(BMDDisplayMode mode);

	bool startCaptureWithMode(BMDDisplayMode videoMode);
	bool startCaptureWithIndex(int videoModeIndex);
	void stopCapture();
    
	virtual HRESULT QueryInterface (REFIID iid, LPVOID *ppv) {return E_NOINTERFACE;}
	virtual ULONG AddRef () {return 1;}
	virtual ULONG Release () {return 1;}
    
	virtual HRESULT VideoInputFormatChanged (/* in */ BMDVideoInputFormatChangedEvents notificationEvents, /* in */ IDeckLinkDisplayMode *newDisplayMode, /* in */ BMDDetectedVideoInputFormatFlags detectedSignalFlags);
	virtual HRESULT VideoInputFrameArrived (/* in */ IDeckLinkVideoInputFrame* videoFrame, /* in */ IDeckLinkAudioInputPacket* audioPacket);
    
	// Looks the mode up in the modes the selected device reports, so every
	// mode the hardware supports works. framerate <= 0 picks the highest
	// progressive rate (interlaced only if there is no progressive mode).
	BMDDisplayMode getDisplayMode(int w, int h);
	BMDDisplayMode getDisplayMode(int w, int h, float framerate);
	bool getDisplayModeInfo(BMDDisplayMode mode, int& w, int& h, float& framerate, string& name);

	int getFrameWidth() { return frameWidth; }
	int getFrameHeight() { return frameHeight; }
	bool hasSignal() { return signalPresent; }
	string getTimecode();
	int getSelectedDeviceIndex() { return selectedIndex; }
    
    VideoFrame *rgbaFrame;
};