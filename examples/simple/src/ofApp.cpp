#include "ofApp.h"
#include "avs_lib/core/plugin_manager.h"

void ofApp::setup() {
    ofSetWindowTitle("ofxAVS Example");
    ofSetFrameRate(60);
    
    // Audio setup
    sample_rate = 44100;
    buffer_size = 512;
    num_input_channels = 1; // Most built-in mics are mono
    
    soundStream.printDeviceList();
    
    ofSoundStreamSettings settings;
    settings.setInListener(this);
    settings.sampleRate = sample_rate;
    settings.numOutputChannels = 0;
    settings.numInputChannels = num_input_channels;
    settings.bufferSize = buffer_size;
    soundStream.setup(settings);
    
    // Visualizer setup
    visualizer.setup(512, 512);
    
    // Demo effects setup for oscilloscope + dynamic movement
    effect_names = {"clear", "oscilloscope", "dynamic_movement", "transform"};
    current_effect = 0;
    auto_cycle_effects = false; // Disable auto cycling for manual control
    effect_cycle_time = 5.0f;
    last_effect_change = 0;
    
    // Setup preset movement expressions
    preset_expressions = {
        {"Downward drift", "x=x; y=y-0.01"},
        {"Upward drift", "x=x; y=y+0.01"},
        {"Rightward drift", "x=x+0.01; y=y"},
        {"Leftward drift", "x=x-0.01; y=y"},
        {"Zoom in", "x=0.5+(x-0.5)*0.95; y=0.5+(y-0.5)*0.95"},
        {"Zoom out", "x=0.5+(x-0.5)*1.05; y=0.5+(y-0.5)*1.05"},
        {"Rotate", "x=0.5+(x-0.5)*cos(0.05)-(y-0.5)*sin(0.05); y=0.5+(x-0.5)*sin(0.05)+(y-0.5)*cos(0.05)"},
        {"Swirl", "x=0.5+(x-0.5)*cos((x*x+y*y)*0.1)-(y-0.5)*sin((x*x+y*y)*0.1); y=0.5+(x-0.5)*sin((x*x+y*y)*0.1)+(y-0.5)*cos((x*x+y*y)*0.1)"},
        {"Wave X", "x=x+sin(y*10)*0.02; y=y"},
        {"Wave Y", "x=x; y=y+sin(x*10)*0.02"}
    };
    
    // Setup interpolation modes
    interpolation_modes = {
        {"None (Stepped/Classic AVS)", avs::InterpolationMode::NONE},
        {"Linear (Smooth)", avs::InterpolationMode::LINEAR},
        {"Nearest", avs::InterpolationMode::NEAREST}
    };
    current_interpolation_mode = avs::InterpolationMode::LINEAR;
    
    // Set up default oscilloscope with dynamic movement
    setupOscilloscopeDynamicMovement();
    
    ofLogNotice() << "ofxAVS Example Started - Oscilloscope + Dynamic Movement + Brightness";
    ofLogNotice() << "";
    ofLogNotice() << "MOVEMENT EXPRESSION CONTROLS:";
    ofLogNotice() << "  0-9: Select preset expressions";
    ofLogNotice() << "  e: Enter custom expression (console input)";
    ofLogNotice() << "";
    ofLogNotice() << "OTHER CONTROLS:";
    ofLogNotice() << "  d: Reload current expression";
    ofLogNotice() << "  c: Clear all effects";
    ofLogNotice() << "  SPACE: Toggle auto-cycle effects";
}

void ofApp::update() {
    // Auto-cycle effects
    if (auto_cycle_effects && ofGetElapsedTimef() - last_effect_change > effect_cycle_time) {
        current_effect = (current_effect + 1) % effect_names.size();
        visualizer.clearEffects();
        visualizer.addEffect(effect_names[current_effect]);
        last_effect_change = ofGetElapsedTimef();
        ofLogNotice() << "Auto-switched to effect: " << effect_names[current_effect];
    }
    
    visualizer.update();
}

void ofApp::draw() {
    ofBackground(0);
    
    // Draw visualizer centered
    float viz_size = 400;
    float x = (ofGetWidth() - viz_size) / 2;
    float y = (ofGetHeight() - viz_size) / 2;
    
    visualizer.draw(x, y, viz_size, viz_size);
    
    // Draw UI
    ofSetColor(255);
    ofDrawBitmapString("ofxAVS Example - Oscilloscope + Dynamic Movement + Brightness", 20, 30);
    ofDrawBitmapString("Current Expression: " + current_expression, 20, 50);
    ofDrawBitmapString("FPS: " + ofToString(ofGetFrameRate(), 1), 20, 70);
    ofDrawBitmapString("Beat: " + std::string(visualizer.isBeat() ? "YES" : "NO"), 20, 90);
    
    // Draw preset expressions list
    ofDrawBitmapString("PRESET EXPRESSIONS:", 20, 130);
    for (size_t i = 0; i < preset_expressions.size() && i < 10; i++) {
        std::string marker = (preset_expressions[i].second == current_expression) ? " <--" : "";
        ofDrawBitmapString(ofToString(i) + ": " + preset_expressions[i].first + marker, 20, 150 + i * 20);
    }
    
    // Draw interpolation mode
    ofDrawBitmapString("INTERPOLATION MODE:", 20, 350);
    for (size_t i = 0; i < interpolation_modes.size(); i++) {
        std::string marker = (interpolation_modes[i].second == current_interpolation_mode) ? " <--" : "";
        ofDrawBitmapString(interpolation_modes[i].first + marker, 20, 370 + i * 20);
    }
    
    // Draw controls
    ofDrawBitmapString("CONTROLS:", ofGetWidth() - 300, 130);
    ofDrawBitmapString("0-9: Select preset expression", ofGetWidth() - 300, 150);
    ofDrawBitmapString("e: Enter custom expression", ofGetWidth() - 300, 170);
    ofDrawBitmapString("i: Cycle interpolation mode", ofGetWidth() - 300, 190);
    ofDrawBitmapString("d: Reload current expression", ofGetWidth() - 300, 210);
    ofDrawBitmapString("c: Clear all effects", ofGetWidth() - 300, 230);
    ofDrawBitmapString("SPACE: Toggle auto-cycle", ofGetWidth() - 300, 250);
    
    // Draw audio info
    ofDrawBitmapString("Audio: " + std::to_string(num_input_channels) + "ch @ " + 
                      std::to_string(sample_rate) + "Hz", 20, ofGetHeight() - 20);
}

void ofApp::keyPressed(int key) {
    // Handle number keys for preset expressions
    if (key >= '0' && key <= '9') {
        int index = key - '0';
        if (index < preset_expressions.size()) {
            setupMovementChain(preset_expressions[index].second);
            ofLogNotice() << "Selected preset: " << preset_expressions[index].first;
        }
        return;
    }
    
    switch(key) {
        case 'e': {
            // Enter custom expression
            std::string input;
            ofLogNotice() << "Enter movement expression (e.g., x=x+0.01; y=y-0.01):";
            std::cout << "Expression: ";
            std::getline(std::cin, input);
            if (!input.empty()) {
                setupMovementChain(input);
                ofLogNotice() << "Custom expression set: " << input;
            }
            break;
        }
        
        case 'i': {
            // Cycle through interpolation modes
            for (size_t i = 0; i < interpolation_modes.size(); i++) {
                if (interpolation_modes[i].second == current_interpolation_mode) {
                    current_interpolation_mode = interpolation_modes[(i + 1) % interpolation_modes.size()].second;
                    break;
                }
            }
            // Reload current effect with new interpolation mode
            if (!current_expression.empty()) {
                setupMovementChain(current_expression);
                
                // Find current mode name for logging
                std::string mode_name = "Unknown";
                for (const auto& mode : interpolation_modes) {
                    if (mode.second == current_interpolation_mode) {
                        mode_name = mode.first;
                        break;
                    }
                }
                ofLogNotice() << "Interpolation mode: " << mode_name;
            }
            break;
        }
            
        case ' ':
            auto_cycle_effects = !auto_cycle_effects;
            last_effect_change = ofGetElapsedTimef();
            ofLogNotice() << "Auto-cycle: " << (auto_cycle_effects ? "ON" : "OFF");
            break;
            
        case 'c':
            visualizer.clearEffects();
            current_expression = "";
            ofLogNotice() << "Cleared all effects";
            break;
            
        case 'd':
            if (!current_expression.empty()) {
                setupMovementChain(current_expression);
                ofLogNotice() << "Reloaded expression: " << current_expression;
            } else {
                setupOscilloscopeDynamicMovement();
                ofLogNotice() << "Reloaded default oscilloscope + dynamic movement + brightness";
            }
            break;
    }
}

void ofApp::audioIn(ofSoundBuffer& buffer) {
    // Pass audio data to visualizer
    visualizer.audioReceived(buffer.getBuffer().data(), buffer.getNumFrames(), buffer.getNumChannels());
}

void ofApp::setupOscilloscopeDynamicMovement() {
    // Clear any existing effects
    visualizer.clearEffects();
    
    // Setup with configurable movement expression
    setupMovementChain("x=x; y=y-0.01");  // Default: downward drift
}

void ofApp::setupMovementChain(const std::string& expression) {
    // Clear any existing effects
    visualizer.clearEffects();
    
    ofLogNotice() << "Setting up movement chain with expression: " << expression;
    
    // 1. Add brightness effect for darkening/trails
    auto brightness_effect = visualizer.addEffect("brightness");
    if (brightness_effect) {
        brightness_effect->parameters().set_int("brightness", -1);  // Slight darkening
    }
    ofLogNotice() << "  - Added brightness effect (-1)";
    
    // 2. Add oscilloscope for audio visualization
    visualizer.addEffect("oscilloscope");
    // Using default parameters (white color, left channel, dots)
    ofLogNotice() << "  - Added oscilloscope effect";
    
    // 3. Add dynamic movement with custom expression
    auto movement = visualizer.addEffect("dynamic_movement");
    if (movement) {
        movement->parameters().set_string("pixel_script", expression);
        movement->parameters().set_bool("rectangular", true);
        movement->parameters().set_int("grid_width", 16);
        movement->parameters().set_int("grid_height", 16);
        movement->parameters().set_int("interpolation", static_cast<int>(current_interpolation_mode));
    }
    ofLogNotice() << "  - Added dynamic movement with expression: " << expression;
    
    current_expression = expression;
}

void ofApp::keyReleased(int key) {}
void ofApp::mouseMoved(int x, int y) {}
void ofApp::mouseDragged(int x, int y, int button) {}
void ofApp::mousePressed(int x, int y, int button) {}
void ofApp::mouseReleased(int x, int y, int button) {}
void ofApp::mouseEntered(int x, int y) {}
void ofApp::mouseExited(int x, int y) {}
void ofApp::windowResized(int w, int h) {}
void ofApp::dragEvent(ofDragInfo dragInfo) {}
void ofApp::gotMessage(ofMessage msg) {}