// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "ofMain.h"
#include "ofxImGui.h"
#include "core/renderer.h"
#include "core/plugin_manager.h"
#include "core/effect_base.h"
#include <vector>

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
    void setAudioData(const avs::AudioData& data);
    
    // Rendering
    void draw(int x, int y, int width, int height);
    void drawUI();
    
    // Effect chain management
    void addEffectToChain(const std::string& effectName);
    void removeEffectFromChain(int index);
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
    
    // Effect chain and UI state
    std::vector<EffectChainItem> effect_chain;
    std::vector<AvailableEffectInfo> available_effects;
    int selected_effect_index = -1;

    // UI panel dimensions
    int chain_panel_width = 200;
    int available_panel_width = 200; 
    int parameters_panel_width = 400;
    
    // Internal methods
    void initializeAvailableEffects();

    // UI rendering methods
    void drawChainPanel();
    void drawAvailableEffectsPanel();
    void drawParametersPanel();
};