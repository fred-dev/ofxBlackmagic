#include "ofMain.h"
#include "ofApp.h"

int main() {
	// The first ofLog crashes on some Windows openFrameworks builds unless a channel is set.
	ofLogToConsole();

	ofGLWindowSettings settings;
	settings.setSize(1920, 540);
	settings.windowMode = OF_WINDOW;
	auto window = ofCreateWindow(settings);
	ofRunApp(window, std::make_shared<ofApp>());
	ofRunMainLoop();
}
