#include "ofApp.h"
#include "avs_lib/core/plugin_manager.h"

void ofApp::setup() {
    ofSetWindowTitle("ofxAVS Chain Example");
    ofSetFrameRate(60);
    ofSetWindowShape(1000, 600);  // Optimized width for 3-panel layout
    
    // Audio setup
    sample_rate = 44100;
    buffer_size = 512;
    num_input_channels = 1;
    
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
    
    // UI layout setup - optimized for 1000px width
    int gutter = 20;
    chain_panel_width = 220;
    available_panel_width = 250;
    visualization_x = chain_panel_width + available_panel_width + (gutter * 2);  // Two gutters: between panels + before viz
    visualization_y = 20;
    // Make visualization square, fitting the remaining width with equal gutter on right
    int remaining_width = 1000 - visualization_x - gutter;  // 1000 - 490 - 20 = 490px remaining
    int available_height = 600 - 40;  // 560px (with top/bottom margins)
    visualization_size = std::min(remaining_width, available_height);  // Square that fits
    
    // Effect chain setup
    selected_effect_index = -1;
    selected_available_index = 0;
    current_panel = CHAIN_PANEL;
    
    // Available effects setup with descriptions
    available_effects = {
        {"clear", "Clear", "Fills the screen with a solid color. Use for fading/trails."},
        {"brightness", "Brightness", "Adjusts overall brightness. Negative values darken."},
        {"oscilloscope", "Oscilloscope", "Draws audio waveform as dots or lines."},
        {"dynamic_movement", "Dynamic Movement", "Pixel transformation using custom expressions."},
        {"movement", "Movement", "Simple coordinate transformations and rotations."},
        {"blur", "Blur", "Applies blur effect to smooth the image."}
    };
    
    // Start with a simple chain
    effect_chain.emplace_back("oscilloscope", "Oscilloscope");
    rebuildEffectChain();
    selected_effect_index = 0;
    
    ofLogNotice() << "ofxAVS Chain Example Started";
    ofLogNotice() << "CONTROLS:";
    ofLogNotice() << "  LEFT/RIGHT: Switch between Chain and Available Effects panels";
    ofLogNotice() << "  UP/DOWN: Navigate within current panel";
    ofLogNotice() << "  ENTER: Add selected effect to chain";
    ofLogNotice() << "  DEL: Remove selected effect from chain";
    ofLogNotice() << "  SPACE: Toggle effect enabled/disabled";
    ofLogNotice() << "  SHIFT+UP/DOWN: Move effect up/down in chain";
}

void ofApp::update() {
    visualizer.update();
}

void ofApp::draw() {
    ofBackground(30);
    
    // Draw effect chain panel
    drawEffectChain();
    
    // Draw available effects panel
    drawAvailableEffects();
    
    // Draw visualization
    drawVisualization();
}

void ofApp::drawEffectChain() {
    // Determine if this panel is active
    bool is_active = (current_panel == CHAIN_PANEL);
    
    // Draw panel background
    ofSetColor(is_active ? 60 : 40);
    ofDrawRectangle(0, 0, chain_panel_width, ofGetHeight());
    
    // Draw panel border
    ofSetColor(is_active ? 150 : 80);
    ofNoFill();
    ofDrawRectangle(0, 0, chain_panel_width, ofGetHeight());
    ofFill();
    
    // Draw title
    ofSetColor(is_active ? 255 : 180);
    ofDrawBitmapString("Effect Chain", 10, 20);
    ofDrawBitmapString("(" + std::to_string(effect_chain.size()) + " effects)", 10, 35);
    
    // Draw effect chain
    int y = 60;
    for (size_t i = 0; i < effect_chain.size(); i++) {
        bool is_selected = (int)i == selected_effect_index && is_active;
        
        // Background for selected item
        if (is_selected) {
            ofSetColor(80, 80, 150);
            ofDrawRectangle(5, y - 15, chain_panel_width - 10, 20);
        }
        
        // Effect enabled/disabled indicator
        if (effect_chain[i].enabled) {
            ofSetColor(100, 255, 100);  // Green for enabled
        } else {
            ofSetColor(150, 150, 150);  // Gray for disabled
        }
        ofDrawCircle(15, y - 5, 3);
        
        // Effect name with text wrapping
        if (is_selected) {
            ofSetColor(255, 255, 100);  // Yellow for selected
        } else if (effect_chain[i].enabled) {
            ofSetColor(255);            // White for enabled
        } else {
            ofSetColor(150);            // Gray for disabled
        }
        
        std::string display_text = std::to_string(i + 1) + ". " + effect_chain[i].display_name;
        
        // Wrap text if it's too long (about 20 characters fits in 200px)
        int max_chars = 20;
        if (display_text.length() > max_chars) {
            std::string first_line = display_text.substr(0, max_chars);
            std::string second_line = "   " + display_text.substr(max_chars);  // Indent continuation
            ofDrawBitmapString(first_line, 25, y);
            ofDrawBitmapString(second_line, 25, y + 12);
            y += 12;  // Extra spacing for wrapped line
        } else {
            ofDrawBitmapString(display_text, 25, y);
        }
        
        // Draw connection line to next effect
        if (i < effect_chain.size() - 1) {
            ofSetColor(100);
            ofDrawLine(15, y + 5, 15, y + 15);
            ofDrawCircle(15, y + 10, 2);
        }
        
        y += 25;
    }
    
}

void ofApp::drawAvailableEffects() {
    // Determine if this panel is active
    bool is_active = (current_panel == AVAILABLE_PANEL);
    
    int panel_x = chain_panel_width + 20;
    
    // Draw panel background
    ofSetColor(is_active ? 60 : 40);
    ofDrawRectangle(panel_x, 0, available_panel_width, ofGetHeight());
    
    // Draw panel border
    ofSetColor(is_active ? 150 : 80);
    ofNoFill();
    ofDrawRectangle(panel_x, 0, available_panel_width, ofGetHeight());
    ofFill();
    
    // Draw title
    ofSetColor(is_active ? 255 : 180);
    ofDrawBitmapString("Available Effects", panel_x + 10, 20);
    ofDrawBitmapString("Select and press ENTER to add", panel_x + 10, 35);
    
    // Draw available effects list
    int y = 60;
    for (size_t i = 0; i < available_effects.size(); i++) {
        bool is_selected = (int)i == selected_available_index && is_active;
        
        // Background for selected item
        if (is_selected) {
            ofSetColor(80, 150, 80);
            ofDrawRectangle(panel_x + 5, y - 15, available_panel_width - 10, 40);
        }
        
        // Effect name
        if (is_selected) {
            ofSetColor(255, 255, 100);  // Yellow for selected
        } else {
            ofSetColor(is_active ? 255 : 180);  // White for active panel, gray for inactive
        }
        
        ofDrawBitmapString(available_effects[i].display_name, panel_x + 15, y);
        
        // Effect description with proper wrapping
        ofSetColor(is_active ? 200 : 120);
        std::string desc = available_effects[i].description;
        
        // Wrap description text (about 30 characters fits in 250px)
        int desc_max_chars = 30;
        if (desc.length() > desc_max_chars) {
            // Find a good break point near the limit
            size_t break_pos = desc.rfind(' ', desc_max_chars);
            if (break_pos == std::string::npos || break_pos < desc_max_chars * 0.7) {
                break_pos = desc_max_chars;
            }
            
            std::string first_line = desc.substr(0, break_pos);
            std::string second_line = desc.substr(break_pos);
            
            // Trim leading space from second line
            if (!second_line.empty() && second_line[0] == ' ') {
                second_line = second_line.substr(1);
            }
            
            ofDrawBitmapString(first_line, panel_x + 15, y + 12);
            
            // Truncate second line if still too long
            if (second_line.length() > desc_max_chars) {
                second_line = second_line.substr(0, desc_max_chars - 3) + "...";
            }
            ofDrawBitmapString(second_line, panel_x + 15, y + 24);
        } else {
            ofDrawBitmapString(desc, panel_x + 15, y + 12);
        }
        
        y += 45;
    }
    
    // Draw controls
    ofSetColor(is_active ? 200 : 120);
    int controls_y = ofGetHeight() - 120;
    ofDrawBitmapString("Controls:", panel_x + 10, controls_y);
    ofDrawBitmapString("LEFT/RIGHT: Switch panels", panel_x + 10, controls_y + 15);
    ofDrawBitmapString("UP/DOWN: Navigate", panel_x + 10, controls_y + 30);
    ofDrawBitmapString("ENTER: Add to chain", panel_x + 10, controls_y + 45);
    ofDrawBitmapString("DEL: Remove from chain", panel_x + 10, controls_y + 60);
    ofDrawBitmapString("SPACE: Toggle enabled", panel_x + 10, controls_y + 75);
}

void ofApp::drawVisualization() {
    // Draw visualization background
    ofSetColor(20);
    ofDrawRectangle(visualization_x, visualization_y, visualization_size, visualization_size);
    
    // Draw visualization border
    ofSetColor(100);
    ofNoFill();
    ofDrawRectangle(visualization_x, visualization_y, visualization_size, visualization_size);
    ofFill();
    
    // Draw the actual AVS visualization
    ofSetColor(255);  // Reset to full white before drawing texture
    visualizer.draw(visualization_x, visualization_y, visualization_size, visualization_size);
    
    // Draw info
    ofSetColor(255);
    ofDrawBitmapString("Visualization Output", visualization_x, visualization_y - 10);
    ofDrawBitmapString("FPS: " + ofToString(ofGetFrameRate(), 1), visualization_x, visualization_y + visualization_size + 20);
    ofDrawBitmapString("Beat: " + std::string(visualizer.isBeat() ? "YES" : "NO"), visualization_x, visualization_y + visualization_size + 35);
}

void ofApp::keyPressed(int key) {
    switch(key) {
        case OF_KEY_LEFT:
            current_panel = CHAIN_PANEL;
            break;
            
        case OF_KEY_RIGHT:
            current_panel = AVAILABLE_PANEL;
            break;
            
        case OF_KEY_UP:
            if (ofGetKeyPressed(OF_KEY_SHIFT) && current_panel == CHAIN_PANEL) {
                moveEffectUp();
            } else if (current_panel == CHAIN_PANEL) {
                if (selected_effect_index > 0) {
                    selected_effect_index--;
                }
            } else if (current_panel == AVAILABLE_PANEL) {
                if (selected_available_index > 0) {
                    selected_available_index--;
                }
            }
            break;
            
        case OF_KEY_DOWN:
            if (ofGetKeyPressed(OF_KEY_SHIFT) && current_panel == CHAIN_PANEL) {
                moveEffectDown();
            } else if (current_panel == CHAIN_PANEL) {
                if (selected_effect_index < (int)effect_chain.size() - 1) {
                    selected_effect_index++;
                }
            } else if (current_panel == AVAILABLE_PANEL) {
                if (selected_available_index < (int)available_effects.size() - 1) {
                    selected_available_index++;
                }
            }
            break;
            
        case OF_KEY_RETURN:
            if (current_panel == AVAILABLE_PANEL) {
                addSelectedEffectToChain();
            }
            break;
            
        case OF_KEY_DEL:
        case OF_KEY_BACKSPACE:
            if (current_panel == CHAIN_PANEL) {
                removeSelectedEffect();
            }
            break;
            
        case ' ':
            if (current_panel == CHAIN_PANEL) {
                toggleEffectEnabled();
            }
            break;
    }
}

void ofApp::audioIn(ofSoundBuffer& buffer) {
    visualizer.audioReceived(buffer.getBuffer().data(), buffer.getNumFrames(), buffer.getNumChannels());
}

void ofApp::addSelectedEffectToChain() {
    if (selected_available_index >= 0 && selected_available_index < (int)available_effects.size()) {
        const auto& effect = available_effects[selected_available_index];
        
        effect_chain.emplace_back(effect.name, effect.display_name);
        rebuildEffectChain();
        
        // Select the newly added effect and switch to chain panel
        selected_effect_index = effect_chain.size() - 1;
        current_panel = CHAIN_PANEL;
    }
}

void ofApp::removeSelectedEffect() {
    if (selected_effect_index >= 0 && selected_effect_index < (int)effect_chain.size()) {
        effect_chain.erase(effect_chain.begin() + selected_effect_index);
        
        // Adjust selection
        if (selected_effect_index >= (int)effect_chain.size()) {
            selected_effect_index = effect_chain.size() - 1;
        }
        
        rebuildEffectChain();
    }
}

void ofApp::moveEffectUp() {
    if (selected_effect_index > 0 && selected_effect_index < (int)effect_chain.size()) {
        std::swap(effect_chain[selected_effect_index], effect_chain[selected_effect_index - 1]);
        selected_effect_index--;
        rebuildEffectChain();
    }
}

void ofApp::moveEffectDown() {
    if (selected_effect_index >= 0 && selected_effect_index < (int)effect_chain.size() - 1) {
        std::swap(effect_chain[selected_effect_index], effect_chain[selected_effect_index + 1]);
        selected_effect_index++;
        rebuildEffectChain();
    }
}

void ofApp::toggleEffectEnabled() {
    if (selected_effect_index >= 0 && selected_effect_index < (int)effect_chain.size()) {
        effect_chain[selected_effect_index].enabled = !effect_chain[selected_effect_index].enabled;
        rebuildEffectChain();
    }
}

void ofApp::rebuildEffectChain() {
    visualizer.clearEffects();
    
    for (const auto& item : effect_chain) {
        if (item.enabled) {
            auto effect = visualizer.addEffect(item.name);
            // Set some default parameters for common effects
            if (item.name == "brightness" && effect) {
                effect->parameters().set_int("brightness", -1);
            }
        }
    }
}

// Unused event handlers
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