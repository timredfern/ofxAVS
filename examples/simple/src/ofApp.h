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
    
    // Demo controls
    int current_effect;
    std::vector<std::string> effect_names;
    bool auto_cycle_effects;
    float effect_cycle_time;
    float last_effect_change;
    
    // Setup methods
    void setupOscilloscopeDynamicMovement();
    void setupMovementChain(const std::string& expression);
    
    // Movement expressions
    std::string current_expression;
    std::vector<std::pair<std::string, std::string>> preset_expressions;
    
    // Interpolation mode
    avs::InterpolationMode current_interpolation_mode;
    std::vector<std::pair<std::string, avs::InterpolationMode>> interpolation_modes;
};