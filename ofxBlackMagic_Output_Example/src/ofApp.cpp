#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

    camWidth 		= 1920;	// try to grab at this size.
    camHeight 		= 1080;
    grabber.initGrabber(camWidth, camHeight);
    
    myImage.allocate(camWidth, camHeight, OF_IMAGE_COLOR_ALPHA);
    
   
    bmOutput.setup('Hp25');
    

}

//--------------------------------------------------------------
void ofApp::update(){
    grabber.update();
    
    if (grabber.isFrameNew()){
        myImage.setFromPixels(grabber.getPixelsRef().getPixels(), camWidth, camHeight, OF_IMAGE_COLOR);
        myImage.setImageType(OF_IMAGE_COLOR_ALPHA);
        int totalpix = camWidth*camHeight;
                    unsigned char c;
                    unsigned char* buf = myImage.getPixels();
        
                    for(int i=0;i<totalpix;i++){
                        c = buf[i*4];
                        buf[i*4] = buf[i*4 + 2];
                        buf[i*4+2] = c;
                    }
            bmOutput.renderFrame(buf, camWidth*camHeight*4);
    }
   
}

//--------------------------------------------------------------
void ofApp::draw(){

    

    //myImage.draw(0,0,1280,720);

    
    

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
