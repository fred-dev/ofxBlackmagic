#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    bmOutput.setup();
    camWidth 		= 1920;	// try to grab at this size.
    camHeight 		= 1080;
    grabber.initGrabber(camWidth, camHeight);
    videoInverted 	= new unsigned char[camWidth*camHeight*3];
    videoTexture.allocate(camWidth,camHeight, GL_RGB);

}

//--------------------------------------------------------------
void ofApp::update(){
    grabber.update();
    
    if (grabber.isFrameNew()){
        int totalPixels = camWidth*camHeight*3;
        unsigned char * pixels = grabber.getPixels();
        for (int i = 0; i < totalPixels; i++){
            videoInverted[i] = 255 - pixels[i];
        }
        videoTexture.loadData(videoInverted, camWidth,camHeight, GL_RGB);
        bmOutput.renderFrame(videoInverted, camWidth*camHeight*3);
    }

}

//--------------------------------------------------------------
void ofApp::draw(){
    videoTexture.draw(0,0);

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
