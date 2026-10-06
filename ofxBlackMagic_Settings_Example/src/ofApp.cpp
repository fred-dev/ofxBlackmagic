#include "ofApp.h"

void ofApp::setup() {
	ofSetVerticalSync(true);
	ofBackground(0);
	devices = cam.listDeviceInfo();
	cam.listDevices(); // logs a summary
	open();
}

void ofApp::open() {
	cam.close();
	if(devices.empty()) {
		message = "no Blackmagic device";
		return;
	}
	profiles = cam.listProfiles(devices[device].index);
	cam.setDeviceID(devices[device].index);
	cam.setPixelFormat(pixelFormat);
	cam.setUseGpuConversion(gpu);
	cam.setInputConnection(connection);
	const bool ok = mode == bmdModeUnknown ? cam.setup(1920, 1080) : cam.setup(mode, devices[device].index);
	message = ok ? "" : "could not start " + devices[device].displayName + " (see the log)";
}

void ofApp::update() {
	if(cam.update()) {
		const float now = ofGetElapsedTimef();
		if(lastFrameTime > 0) captureFps = ofLerp(1.f / (now - lastFrameTime), captureFps, 0.9f);
		lastFrameTime = now;
	}
}

void ofApp::draw() {
	ofSetColor(255);
	if(cam.getWidth() > 0) {
		ofRectangle r(0, 0, cam.getWidth(), cam.getHeight());
		r.scaleTo(ofRectangle(0, 0, ofGetWidth(), ofGetHeight()));
		cam.draw(r.x, r.y, r.width, r.height); // colour, converted on the GPU (or CPU with 'g')
	}

	std::stringstream ss;
	if(!devices.empty()) {
		const auto & d = devices[device];
		ss << d.displayName << (d.modelName.empty() ? "" : " (" + d.modelName + ")") << "  " << d.interfaceName
		   << (d.persistentId ? "  id " + ofToHex(d.persistentId) : "") << "\n";
		ss << "input: " << ofxBlackMagic::getConnectionName(cam.getInputConnection()) << "   available:";
		for(auto c : cam.getInputConnections()) ss << " " << ofxBlackMagic::getConnectionName(c);
		ss << "\n";
		if(profiles.size() > 1) {
			ss << "profiles:";
			for(auto & p : profiles) ss << (p.active ? "  [" : "  ") << p.name << (p.active ? "]" : "");
			ss << "\n";
		}
	}
	ss << "capture: " << cam.getDisplayModeName() << " " << cam.getWidth() << "x" << cam.getHeight() << ", "
	   << ofxBlackMagic::getPixelFormatName(cam.getPixelFormat()) << ", colour on the " << (gpu ? "GPU" : "CPU") << "\n";
	ss << "signal:  " << cam.getSignal().describe() << "\n";
	ss << "frames: " << cam.getFramesArrived() << " arrived, " << cam.getFramesSkipped() << " skipped   capture fps " << int(captureFps)
	   << "   app fps " << int(ofGetFrameRate()) << "\n";
	if(!cam.getTimecode().empty()) ss << "timecode: " << cam.getTimecode() << "\n";
	if(!message.empty()) ss << message << "\n";
	ss << "\nd device  i input  t pixel format  m capture = signal  g GPU/CPU  p profile  f fullscreen";
	ofDrawBitmapStringHighlight(ss.str(), 10, 20);

	const string warning = cam.getSignalWarning();
	if(!warning.empty()) {
		ofDrawBitmapStringHighlight("WARNING: " + warning + "\npress m to set the capture to the signal", 10, ofGetHeight() - 30, ofColor(150, 0, 0), ofColor(255));
	}
}

void ofApp::exit() {
	cam.close();
}

void ofApp::keyPressed(int key) {
	if(key == 'd' && !devices.empty()) {
		device = (device + 1) % devices.size();
		connection = bmdVideoConnectionUnspecified;
		mode = bmdModeUnknown;
		open();
	}
	if(key == 'i') {
		auto inputs = cam.getInputConnections();
		if(!inputs.empty()) {
			auto it = std::find(inputs.begin(), inputs.end(), cam.getInputConnection());
			connection = (it == inputs.end() || it + 1 == inputs.end()) ? inputs.front() : *(it + 1);
			cam.setInputConnection(connection); // switches while capturing
		}
	}
	if(key == 't') {
		static const BMDPixelFormat formats[] = { bmdFormat8BitYUV, bmdFormat10BitYUV, bmdFormat8BitARGB, bmdFormat10BitRGB };
		int i = 0;
		while(i < 4 && formats[i] != pixelFormat) i++;
		pixelFormat = formats[(i + 1) % 4];
		open();
	}
	if(key == 'm') {
		// set the capture to what is arriving (a deliberate choice, the capture never does it by itself)
		const auto signal = cam.getSignal();
		if(signal.detected) {
			mode = signal.displayMode;
			pixelFormat = signal.rgb ? (signal.bitDepth >= 10 ? bmdFormat10BitRGB : bmdFormat8BitARGB)
									 : (signal.bitDepth >= 10 ? bmdFormat10BitYUV : bmdFormat8BitYUV);
			open();
		} else {
			message = "this device doesn't report the signal format: set the mode by hand";
		}
	}
	if(key == 'g') {
		gpu = !gpu;
		cam.setUseGpuConversion(gpu);
	}
	if(key == 'p' && !devices.empty()) {
		if(profiles.size() > 1) {
			auto it = std::find_if(profiles.begin(), profiles.end(), [](const DeckLinkProfileInfo & p) { return p.active; });
			auto next = (it == profiles.end() || it + 1 == profiles.end()) ? profiles.front() : *(it + 1);
			cam.close();
			if(cam.setProfile(devices[device].index, next.id)) {
				ofSleepMillis(500); // activation is asynchronous
				devices = cam.listDeviceInfo();
				device = 0;
			}
			open();
		} else {
			message = "this device has one profile";
		}
	}
	if(key == 'f') ofToggleFullscreen();
}
