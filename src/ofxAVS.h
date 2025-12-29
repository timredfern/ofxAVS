// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "ofMain.h"
#include "ofxImGui.h"
#include "ofxFft.h"
#include "core/renderer.h"
#include "core/plugin_manager.h"
#include "core/effect_base.h"
#include <vector>

// FFT mode selection:
// Define AVS_ENHANCED_FFT for modern processing (2048 samples, smoothing, dB scale)
// Undefine for original Winamp behavior (512 samples, log table, no smoothing)
#define AVS_ENHANCED_FFT

// Effect chain item for the UI (metadata only - effects owned by renderer)
struct EffectChainItem {
    std::string name;
    std::string display_name;
};

// Available effect info
struct AvailableEffectInfo {
    std::string name;
    std::string display_name;
    std::string description;
};

class ofxAVS {
public:
    ofxAVS();
    ~ofxAVS();

    // Setup and audio
    void setup();
    void update();
    void audioIn(ofSoundBuffer& buffer);  // Process audio with FFT
    void setAudioData(const avs::AudioData& data);  // Direct access if needed

    // Rendering
    void draw(int x, int y, int width, int height);
    void drawUI();

    // Effect chain management
    void addEffectToChain(const std::string& effectName);
    void removeEffectFromChain(int index);
    void duplicateEffect(int index);
    void moveEffectUp(int index);
    void moveEffectDown(int index);

    // UI panel management
    void setSelectedEffect(int index);
    int getSelectedEffect() const { return selected_effect_index; }

    // Effect chain access
    const std::vector<EffectChainItem>& getEffectChain() const { return effect_chain; }
    const std::vector<AvailableEffectInfo>& getAvailableEffects() const { return available_effects; }

private:
    // Core AVS components
    std::unique_ptr<avs::DefaultRenderer> renderer;
    ofTexture texture;
    ofPixels pixels;
    int width, height;
    avs::AudioData current_audio_data;

    // FFT for spectrum analysis
    ofxFft* fft;
#ifdef AVS_ENHANCED_FFT
    static const int FFT_SIZE = 2048;  // Higher resolution for enhanced mode
    float smoothedSpectrum[576];       // Temporal smoothing buffer
#else
    static const int FFT_SIZE = 512;   // Original Winamp FFT size
    unsigned char logTable[256];       // AVS log compression table
#endif

    // Effect chain and UI state
    std::vector<EffectChainItem> effect_chain;
    std::vector<AvailableEffectInfo> available_effects;
    int selected_effect_index = -1;

    // UI panel dimensions
    int chain_panel_width = 200;
    int available_panel_width = 200;
    int parameters_panel_width = 440;
    int parameters_panel_height = 480;

    // Internal methods
    void initializeAvailableEffects();

    // UI rendering methods
    void drawChainPanel();
    void drawAvailableEffectsPanel();
    void drawParametersPanel();
};