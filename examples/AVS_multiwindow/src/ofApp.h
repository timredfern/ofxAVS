// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "ofMain.h"
#include "ofxAVS.h"
#include "ofxImGui.h"
#include "ofxAudioDecoder.h"
#include <memory>
#include <vector>

class ofApp : public ofBaseApp {

public:
    void setup();
    void update();
    void draw();  // Chain window
    void exit();

    void keyPressed(int key);
    void dragEvent(ofDragInfo dragInfo);

    // Audio callbacks
    void audioIn(ofSoundBuffer& buffer);
    void audioOut(ofSoundBuffer& buffer);

    // Window management
    void setWindows(std::shared_ptr<ofAppBaseWindow> chain, std::shared_ptr<ofAppBaseWindow> output);

    // Output window event handlers
    void drawOutput(ofEventArgs& args);
    void keyPressedOutput(ofKeyEventArgs& args);

private:
    ofxAVS avs;
    ofxImGui::Gui chain_gui_;  // ImGui for chain window
    ofSoundStream soundStream;
    bool audioInitialized = false;
    bool hasAudioInput = false;

    // Window handles
    std::shared_ptr<ofAppBaseWindow> chain_window_;
    std::shared_ptr<ofAppBaseWindow> output_window_;

    // Audio device selection
    std::vector<ofSoundDevice> audioDevices;
    std::vector<int> inputDeviceIndices;
    std::vector<int> outputDeviceIndices;
    int selectedInputDevice = -1;
    int selectedOutputDevice = -1;
    std::string selectedInputDeviceName;
    std::string selectedOutputDeviceName;

    // Sound file playback
    ofSoundBuffer audioFileBuffer;
    size_t playbackPos = 0;
    bool useFileInput = false;
    bool isPlaying = false;
    std::string loadedFileName;
    std::string loadedFilePath;
    float micGain = 1.0f;

    void loadSoundFile(const std::string& path, bool autoPlay = true);
    void drawAudioControls();
    void setupAudioDevices();
    void restartAudio();

    // App settings persistence
    void loadAppSettings();
    void saveAppSettings();

    // Parameter windows
    struct ParamWindowInfo {
        std::shared_ptr<ofAppBaseWindow> window;
        std::unique_ptr<ofxImGui::Gui> gui;
        avs::Configurable* configurable;
        uintptr_t effect_id;  // For validation after effect deletion
        bool needs_setup = true;  // Defer setup to first draw when window context is active
    };
    std::vector<std::unique_ptr<ParamWindowInfo>> param_windows_;

    void openParamWindow(avs::Configurable* configurable);
    void closeParamWindow(avs::Configurable* configurable);
    void cleanupInvalidParamWindows();
    bool isEffectValid(uintptr_t id);
    avs::EffectBase* findEffectById(avs::EffectContainer* container, uintptr_t id);

    // Draw handler for parameter windows (called via event)
    void drawParamWindows(ofEventArgs& args);  // Event handler - finds current window
    void drawParamWindow(ParamWindowInfo& info);  // Actual drawing
};
