#include "ofxBlackMagic.h"

#include "ColorConversion.h"

namespace {
// Raw frame (RGBA8 texture, 4 bytes per texel) -> RGB, one fragment per output pixel.
// mode 0: 8 bit YUV 4:2:2 (2vuy: Cb Y0 Cr Y1), 1: 10 bit YUV 4:2:2 (v210),
//      2: 8 bit ARGB, 3: 10 bit RGB (r210, big endian 2:10:10:10, video levels)
const char * fragmentBody = R"(
uniform sampler2DRect raw;
uniform int mode;
uniform int rec709;
IN vec2 vTex;

vec4 bytesAt(float x, float y) {
	return floor(TEXEL(raw, vec2(x + 0.5, y + 0.5)) * 255.0 + 0.5);
}

// little endian 32 bit word -> its three 10 bit components
vec3 v210Word(vec4 b) {
	return vec3(b.r + mod(b.g, 4.0) * 256.0,
				floor(b.g / 4.0) + mod(b.b, 16.0) * 64.0,
				floor(b.b / 16.0) + mod(b.a, 64.0) * 16.0);
}

// y 0..1, cb / cr -0.5..0.5
vec3 yuvToRgb(float y, float cb, float cr) {
	if (rec709 == 1) return vec3(y + 1.5748 * cr, y - 0.1873 * cb - 0.4681 * cr, y + 1.8556 * cb);
	return vec3(y + 1.402 * cr, y - 0.344136 * cb - 0.714136 * cr, y + 1.772 * cb);
}

void main() {
	float px = floor(vTex.x);
	float py = floor(vTex.y);
	vec3 rgb;
	if (mode == 0) {
		vec4 b = bytesAt(floor(px / 2.0), py);
		float y = mod(px, 2.0) < 0.5 ? b.g : b.a;
		rgb = yuvToRgb((y - 16.0) / 219.0, (b.r - 128.0) / 224.0, (b.b - 128.0) / 224.0);
	} else if (mode == 1) {
		// 6 pixels in 4 words: Cb0 Y0 Cr0 | Y1 Cb1 Y2 | Cr1 Y3 Cb2 | Y4 Cr2 Y5
		float group = floor(px / 6.0);
		float k = px - group * 6.0;
		float base = group * 4.0;
		vec3 w0 = v210Word(bytesAt(base, py));
		vec3 w1 = v210Word(bytesAt(base + 1.0, py));
		vec3 w2 = v210Word(bytesAt(base + 2.0, py));
		vec3 w3 = v210Word(bytesAt(base + 3.0, py));
		float y; float cb; float cr;
		if (k < 0.5)      { y = w0.y; cb = w0.x; cr = w0.z; }
		else if (k < 1.5) { y = w1.x; cb = w0.x; cr = w0.z; }
		else if (k < 2.5) { y = w1.z; cb = w1.y; cr = w2.x; }
		else if (k < 3.5) { y = w2.y; cb = w1.y; cr = w2.x; }
		else if (k < 4.5) { y = w3.x; cb = w2.z; cr = w3.y; }
		else              { y = w3.z; cb = w2.z; cr = w3.y; }
		rgb = yuvToRgb((y - 64.0) / 876.0, (cb - 512.0) / 896.0, (cr - 512.0) / 896.0);
	} else if (mode == 2) {
		vec4 b = bytesAt(px, py);
		rgb = b.gba / 255.0;
	} else {
		vec4 b = bytesAt(px, py);
		float r = mod(b.r, 64.0) * 16.0 + floor(b.g / 16.0);
		float g = mod(b.g, 16.0) * 64.0 + floor(b.b / 4.0);
		float bl = mod(b.b, 4.0) * 256.0 + b.a;
		rgb = (vec3(r, g, bl) - 64.0) / 876.0;
	}
	OUT = vec4(clamp(rgb, 0.0, 1.0), 1.0);
}
)";

const char * vertexGL2 = R"(#version 120
varying vec2 vTex;
void main() {
	vTex = gl_MultiTexCoord0.xy;
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
)";

const char * vertexGL3 = R"(#version 150
uniform mat4 modelViewProjectionMatrix;
in vec4 position;
in vec2 texcoord;
out vec2 vTex;
void main() {
	vTex = texcoord;
	gl_Position = modelViewProjectionMatrix * position;
}
)";

const char * fragmentHeaderGL2 = "#version 120\n#extension GL_ARB_texture_rectangle : enable\n#define IN varying\n#define TEXEL texture2DRect\n#define OUT gl_FragColor\n";
const char * fragmentHeaderGL3 = "#version 150\n#define IN in\n#define TEXEL texture\nout vec4 fragColor;\n#define OUT fragColor\n";

int shaderMode(BMDPixelFormat format) {
	switch (format) {
	case bmdFormat10BitYUV: return 1;
	case bmdFormat8BitARGB: return 2;
	case bmdFormat10BitRGB: return 3;
	default: return 0;
	}
}

bool isTenBitFormat(BMDPixelFormat format) {
	return format == bmdFormat10BitYUV || format == bmdFormat10BitRGB;
}
}

//--------------------------------------------------------------
ofxBlackMagic::ofxBlackMagic()
:grayPixOld(true)
,colorPixOld(true)
,colorPix16Old(true)
,rawCopyOld(true)
,rawTexOld(true)
,grayTexOld(true)
,colorTexOld(true)
,pboIndex(0)
,useGpuConversion(true)
,width(0)
,height(0)
,deviceId(0)
,desiredFrameRate(-1)
,bFrameNew(false)
,bInitialized(false)
,bUseTexture(true)
,inputConnection(bmdVideoConnectionUnspecified)
,colorFrameCaptureMode(LOW_LATENCY) {
}

ofxBlackMagic::~ofxBlackMagic() {
	close();
}

bool ofxBlackMagic::openDevice(int deviceId) {
	if(!controller.init()) {
		return false;
	}
	this->deviceId = deviceId;
	if(!controller.selectDevice(deviceId)) {
		return false;
	}
	if(inputConnection != bmdVideoConnectionUnspecified) {
		controller.setInputConnection(inputConnection);
	}
	ofLogVerbose("ofxBlackMagic") << "Available display modes: " << ofToString(controller.getDisplayModeNames());
	return true;
}

bool ofxBlackMagic::startCapture(BMDDisplayMode displayMode) {
	if(displayMode == bmdModeUnknown) {
		ofLogError("ofxBlackMagic") << "Resolution and framerate combination not supported.";
		return false;
	}
	if(!controller.startCaptureWithMode(displayMode)) {
		return false;
	}
	width = controller.getFrameWidth();
	height = controller.getFrameHeight();
	bInitialized = true;
	return true;
}

bool ofxBlackMagic::setup(int width, int height) {
	return setup(width, height, desiredFrameRate, deviceId, colorFrameCaptureMode);
}

bool ofxBlackMagic::setup(int width, int height, float framerate, int deviceId, ColorFrameCaptureMode colorFrameCaptureMode) {
	this->colorFrameCaptureMode = colorFrameCaptureMode;
	if(!openDevice(deviceId)) {
		return false;
	}
	return startCapture(controller.getDisplayMode(width, height, framerate));
}

bool ofxBlackMagic::setup(BMDDisplayMode displayMode, int deviceId, ColorFrameCaptureMode colorFrameCaptureMode) {
	this->colorFrameCaptureMode = colorFrameCaptureMode;
	if(!openDevice(deviceId)) {
		return false;
	}
	return startCapture(displayMode);
}

void ofxBlackMagic::setColorFrameCaptureMode(ColorFrameCaptureMode colorFrameCaptureMode) {
	this->colorFrameCaptureMode = colorFrameCaptureMode;
}

ofxBlackMagic::ColorFrameCaptureMode ofxBlackMagic::getColorFrameCaptureMode() {
	return colorFrameCaptureMode;
}

void ofxBlackMagic::setPixelFormat(BMDPixelFormat format) {
	controller.setPixelFormat(format);
}

BMDPixelFormat ofxBlackMagic::getPixelFormat() const {
	return controller.getPixelFormat();
}

string ofxBlackMagic::getPixelFormatName(BMDPixelFormat format) {
	return DeckLinkController::pixelFormatName(format);
}

DeckLinkSignalInfo ofxBlackMagic::getSignal() {
	return controller.getSignal();
}

string ofxBlackMagic::getSignalWarning() {
	return controller.getSignalWarning();
}

void ofxBlackMagic::setUseGpuConversion(bool useGpu) {
	useGpuConversion = useGpu;
	colorTexOld = true;
}

//--------------------------------------------------------------
vector<DeckLinkDeviceInfo> ofxBlackMagic::listDeviceInfo() {
	// enumerate again when not capturing (devices plugged in, profile changed)
	if(!controller.isCapturing() && !controller.init()) {
		return {};
	}
	return controller.getDeviceInfoList();
}

vector<ofVideoDevice> ofxBlackMagic::listDevices() {
	vector<ofVideoDevice> devices;
	for(auto & info : listDeviceInfo()) {
		ofVideoDevice device;
		device.id = info.index;
		device.deviceName = info.displayName;
		device.hardwareName = info.modelName.empty() ? "Blackmagic DeckLink" : info.modelName;
		device.serialID = info.persistentId != 0 ? ofToHex(info.persistentId) : "";
		device.bAvailable = info.canCapture;
		devices.push_back(device);
		string inputs;
		for(auto c : info.inputConnections) {
			inputs += (inputs.empty() ? "" : "/") + DeckLinkController::connectionName(c);
		}
		ofLogNotice("ofxBlackMagic") << info.index << ": " << info.displayName
			<< (info.modelName.empty() || info.modelName == info.displayName ? "" : " (" + info.modelName + ")")
			<< (info.interfaceName.empty() ? "" : ", " + info.interfaceName)
			<< (inputs.empty() ? "" : ", inputs " + inputs)
			<< (info.canCapture ? "" : ", no input")
			<< (info.persistentId != 0 ? ", id " + device.serialID : "");
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

void ofxBlackMagic::setInputConnection(BMDVideoConnection connection) {
	inputConnection = connection;
	if(controller.getSelectedDeviceIndex() >= 0) {
		controller.setInputConnection(connection);
	}
}

vector<BMDVideoConnection> ofxBlackMagic::getInputConnections() {
	return controller.getInputConnections();
}

BMDVideoConnection ofxBlackMagic::getInputConnection() {
	return controller.getInputConnection();
}

string ofxBlackMagic::getConnectionName(BMDVideoConnection connection) {
	return DeckLinkController::connectionName(connection);
}

vector<DeckLinkProfileInfo> ofxBlackMagic::listProfiles(int deviceId) {
	if(controller.getDeviceCount() == 0 && !controller.init()) {
		return {};
	}
	return controller.getProfiles(deviceId);
}

bool ofxBlackMagic::setProfile(int deviceId, BMDProfileID profile) {
	if(controller.getDeviceCount() == 0 && !controller.init()) {
		return false;
	}
	return controller.setProfile(deviceId, profile);
}

//--------------------------------------------------------------
void ofxBlackMagic::close() {
	if(controller.isCapturing()) {
		controller.stopCapture();
	}
	bInitialized = false;
	bFrameNew = false;
}

void ofxBlackMagic::checkFrameSize() {
	// The input format can change while capturing (format detection)
	const DeckLinkRawFrame & raw = controller.buffer.getFront();
	if(raw.width > 0 && raw.height > 0 && (raw.width != width || raw.height != height)) {
		ofLogNotice("ofxBlackMagic") << "Input is now " << raw.width << "x" << raw.height << " (" << DeckLinkController::pixelFormatName(raw.pixelFormat) << ")";
		width = raw.width;
		height = raw.height;
	}
}

bool ofxBlackMagic::update() {
	if(controller.buffer.swapFront()) {
		checkFrameSize();
		grayPixOld = colorPixOld = colorPix16Old = rawCopyOld = true;
		rawTexOld = grayTexOld = colorTexOld = true;
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
	return controller.getDisplayModeInfo(controller.getCurrentDisplayMode(), w, h, rate, name) ? name : "";
}

float ofxBlackMagic::getFrameRate() {
	int w, h;
	float rate = 0;
	string name;
	controller.getDisplayModeInfo(controller.getCurrentDisplayMode(), w, h, rate, name);
	return rate;
}

BMDPixelFormat ofxBlackMagic::getRawPixelFormat() {
	return controller.buffer.getFront().pixelFormat;
}

uint64_t ofxBlackMagic::getFramesArrived() {
	return controller.getFramesArrived();
}

uint64_t ofxBlackMagic::getFramesSkipped() {
	return controller.getFramesSkipped();
}

uint64_t ofxBlackMagic::getFrameNumber() {
	return controller.buffer.getFront().number;
}

//--------------------------------------------------------------
const DeckLinkRawFrame& ofxBlackMagic::getRawFrame() {
	return controller.buffer.getFront();
}

vector<unsigned char>& ofxBlackMagic::getYuvRaw() {
	if(rawCopyOld) {
		const DeckLinkRawFrame & raw = controller.buffer.getFront();
		DeckLinkController::readFrame(raw.frame, [&](const unsigned char * bytes) {
			rawCopy.assign(bytes, bytes + (size_t)raw.rowBytes * raw.height);
		});
		rawCopyOld = false;
	}
	return rawCopy;
}

ofPixels& ofxBlackMagic::getGrayPixels() {
	if(grayPixOld) {
		const DeckLinkRawFrame & raw = controller.buffer.getFront();
		if(raw.frame != nullptr && raw.width > 0 && raw.height > 0) {
			grayPix.allocate(raw.width, raw.height, OF_PIXELS_GRAY);
			DeckLinkController::readFrame(raw.frame, [&](const unsigned char * bytes) {
				for(int y = 0; y < raw.height; y++) {
					const unsigned char * src = bytes + (size_t)y * raw.rowBytes;
					unsigned char * dst = grayPix.getData() + (size_t)y * raw.width;
					switch(raw.pixelFormat) {
					case bmdFormat10BitYUV: v210_to_y(src, dst, raw.width); break;
					case bmdFormat8BitARGB: argb_to_y(src, dst, raw.width); break;
					case bmdFormat10BitRGB: r210_to_y(src, dst, raw.width); break;
					default: cby0cry1_to_y(const_cast<unsigned char *>(src), dst, raw.width); break;
					}
				}
			});
		}
		grayPixOld = false;
	}
	return grayPix;
}

ofPixels& ofxBlackMagic::getColorPixels() {
	if(colorPixOld) {
		const DeckLinkRawFrame & raw = controller.buffer.getFront();
		if(raw.frame != nullptr) {
			controller.convertToRGBA(raw.frame, colorPix);
		}
		colorPixOld = false;
	}
	return colorPix;
}

ofShortPixels& ofxBlackMagic::getColorPixels16() {
	if(colorPix16Old) {
		if(colorTexOld) {
			convertOnGpu();
			colorTexOld = false;
		}
		if(convertFbo.isAllocated()) {
			convertFbo.readToPixels(colorPix16);
		}
		colorPix16Old = false;
	}
	return colorPix16;
}

//--------------------------------------------------------------
// The raw frame goes to the GPU through two pixel buffer objects used in turn, so
// the upload doesn't wait for the previous one.
bool ofxBlackMagic::uploadRawTexture() {
	const DeckLinkRawFrame & raw = controller.buffer.getFront();
	if(raw.frame == nullptr || raw.rowBytes <= 0 || raw.height <= 0) {
		return false;
	}
	const int texWidth = raw.rowBytes / 4;
	if(!rawTex.isAllocated() || int(rawTex.getWidth()) != texWidth || int(rawTex.getHeight()) != raw.height) {
		rawTex.allocate(texWidth, raw.height, GL_RGBA8, true, GL_RGBA, GL_UNSIGNED_BYTE);
		rawTex.setTextureMinMagFilter(GL_NEAREST, GL_NEAREST);
	}
	const size_t size = (size_t)raw.rowBytes * raw.height;
	ofBufferObject & buffer = pbo[pboIndex];
	pboIndex = 1 - pboIndex;
	if(!buffer.isAllocated() || (size_t)buffer.size() < size) {
		buffer.allocate(size, GL_STREAM_DRAW);
	}
	const bool ok = DeckLinkController::readFrame(raw.frame, [&](const unsigned char * bytes) {
		buffer.updateData(0, size, bytes);
	});
	if(ok) {
		rawTex.loadData(buffer, GL_RGBA, GL_UNSIGNED_BYTE);
	}
	return ok;
}

bool ofxBlackMagic::convertOnGpu() {
	const DeckLinkRawFrame & raw = controller.buffer.getFront();
	if(raw.frame == nullptr) {
		return false;
	}
	ofTexture & source = getRawTexture();
	if(!source.isAllocated()) {
		return false;
	}
	if(!convertShader.isLoaded()) {
		const bool gl3 = ofIsGLProgrammableRenderer();
		convertShader.setupShaderFromSource(GL_VERTEX_SHADER, gl3 ? vertexGL3 : vertexGL2);
		convertShader.setupShaderFromSource(GL_FRAGMENT_SHADER, string(gl3 ? fragmentHeaderGL3 : fragmentHeaderGL2) + fragmentBody);
		if(gl3) {
			convertShader.bindDefaults();
		}
		if(!convertShader.linkProgram()) {
			ofLogError("ofxBlackMagic") << "colour conversion shader failed, using the CPU";
			useGpuConversion = false;
			return false;
		}
	}
	const int internalFormat = isTenBitFormat(raw.pixelFormat) ? GL_RGBA16 : GL_RGBA8;
	if(!convertFbo.isAllocated() || int(convertFbo.getWidth()) != raw.width || int(convertFbo.getHeight()) != raw.height
		|| convertFbo.getTexture().getTextureData().glInternalFormat != internalFormat) {
		ofFbo::Settings settings;
		settings.width = raw.width;
		settings.height = raw.height;
		settings.internalformat = internalFormat;
		settings.useDepth = false;
		settings.useStencil = false;
		convertFbo.allocate(settings);
		quad.clear();
		quad.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
		const float w = raw.width, h = raw.height;
		for(auto p : { glm::vec2(0, 0), glm::vec2(w, 0), glm::vec2(w, h), glm::vec2(0, h) }) {
			quad.addVertex(glm::vec3(p, 0));
			quad.addTexCoord(p); // = output pixel position, the shader works in pixels
		}
	}
	convertFbo.begin();
	ofPushStyle();
	ofDisableBlendMode();
	convertShader.begin();
	convertShader.setUniformTexture("raw", source, 0);
	convertShader.setUniform1i("mode", shaderMode(raw.pixelFormat));
	convertShader.setUniform1i("rec709", raw.height > 576 ? 1 : 0);
	quad.draw();
	convertShader.end();
	ofPopStyle();
	convertFbo.end();
	return true;
}

ofTexture& ofxBlackMagic::getRawTexture() {
	if(rawTexOld) {
		uploadRawTexture();
		rawTexOld = false;
	}
	return rawTex;
}

ofTexture& ofxBlackMagic::getYuvTexture() {
	return getRawTexture();
}

ofTexture& ofxBlackMagic::getGrayTexture() {
	if(grayTexOld) {
		ofPixels & pix = getGrayPixels();
		if(pix.isAllocated()) {
			grayTex.loadData(pix);
		}
		grayTexOld = false;
	}
	return grayTex;
}

ofTexture& ofxBlackMagic::getColorTexture() {
	if(useGpuConversion) {
		if(colorTexOld) {
			convertOnGpu();
			colorTexOld = false;
		}
		if(useGpuConversion && convertFbo.isAllocated()) {
			return convertFbo.getTexture();
		}
	}
	// CPU
	if(colorTexOld) {
		ofPixels& pix = getColorPixels();
		if(pix.isAllocated()) {
			colorTex.loadData(pix);
		}
		colorTexOld = false;
	}
	return colorTex;
}

//--------------------------------------------------------------
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

void ofxBlackMagic::drawYuv(float x, float y) {
	getRawTexture().draw(x, y);
}

void ofxBlackMagic::drawYuv(float x, float y, float w, float h) {
	getRawTexture().draw(x, y, w, h);
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
