#include "DeckLinkController.h"

#ifdef _WIN32
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#endif

//--------------------------------------------------------------
// Platform helpers: strings, flags and object creation differ between the
// macOS, Linux and Windows versions of the SDK.
namespace {
#if defined(_WIN32)
	typedef BSTR DLString;
	std::string toString(DLString s) {
		if (s == NULL) return "";
		const int n = WideCharToMultiByte(CP_UTF8, 0, s, -1, NULL, 0, NULL, NULL);
		if (n <= 1) return "";
		std::string out(n - 1, '\0');
		WideCharToMultiByte(CP_UTF8, 0, s, -1, &out[0], n, NULL, NULL);
		return out;
	}
	void freeString(DLString s) { SysFreeString(s); }
#elif defined(__APPLE__)
	typedef CFStringRef DLString;
	std::string toString(DLString s) {
		// CFStringGetCStringPtr may return NULL, so copy into a buffer instead
		char buffer[512];
		if (s != NULL && CFStringGetCString(s, buffer, sizeof(buffer), kCFStringEncodingUTF8)) {
			return std::string(buffer);
		}
		return "";
	}
	void freeString(DLString s) { if (s) CFRelease(s); }
#else
	typedef const char* DLString;
	std::string toString(DLString s) { return s ? std::string(s) : std::string(); }
	void freeString(DLString s) { free((void*)s); }
#endif

	IDeckLinkIterator* createIterator() {
#ifdef _WIN32
		IDeckLinkIterator* it = NULL;
		if (CoCreateInstance(CLSID_CDeckLinkIterator, NULL, CLSCTX_ALL, IID_IDeckLinkIterator, (void**)&it) != S_OK) return NULL;
		return it;
#else
		return CreateDeckLinkIteratorInstance();
#endif
	}

	IDeckLinkVideoConversion* createConverter() {
#ifdef _WIN32
		IDeckLinkVideoConversion* c = NULL;
		if (CoCreateInstance(CLSID_CDeckLinkVideoConversion, NULL, CLSCTX_ALL, IID_IDeckLinkVideoConversion, (void**)&c) != S_OK) return NULL;
		return c;
#else
		return CreateVideoConversionInstance();
#endif
	}

	std::string displayName(IDeckLink* device) {
		DLString name = NULL;
		if (device->GetDisplayName(&name) == S_OK) {
			std::string s = toString(name);
			freeString(name);
			return s;
		}
		return "DeckLink";
	}

	std::string modeName(IDeckLinkDisplayMode* mode) {
		DLString name = NULL;
		if (mode->GetName(&name) == S_OK) {
			std::string s = toString(name);
			freeString(name);
			return s;
		}
		return "Unknown mode";
	}

	float modeFrameRate(IDeckLinkDisplayMode* mode) {
		BMDTimeValue duration;
		BMDTimeScale scale;
		mode->GetFrameRate(&duration, &scale);
		return duration > 0 ? float(scale) / float(duration) : 0;
	}

	void releaseFrame(DeckLinkRawFrame& f) {
		if (f.frame != NULL) {
			f.frame->Release();
			f.frame = NULL;
		}
	}
}

bool DeckLinkController::readFrame(IDeckLinkVideoFrame* frame, const std::function<void(const unsigned char*)>& reader) {
	if (frame == NULL) return false;
	IDeckLinkVideoBuffer* videoBuffer = NULL;
	if (frame->QueryInterface(IID_IDeckLinkVideoBuffer, (void**)&videoBuffer) != S_OK || videoBuffer == NULL) return false;
	bool ok = false;
	if (videoBuffer->StartAccess(bmdBufferAccessRead) == S_OK) {
		void* bytes = NULL;
		if (videoBuffer->GetBytes(&bytes) == S_OK && bytes != NULL) {
			reader(static_cast<const unsigned char*>(bytes));
			ok = true;
		}
		videoBuffer->EndAccess(bmdBufferAccessRead);
	}
	videoBuffer->Release();
	return ok;
}

//--------------------------------------------------------------
DeckLinkController::DeckLinkController()
	: selectedDevice(NULL)
	, deckLinkInput(NULL)
	, supportFormatDetection(false)
	, currentlyCapturing(false)
	, selectedIndex(-1)
	, comInitialized(false)
	, requestedPixelFormat(bmdFormat8BitYUV)
	, videoConverter(NULL)
	, frameWidth(0)
	, frameHeight(0)
	, signalPresent(false)
	, currentDisplayMode(bmdModeUnknown)
	, currentPixelFormat(bmdFormat8BitYUV)
	, framesArrived(0)
	, framesSkipped(0) {
}

DeckLinkController::~DeckLinkController() {
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
	releaseDevices();
	releaseFrames();
	if (videoConverter) {
		videoConverter->Release();
	}
#ifdef _WIN32
	if (comInitialized) CoUninitialize();
#endif
}

void DeckLinkController::releaseFrames() {
	buffer.forEach([](DeckLinkRawFrame& f) { releaseFrame(f); });
}

void DeckLinkController::releaseDevices() {
	for (auto device : deviceList) {
		device->Release();
	}
	deviceList.clear();
}

bool DeckLinkController::init() {
#ifdef _WIN32
	if (!comInitialized) {
		// The DeckLink API is COM on Windows. Any apartment works for us.
		const HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if (SUCCEEDED(hr)) {
			comInitialized = true; // S_OK or S_FALSE: balance with CoUninitialize
		} else if (hr != RPC_E_CHANGED_MODE) {
			ofLogError("DeckLinkController") << "COM initialisation failed";
			return false;
		}
	}
#endif
	IDeckLinkIterator* deckLinkIterator = createIterator();
	if (deckLinkIterator == NULL) {
		ofLogError("DeckLinkController") << "Please install Blackmagic Desktop Video (16.0 or newer) to use the DeckLink devices.";
		return false;
	}

	// List all DeckLink devices (init() may be called again, e.g. by listDevices())
	if (currentlyCapturing) stopCapture();
	if (deckLinkInput != NULL) {
		deckLinkInput->Release();
		deckLinkInput = NULL;
	}
	while (modeList.size() > 0) {
		modeList.back()->Release();
		modeList.pop_back();
	}
	selectedDevice = NULL;
	selectedIndex = -1;
	releaseDevices();
	IDeckLink* deckLink = NULL;
	while (deckLinkIterator->Next(&deckLink) == S_OK) {
		deviceList.push_back(deckLink);
	}
	deckLinkIterator->Release();

	if (deviceList.empty()) {
		ofLogError("DeckLinkController") << "No Blackmagic device found.";
		return false;
	}

	if (videoConverter == NULL) {
		videoConverter = createConverter();
		if (videoConverter == NULL) {
			ofLogWarning("DeckLinkController") << "No video converter available: CPU colour pixels are not available (the GPU conversion still works).";
		}
	}
	return true;
}

int DeckLinkController::getDeviceCount() {
	return int(deviceList.size());
}

std::vector<std::string> DeckLinkController::getDeviceNameList() {
	std::vector<std::string> nameList;
	for (auto device : deviceList) {
		nameList.push_back(displayName(device));
	}
	return nameList;
}

std::vector<DeckLinkDeviceInfo> DeckLinkController::getDeviceInfoList() {
	std::vector<DeckLinkDeviceInfo> infos;
	for (size_t i = 0; i < deviceList.size(); i++) {
		DeckLinkDeviceInfo info;
		info.index = int(i);
		info.displayName = displayName(deviceList[i]);
		DLString model = NULL;
		if (deviceList[i]->GetModelName(&model) == S_OK) {
			info.modelName = toString(model);
			freeString(model);
		}
		IDeckLinkProfileAttributes* attributes = NULL;
		if (deviceList[i]->QueryInterface(IID_IDeckLinkProfileAttributes, (void**)&attributes) == S_OK) {
			DLInt value = 0;
			DLBool flag = false;
			if (attributes->GetInt(BMDDeckLinkPersistentID, &value) == S_OK) info.persistentId = value;
			if (attributes->GetInt(BMDDeckLinkSubDeviceIndex, &value) == S_OK) info.subDeviceIndex = int(value);
			if (attributes->GetInt(BMDDeckLinkNumberOfSubDevices, &value) == S_OK) info.numSubDevices = int(value);
			if (attributes->GetInt(BMDDeckLinkVideoIOSupport, &value) == S_OK) {
				info.canCapture = (value & bmdDeviceSupportsCapture) != 0;
				info.canPlayback = (value & bmdDeviceSupportsPlayback) != 0;
			}
			if (attributes->GetInt(BMDDeckLinkDeviceInterface, &value) == S_OK) {
				info.interfaceName = value == bmdDeviceInterfacePCI ? "PCIe"
					: value == bmdDeviceInterfaceUSB ? "USB"
					: value == bmdDeviceInterfaceThunderbolt ? "Thunderbolt" : "";
			}
			if (attributes->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &flag) == S_OK) info.supportsFormatDetection = flag != 0;
			if (attributes->GetInt(BMDDeckLinkVideoInputConnections, &value) == S_OK) {
				for (BMDVideoConnection c : { bmdVideoConnectionSDI, bmdVideoConnectionHDMI, bmdVideoConnectionOpticalSDI, bmdVideoConnectionComponent, bmdVideoConnectionComposite, bmdVideoConnectionSVideo }) {
					if (value & c) info.inputConnections.push_back(c);
				}
			}
			attributes->Release();
		}
		infos.push_back(info);
	}
	return infos;
}

bool DeckLinkController::selectDevice(int index) {
	if (index < 0 || index >= int(deviceList.size())) {
		ofLogError("DeckLinkController") << "No device " << index << " (" << deviceList.size() << " found)";
		return false;
	}

	// Release the previous device and mode list
	if (currentlyCapturing) stopCapture();
	if (deckLinkInput != NULL) {
		deckLinkInput->Release();
		deckLinkInput = NULL;
	}
	while (modeList.size() > 0) {
		modeList.back()->Release();
		modeList.pop_back();
	}

	if (deviceList[index]->QueryInterface(IID_IDeckLinkInput, (void**)&deckLinkInput) != S_OK) {
		ofLogError("DeckLinkController") << displayName(deviceList[index]) << " has no input (output only device?)";
		deckLinkInput = NULL;
		return false;
	}

	// Cache the mode list
	IDeckLinkDisplayModeIterator* displayModeIterator = NULL;
	if (deckLinkInput->GetDisplayModeIterator(&displayModeIterator) == S_OK) {
		IDeckLinkDisplayMode* displayMode = NULL;
		while (displayModeIterator->Next(&displayMode) == S_OK) {
			modeList.push_back(displayMode);
		}
		displayModeIterator->Release();
	}

	// Input format detection
	supportFormatDetection = false;
	IDeckLinkProfileAttributes* attributes = NULL;
	if (deviceList[index]->QueryInterface(IID_IDeckLinkProfileAttributes, (void**)&attributes) == S_OK) {
		DLBool flag = false;
		if (attributes->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &flag) == S_OK) supportFormatDetection = flag != 0;
		attributes->Release();
	}

	selectedDevice = deviceList[index];
	selectedIndex = index;
	return true;
}

//--------------------------------------------------------------
std::vector<BMDVideoConnection> DeckLinkController::getInputConnections() {
	std::vector<BMDVideoConnection> connections;
	if (selectedIndex < 0) return connections;
	for (auto& info : getDeviceInfoList()) {
		if (info.index == selectedIndex) connections = info.inputConnections;
	}
	return connections;
}

bool DeckLinkController::setInputConnection(BMDVideoConnection connection) {
	if (selectedDevice == NULL) {
		ofLogError("DeckLinkController") << "setInputConnection: select a device first";
		return false;
	}
	IDeckLinkConfiguration* config = NULL;
	if (selectedDevice->QueryInterface(IID_IDeckLinkConfiguration, (void**)&config) != S_OK || config == NULL) return false;
	const bool ok = config->SetInt(bmdDeckLinkConfigVideoInputConnection, connection) == S_OK;
	config->Release();
	if (!ok) {
		ofLogError("DeckLinkController") << "this device has no " << connectionName(connection) << " input";
	}
	return ok;
}

BMDVideoConnection DeckLinkController::getInputConnection() {
	if (selectedDevice == NULL) return bmdVideoConnectionUnspecified;
	IDeckLinkConfiguration* config = NULL;
	if (selectedDevice->QueryInterface(IID_IDeckLinkConfiguration, (void**)&config) != S_OK || config == NULL) return bmdVideoConnectionUnspecified;
	DLInt value = 0;
	config->GetInt(bmdDeckLinkConfigVideoInputConnection, &value);
	config->Release();
	return BMDVideoConnection(value);
}

std::vector<DeckLinkProfileInfo> DeckLinkController::getProfiles(int deviceIndex) {
	std::vector<DeckLinkProfileInfo> profiles;
	if (deviceIndex < 0 || deviceIndex >= int(deviceList.size())) return profiles;
	IDeckLinkProfileManager* manager = NULL;
	if (deviceList[deviceIndex]->QueryInterface(IID_IDeckLinkProfileManager, (void**)&manager) != S_OK || manager == NULL) {
		return profiles; // single profile device
	}
	IDeckLinkProfileIterator* it = NULL;
	if (manager->GetProfiles(&it) == S_OK && it != NULL) {
		IDeckLinkProfile* profile = NULL;
		while (it->Next(&profile) == S_OK) {
			DeckLinkProfileInfo info;
			DLBool active = false;
			if (profile->IsActive(&active) == S_OK) info.active = active != 0;
			IDeckLinkProfileAttributes* attributes = NULL;
			if (profile->QueryInterface(IID_IDeckLinkProfileAttributes, (void**)&attributes) == S_OK) {
				DLInt id = 0;
				if (attributes->GetInt(BMDDeckLinkProfileID, &id) == S_OK) info.id = BMDProfileID(id);
				attributes->Release();
			}
			info.name = profileName(info.id);
			profiles.push_back(info);
			profile->Release();
		}
		it->Release();
	}
	manager->Release();
	return profiles;
}

bool DeckLinkController::setProfile(int deviceIndex, BMDProfileID profileId) {
	if (deviceIndex < 0 || deviceIndex >= int(deviceList.size())) return false;
	IDeckLinkProfileManager* manager = NULL;
	if (deviceList[deviceIndex]->QueryInterface(IID_IDeckLinkProfileManager, (void**)&manager) != S_OK || manager == NULL) {
		ofLogError("DeckLinkController") << "this device has only one profile";
		return false;
	}
	bool ok = false;
	IDeckLinkProfile* profile = NULL;
	if (manager->GetProfile(profileId, &profile) == S_OK && profile != NULL) {
		ok = profile->SetActive() == S_OK;
		profile->Release();
	}
	manager->Release();
	if (ok) {
		ofLogNotice("DeckLinkController") << "profile " << profileName(profileId) << " activated: list the devices again";
	} else {
		ofLogError("DeckLinkController") << "could not activate profile " << profileName(profileId) << " (is the device in use?)";
	}
	return ok;
}

std::string DeckLinkController::connectionName(BMDVideoConnection connection) {
	switch (connection) {
	case bmdVideoConnectionSDI: return "SDI";
	case bmdVideoConnectionHDMI: return "HDMI";
	case bmdVideoConnectionOpticalSDI: return "Optical SDI";
	case bmdVideoConnectionComponent: return "Component";
	case bmdVideoConnectionComposite: return "Composite";
	case bmdVideoConnectionSVideo: return "S-Video";
	default: return "unspecified";
	}
}

std::string DeckLinkController::profileName(BMDProfileID profile) {
	switch (profile) {
	case bmdProfileOneSubDeviceFullDuplex: return "1 sub-device, full duplex";
	case bmdProfileOneSubDeviceHalfDuplex: return "1 sub-device, half duplex";
	case bmdProfileTwoSubDevicesFullDuplex: return "2 sub-devices, full duplex";
	case bmdProfileTwoSubDevicesHalfDuplex: return "2 sub-devices, half duplex";
	case bmdProfileFourSubDevicesHalfDuplex: return "4 sub-devices, half duplex";
	default: return "unknown profile";
	}
}

std::string DeckLinkController::pixelFormatName(BMDPixelFormat format) {
	switch (format) {
	case bmdFormat8BitYUV: return "8 bit YUV 4:2:2";
	case bmdFormat10BitYUV: return "10 bit YUV 4:2:2";
	case bmdFormat8BitARGB: return "8 bit RGB 4:4:4";
	case bmdFormat10BitRGB: return "10 bit RGB 4:4:4";
	default: return "other";
	}
}

std::vector<std::string> DeckLinkController::getDisplayModeNames() {
	std::vector<std::string> modeNames;
	for (auto mode : modeList) {
		modeNames.push_back(modeName(mode));
	}
	return modeNames;
}

bool DeckLinkController::isFormatDetectionEnabled() {
	return supportFormatDetection;
}

bool DeckLinkController::isCapturing() {
	return currentlyCapturing;
}

IDeckLinkDisplayMode* DeckLinkController::findMode(BMDDisplayMode mode) {
	for (auto m : modeList) {
		if (m->GetDisplayMode() == mode) return m;
	}
	return NULL;
}

bool DeckLinkController::getDisplayModeInfo(BMDDisplayMode mode, int& w, int& h, float& framerate, std::string& name) {
	IDeckLinkDisplayMode* m = findMode(mode);
	if (m == NULL) return false;
	w = int(m->GetWidth());
	h = int(m->GetHeight());
	framerate = modeFrameRate(m);
	name = modeName(m);
	return true;
}

bool DeckLinkController::startCaptureWithIndex(int videoModeIndex) {
	if (videoModeIndex < 0 || videoModeIndex >= int(modeList.size())) {
		ofLogError("DeckLinkController") << "An invalid display mode was selected.";
		return false;
	}
	return startCaptureWithMode(modeList[videoModeIndex]->GetDisplayMode());
}

bool DeckLinkController::enableInput(BMDDisplayMode mode, BMDPixelFormat pixelFormat) {
	const BMDVideoInputFlags flags = supportFormatDetection ? bmdVideoInputEnableFormatDetection : bmdVideoInputFlagDefault;
	if (deckLinkInput->EnableVideoInput(mode, pixelFormat, flags) != S_OK) return false;
	currentDisplayMode = uint32_t(mode);
	currentPixelFormat = uint32_t(pixelFormat);
	return true;
}

bool DeckLinkController::startCaptureWithMode(BMDDisplayMode videoMode) {
	if (deckLinkInput == NULL) {
		ofLogError("DeckLinkController") << "No device selected.";
		return false;
	}
	IDeckLinkDisplayMode* m = findMode(videoMode);
	if (m == NULL) {
		ofLogError("DeckLinkController") << "The selected device does not support that display mode.";
		return false;
	}
	frameWidth = int(m->GetWidth());
	frameHeight = int(m->GetHeight());
	releaseFrames();
	buffer.setup(DeckLinkRawFrame());
	framesArrived = 0;
	framesSkipped = 0;

	{
		std::lock_guard<std::mutex> guard(signalMutex);
		signal = DeckLinkSignalInfo();
		signalWarning.clear();
	}
	deckLinkInput->SetCallback(this);
	if (!enableInput(videoMode, requestedPixelFormat)) {
		std::string name;
		int w, h;
		float rate;
		getDisplayModeInfo(videoMode, w, h, rate, name);
		ofLogError("DeckLinkController") << "Unable to capture " << name << " as " << pixelFormatName(requestedPixelFormat)
										 << ". Is the format supported by the device, or the device in use by another application?";
		return false;
	}
	if (deckLinkInput->StartStreams() != S_OK) {
		ofLogError("DeckLinkController") << "Unable to start the capture. Is the device in use by another application?";
		deckLinkInput->DisableVideoInput();
		return false;
	}
	currentlyCapturing = true;
	return true;
}

void DeckLinkController::stopCapture() {
	if (deckLinkInput == NULL) return;
	deckLinkInput->StopStreams();
	deckLinkInput->DisableVideoInput();
	deckLinkInput->SetCallback(NULL);
	currentlyCapturing = false;
	releaseFrames();
}

//--------------------------------------------------------------
// Called by the SDK (on its own thread) when format detection sees a new signal.
// The capture keeps the mode and pixel format it was set to; this only records what
// the signal is and warns when it doesn't match.
HRESULT DeckLinkController::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents notificationEvents, IDeckLinkDisplayMode* newMode, BMDDetectedVideoInputFormatFlags detectedSignalFlags) {
	if (newMode == NULL) return S_OK;
	std::string warning;
	{
		std::lock_guard<std::mutex> guard(signalMutex);
		signal.detected = true;
		signal.displayMode = newMode->GetDisplayMode();
		signal.modeName = modeName(newMode);
		signal.width = int(newMode->GetWidth());
		signal.height = int(newMode->GetHeight());
		signal.frameRate = modeFrameRate(newMode);
		signal.rgb = (detectedSignalFlags & bmdDetectedVideoInputRGB444) != 0;
		signal.bitDepth = (detectedSignalFlags & bmdDetectedVideoInput12BitDepth) ? 12
			: (detectedSignalFlags & bmdDetectedVideoInput10BitDepth)            ? 10
			: (detectedSignalFlags & bmdDetectedVideoInput8BitDepth)             ? 8
																				  : 0;
		updateSignalWarning();
		warning = signalWarning;
	}
	if (!warning.empty()) {
		ofLogWarning("DeckLinkController") << warning;
	} else {
		ofLogNotice("DeckLinkController") << "signal: " << getSignal().describe() << " (matches the capture settings)";
	}
	return S_OK;
}

void DeckLinkController::updateSignalWarning() {
	signalWarning.clear();
	if (!signal.detected) {
		if (currentlyCapturing && framesArrived > 0 && !signal.present) {
			signalWarning = "no picture in the selected mode: check the camera's output format (this device can't report what it is)";
		}
		return;
	}
	std::string capture, settingName;
	int w, h;
	float rate;
	getDisplayModeInfo(getCurrentDisplayMode(), w, h, rate, settingName);
	capture = settingName + " " + ofToString(w) + "x" + ofToString(h) + ", " + pixelFormatName(requestedPixelFormat);
	if (signal.displayMode != getCurrentDisplayMode()) {
		signalWarning = "the signal is " + signal.describe() + " but the capture is set to " + capture + ": no picture until they match";
	} else if (signal.rgb != isRGB(requestedPixelFormat)) {
		signalWarning = "the signal is " + signal.describe() + ", the capture is set to " + capture + ": the device converts the colour format";
	} else if (signal.bitDepth != 0 && signal.bitDepth != bitDepth(requestedPixelFormat)) {
		signalWarning = "the signal is " + signal.describe() + ", the capture is set to " + capture
			+ (signal.bitDepth > bitDepth(requestedPixelFormat) ? ": precision is lost" : ": the extra bits carry no information");
	}
}

DeckLinkSignalInfo DeckLinkController::getSignal() {
	std::lock_guard<std::mutex> guard(signalMutex);
	return signal;
}

std::string DeckLinkController::getSignalWarning() {
	std::lock_guard<std::mutex> guard(signalMutex);
	return signalWarning;
}

std::string DeckLinkSignalInfo::describe() const {
	if (!detected) return present ? "a signal (format unknown)" : "no signal";
	return modeName + " " + ofToString(width) + "x" + ofToString(height) + ", " + (bitDepth ? ofToString(bitDepth) + " bit " : std::string())
		+ (rgb ? "RGB 4:4:4" : "YUV 4:2:2");
}

bool DeckLinkController::isRGB(BMDPixelFormat format) {
	return format == bmdFormat8BitARGB || format == bmdFormat8BitBGRA || format == bmdFormat10BitRGB || format == bmdFormat12BitRGB
		|| format == bmdFormat10BitRGBX;
}

int DeckLinkController::bitDepth(BMDPixelFormat format) {
	switch (format) {
	case bmdFormat10BitYUV:
	case bmdFormat10BitRGB:
	case bmdFormat10BitRGBX: return 10;
	case bmdFormat12BitRGB: return 12;
	default: return 8;
	}
}

HRESULT DeckLinkController::VideoInputFrameArrived(IDeckLinkVideoInputFrame* videoFrame, IDeckLinkAudioInputPacket* audioPacket) {
	if (videoFrame == NULL) return S_OK;
	framesArrived++;
	const bool present = (videoFrame->GetFlags() & bmdFrameHasNoInputSource) == 0;
	if (present != signalPresent || framesArrived == 1) {
		signalPresent = present;
		std::string warning;
		{
			std::lock_guard<std::mutex> guard(signalMutex);
			signal.present = present;
			updateSignalWarning();
			warning = signalWarning;
		}
		if (!present && !warning.empty()) ofLogWarning("DeckLinkController") << warning;
	}

	// Timecode, if the source sends any (RP188 first, then VITC)
	std::string tc, userBits;
	getAncillaryDataFromFrame(videoFrame, bmdTimecodeRP188Any, tc, userBits);
	if (tc.empty()) {
		getAncillaryDataFromFrame(videoFrame, bmdTimecodeVITC, tc, userBits);
	}
	{
		std::lock_guard<std::mutex> guard(timecodeMutex);
		timecode = tc;
	}

	const int w = int(videoFrame->GetWidth());
	const int h = int(videoFrame->GetHeight());
	frameWidth = w;
	frameHeight = h;

	// No copy: keep the SDK frame until the app has moved on to a newer one
	DeckLinkRawFrame& back = buffer.getBack();
	releaseFrame(back); // the app never reads the back slot
	videoFrame->AddRef();
	back.frame = videoFrame;
	back.width = w;
	back.height = h;
	back.rowBytes = int(videoFrame->GetRowBytes());
	back.pixelFormat = videoFrame->GetPixelFormat();
	back.number = framesArrived;
	if (buffer.swapBack()) {
		framesSkipped++;
	}
	return S_OK;
}

bool DeckLinkController::convertToRGBA(IDeckLinkVideoFrame* frame, ofPixels& rgba) {
	if (frame == NULL) return false;
	const int w = int(frame->GetWidth()), h = int(frame->GetHeight());
	IDeckLinkVideoFrame* argb = frame;
	bool converted = false;
	if (frame->GetPixelFormat() != bmdFormat8BitARGB) {
		if (videoConverter == NULL) return false;
		argb = NULL;
		const BMDColorspace colorspace = h <= 576 ? bmdColorspaceRec601 : bmdColorspaceRec709;
		if (videoConverter->ConvertNewFrame(frame, bmdFormat8BitARGB, colorspace, NULL, &argb) != S_OK || argb == NULL) return false;
		converted = true;
	}
	const int rowBytes = int(argb->GetRowBytes());
	rgba.allocate(w, h, OF_PIXELS_RGBA);
	const bool ok = readFrame(argb, [&](const unsigned char* bytes) {
		unsigned char* dst = rgba.getData();
		for (int y = 0; y < h; y++) {
			const unsigned char* src = bytes + size_t(y) * rowBytes;
			for (int x = 0; x < w; x++, src += 4, dst += 4) {
				dst[0] = src[1];
				dst[1] = src[2];
				dst[2] = src[3];
				dst[3] = src[0];
			}
		}
	});
	if (converted) argb->Release();
	return ok;
}

std::string DeckLinkController::getTimecode() {
	std::lock_guard<std::mutex> guard(timecodeMutex);
	return timecode;
}

void DeckLinkController::getAncillaryDataFromFrame(IDeckLinkVideoInputFrame* videoFrame, BMDTimecodeFormat timecodeFormat, std::string& timecodeString, std::string& userBitsString) {
	IDeckLinkTimecode* tc = NULL;
	timecodeString = "";
	userBitsString = "";
	if (videoFrame == NULL || videoFrame->GetTimecode(timecodeFormat, &tc) != S_OK || tc == NULL) return;
	DLString s = NULL;
	if (tc->GetString(&s) == S_OK) {
		timecodeString = toString(s);
		freeString(s);
	}
	BMDTimecodeUserBits userBits = 0;
	tc->GetTimecodeUserBits(&userBits);
	userBitsString = "0x" + ofToHex(userBits);
	tc->Release();
}

//--------------------------------------------------------------
// picks the mode with matching resolution, with highest available framerate
// and a preference for progressive over interlaced
BMDDisplayMode DeckLinkController::getDisplayMode(int w, int h) {
	return getDisplayMode(w, h, -1);
}

BMDDisplayMode DeckLinkController::getDisplayMode(int w, int h, float framerate) {
	IDeckLinkDisplayMode* best = NULL;
	float bestRate = 0;
	bool bestProgressive = false;
	std::string available;
	for (auto m : modeList) {
		if (m->GetWidth() != w || m->GetHeight() != h) continue;
		const float rate = modeFrameRate(m);
		const bool progressive = m->GetFieldDominance() == bmdProgressiveFrame || m->GetFieldDominance() == bmdProgressiveSegmentedFrame;
		available += ofToString(rate, 2) + (progressive ? "p " : "i ");
		if (framerate > 0) {
			// 23.98 / 29.97 / 59.94 are matched with a small tolerance
			if (fabs(rate - framerate) < 0.02f && (best == NULL || (progressive && !bestProgressive))) {
				best = m;
				bestProgressive = progressive;
			}
		} else if (best == NULL || (progressive && !bestProgressive) || (progressive == bestProgressive && rate > bestRate)) {
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
