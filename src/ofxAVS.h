// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "ofMain.h"
#include "ofxImGui.h"
#include "../libs/avs_lib/core/renderer.h"
#include "../libs/avs_lib/core/plugin_manager.h"
#include "../libs/avs_lib/core/effect_base.h"
#include <vector>
#include <map>

// Effect chain item for the UI
struct EffectChainItem {
    std::string name;
    std::string display_name;
    std::unique_ptr<avs::EffectBase> effect; // Store the actual effect instance
};

// Available effect info
struct AvailableEffectInfo {
    std::string name;
    std::string display_name; 
    std::string description;
};

// Parameter control state for UI
struct ParameterControlState {
    std::string control_id;
    int int_value = 0;
    float float_value = 0.0f;
    bool bool_value = false;
    ofColor color_value = ofColor(255, 255, 255);
    
    ParameterControlState() = default;
    ParameterControlState(const std::string& id) : control_id(id) {}
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
    std::vector<uint32_t> framebuffer;
    std::vector<uint32_t> output_buffer;
    int width, height;
    avs::AudioData current_audio_data;
    
    // Effect chain and UI state
    std::vector<EffectChainItem> effect_chain;
    std::vector<AvailableEffectInfo> available_effects;
    int selected_effect_index = -1;
    
    // UI control states
    std::map<std::string, ParameterControlState> control_states;
    
    // UI panel dimensions
    int chain_panel_width = 200;
    int available_panel_width = 300; 
    int parameters_panel_width = 250;
    
    // Internal methods
    void initializeAvailableEffects();
    void rebuildEffectChain();
    void updateEffectParameters();
    void initializeParameterDefaults();
    
    // UI rendering methods
    void drawChainPanel();
    void drawAvailableEffectsPanel();
    void drawParametersPanel();
};