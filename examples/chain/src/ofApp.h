#pragma once

#include "ofMain.h"
#include "ofxAVS.h"
#include "ofxImGui.h"

class ofApp : public ofBaseApp {

public:
    void setup();
    void update();
    void draw();
    void exit();

    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    
    // Audio callbacks
    void audioIn(ofSoundBuffer& buffer);

private:
    // Main AVS visualization with UI
    ofxAVS avs;
    
    // Audio input
    ofSoundStream soundStream;
    bool audioInitialized;
    
    // ImGui setup
    ofxImGui::Gui gui;
    
    // Audio processing
    void processAudioData(ofSoundBuffer& buffer);
};