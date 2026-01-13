// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "ofMain.h"
#include "ofxAVS.h"
#include "ofxImGui.h"
#include "ofxAudioDecoder.h"

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
    void audioOut(ofSoundBuffer& buffer);

private:
    ofxAVS avs;
    ofSoundStream soundStream;
    bool audioInitialized = false;
    bool hasAudioInput = false;
    ofxImGui::Gui gui;

    // Audio device selection
    std::vector<ofSoundDevice> audioDevices;
    std::vector<int> inputDeviceIndices;   // Indices into audioDevices for input-capable devices
    std::vector<int> outputDeviceIndices;  // Indices into audioDevices for output-capable devices
    int selectedInputDevice = -1;   // Index into inputDeviceIndices (-1 = none)
    int selectedOutputDevice = -1;  // Index into outputDeviceIndices
    std::string selectedInputDeviceName;   // For persistence
    std::string selectedOutputDeviceName;  // For persistence

    // Sound file playback
    ofSoundBuffer audioFileBuffer;
    size_t playbackPos = 0;
    bool useFileInput = false;
    bool isPlaying = false;
    std::string loadedFileName;
    std::string loadedFilePath;  // Full path for persistence
    float micGain = 1.0f;  // Microphone gain (1x to 100x)

    void loadSoundFile(const std::string& path);
    void drawAudioControls();
    void setupAudioDevices();
    void restartAudio();

    // App settings persistence (separate from AVS preset)
    void loadAppSettings();
    void saveAppSettings();
};
