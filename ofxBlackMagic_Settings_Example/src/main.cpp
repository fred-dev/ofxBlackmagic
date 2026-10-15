#include "ofMain.h"
#include "ofApp.h"

int main() {
	// The first ofLog crashes on some Windows openFrameworks builds unless a channel is set.
	ofLogToConsole();

	ofGLWindowSettings settings;
	settings.setSize(1280, 800);
	settings.setGLVersion(3, 2);
	auto window = ofCreateWindow(settings);
	ofRunApp(window, std::make_shared<ofApp>());
	ofRunMainLoop();
}
