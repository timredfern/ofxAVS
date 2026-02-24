// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "ofMain.h"
#include "ofxAVS.h"

class ofApp : public ofBaseApp {

public:
    void setup();
    void update();
    void draw();
    void exit();

    void keyPressed(int key);
    void windowResized(int w, int h);

    // Audio callback
    void audioIn(ofSoundBuffer& buffer);

private:
    ofxAVS avs;
    ofSoundStream soundStream;
    bool audioInitialized = false;
};
