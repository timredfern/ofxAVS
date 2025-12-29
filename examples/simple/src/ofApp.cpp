// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetWindowTitle("AVS Simple Example");
    ofSetWindowShape(800, 640);

    audioInitialized = false;

    gui.setup();
    avs.setup();

    // Setup audio input
    vector<ofSoundDevice> devices = soundStream.getDeviceList();

    ofSoundStreamSettings settings;
    settings.sampleRate = 44100;
    settings.numInputChannels = 1;
    settings.numOutputChannels = 0;
    settings.bufferSize = 576;
    settings.setInListener(this);

    for (auto& device : devices) {
        if (device.inputChannels > 0) {
            settings.setInDevice(device);
            try {
                soundStream.setup(settings);
                audioInitialized = true;
                ofLogNotice() << "Audio setup with: " << device.name;
                break;
            } catch (...) {
                ofLogWarning() << "Failed: " << device.name;
            }
        }
    }

    if (!audioInitialized) {
        ofLogWarning() << "No audio input available";
    }
}

//--------------------------------------------------------------
void ofApp::update(){
    avs.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(20);
    avs.draw(100, 20, 600, 600);

    gui.begin();
    avs.drawUI();
    gui.end();

    ofSetColor(255);
    ofDrawBitmapString("FPS: " + ofToString(ofGetFrameRate(), 1), 20, ofGetHeight() - 20);
}

//--------------------------------------------------------------
void ofApp::audioIn(ofSoundBuffer& buffer) {
    avs.audioIn(buffer);
}

//--------------------------------------------------------------
void ofApp::exit(){
    if (audioInitialized) {
        soundStream.stop();
        soundStream.close();
        audioInitialized = false;
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){}
void ofApp::keyReleased(int key){}
void ofApp::mouseMoved(int x, int y){}
void ofApp::mouseDragged(int x, int y, int button){}
void ofApp::mousePressed(int x, int y, int button){}
void ofApp::mouseReleased(int x, int y, int button){}
void ofApp::mouseEntered(int x, int y){}
void ofApp::mouseExited(int x, int y){}
void ofApp::windowResized(int w, int h){}
void ofApp::dragEvent(ofDragInfo dragInfo){}
void ofApp::gotMessage(ofMessage msg){}
