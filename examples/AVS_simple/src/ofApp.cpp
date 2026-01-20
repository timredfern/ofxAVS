// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetWindowTitle("AVS_simple");
    ofSetWindowShape(640, 480);
    ofSetFrameRate(60);

    avs.setup();
    avs.getRoot()->parameters().set_bool("clear_each_frame", true);
    avs.addEffect("Oscilloscope");

    // Setup audio input (mic) - use 1 channel for compatibility
    ofSoundStreamSettings settings;
    settings.sampleRate = 44100;
    settings.numInputChannels = 1;
    settings.numOutputChannels = 0;
    settings.bufferSize = 576;
    settings.setInListener(this);

    auto devices = soundStream.getDeviceList();
    for (auto& device : devices) {
        if (device.inputChannels > 0) {
            settings.setInDevice(device);
            if (soundStream.setup(settings)) {
                audioInitialized = true;
                ofLogNotice() << "Audio: " << device.name;
                break;
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
    ofBackground(0);
    avs.draw(0, 0, ofGetWidth(), ofGetHeight());
}

//--------------------------------------------------------------
void ofApp::audioIn(ofSoundBuffer& buffer) {
    // Apply 5x gain for better visibility
    ofSoundBuffer amplified = buffer;
    for (size_t i = 0; i < amplified.size(); i++) {
        amplified[i] = ofClamp(amplified[i] * 5.0f, -1.0f, 1.0f);
    }
    avs.audioIn(amplified);
}

//--------------------------------------------------------------
void ofApp::exit(){
    if (audioInitialized) {
        soundStream.stop();
        soundStream.close();
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if (key == 'f' || key == 'F') {
        ofToggleFullscreen();
    }
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    // AVS will adapt to new size on next draw
}
