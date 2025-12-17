#pragma once

#include "ofMain.h"
#include "ofxAVS.h"

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
        EffectChainItem(const std::string& n, const std::string& d) 
            : name(n), display_name(d), enabled(true) {}
    };
    
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
        AVAILABLE_PANEL
    };
    PanelMode current_panel;
    
    // UI layout
    int chain_panel_width;
    int available_panel_width;
    int visualization_x;
    int visualization_y;
    int visualization_size;
    
    // Panel UI methods
    void drawEffectChain();
    void drawAvailableEffects();
    void drawVisualization();
    void addSelectedEffectToChain();
    void removeSelectedEffect();
    void moveEffectUp();
    void moveEffectDown();
    void toggleEffectEnabled();
    void rebuildEffectChain();
};