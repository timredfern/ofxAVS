#pragma once

#include "ofMain.h"
#include "ofxAVS.h"
#include "ofxImGui.h"
#include "avs_lib/core/plugin_manager.h"

class ofApp : public ofBaseApp {

public:
    void setup();
    void update();
    void draw();

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
    avs::ofxAVS visualizer;
    ofSoundStream soundStream;
    
    // Audio settings
    int sample_rate;
    int buffer_size;
    int num_input_channels;
    
    // Effect chain UI
    struct EffectChainItem {
        std::string name;
        std::string display_name;
        bool enabled;
        avs::EffectBase* effect_ptr;
        EffectChainItem(const std::string& n, const std::string& d, avs::EffectBase* ptr = nullptr) 
            : name(n), display_name(d), enabled(true), effect_ptr(ptr) {}
    };
    
    // Parameter control state
    struct ParameterControlState {
        std::string control_id;
        float float_value;
        bool bool_value;
        int int_value;
        ofColor color_value;
        
        ParameterControlState() : 
            control_id(""), float_value(0.5f), bool_value(false), 
            int_value(0), color_value(255, 255, 255) {}
            
        ParameterControlState(const std::string& id) : 
            control_id(id), float_value(0.5f), bool_value(false), 
            int_value(0), color_value(255, 255, 255) {}
    };
    std::map<std::string, ParameterControlState> control_states;
    
    std::vector<EffectChainItem> effect_chain;
    int selected_effect_index;
    
    // Available effects data
    struct AvailableEffect {
        std::string name;
        std::string display_name;
        std::string description;
        AvailableEffect(const std::string& n, const std::string& d, const std::string& desc) 
            : name(n), display_name(d), description(desc) {}
    };
    std::vector<AvailableEffect> available_effects;
    int selected_available_index;
    
    // UI state
    enum PanelMode {
        CHAIN_PANEL,
        AVAILABLE_PANEL,
        PARAMETERS_PANEL
    };
    PanelMode current_panel;
    
    // UI layout
    int chain_panel_width;
    int available_panel_width;
    int parameters_panel_width;
    int visualization_x;
    int visualization_y;
    int visualization_size;
    
    // ImGui setup
    ofxImGui::Gui gui;
    
    // Panel UI methods
    void drawEffectChain();
    void drawAvailableEffects();
    void drawParametersPanel();
    void drawVisualization();
    void addSelectedEffectToChain();
    void removeSelectedEffect();
    void moveEffectUp();
    void moveEffectDown();
    void toggleEffectEnabled();
    void rebuildEffectChain();
    
    // Parameter control methods
    void handleParameterMousePressed(int x, int y);
    void handleParameterMouseDragged(int x, int y);
    void updateEffectParameters();
    void initializeParameterDefaults();
    bool isPointInParametersPanel(int x, int y);
};