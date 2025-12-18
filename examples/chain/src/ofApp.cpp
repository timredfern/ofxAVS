#include "ofApp.h"
#include "avs_lib/core/plugin_manager.h"

void ofApp::setup() {
    ofSetWindowTitle("ofxAVS Chain Example");
    ofSetFrameRate(60);
    ofSetWindowShape(1400, 600);  // Optimized width for 4-panel layout
    
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
    
    // UI layout setup - optimized for 1400px width with 4 panels
    int gutter = 20;
    chain_panel_width = 220;
    available_panel_width = 250;
    int available_height = 600 - 40;  // 560px (with top/bottom margins)
    parameters_panel_width = std::min(300, available_height);  // Make parameters panel square
    visualization_x = chain_panel_width + available_panel_width + parameters_panel_width + (gutter * 3);  // Three gutters: between panels + before viz
    visualization_y = 0;
    // Make visualization square, fitting the remaining width with equal gutter on right
    int remaining_width = 1400 - visualization_x - gutter;  // Remaining width after 3 panels + gutters
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
    
    // Setup ImGui
    gui.setup();
    
    // Start with a simple chain
    effect_chain.emplace_back("oscilloscope", "Oscilloscope");
    rebuildEffectChain();
    selected_effect_index = 0;
    
    ofLogNotice() << "ofxAVS Chain Example Started";
    ofLogNotice() << "CONTROLS:";
    ofLogNotice() << "  LEFT/RIGHT: Switch between Chain, Available Effects, and Parameters panels";
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
    
    // Start ImGui frame once per draw cycle
    gui.begin();
    
    // Draw effect chain panel
    drawEffectChain();
    
    // Draw available effects panel
    drawAvailableEffects();
    
    // Draw parameters panel
    drawParametersPanel();
    
    // Draw visualization
    drawVisualization();
    
    // End ImGui frame
    gui.end();
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

void ofApp::drawParametersPanel() {
    // Determine if this panel is active
    bool is_active = (current_panel == PARAMETERS_PANEL);
    
    int panel_x = chain_panel_width + available_panel_width + 40;
    
    // Draw panel background (make it square)
    ofSetColor(is_active ? 60 : 40);
    ofDrawRectangle(panel_x, 0, parameters_panel_width, parameters_panel_width);
    
    // Draw panel border (make it square)
    ofSetColor(is_active ? 150 : 80);
    ofNoFill();
    ofDrawRectangle(panel_x, 0, parameters_panel_width, parameters_panel_width);
    ofFill();
    
    if (selected_effect_index >= 0 && selected_effect_index < effect_chain.size()) {
        const auto& effect_item = effect_chain[selected_effect_index];
        
        // Get UI layout for this effect
        const avs::EffectUILayout* layout = avs::UILayoutRegistry::instance().getLayout(effect_item.name);
        
        if (layout) {
            // Get the controls
            auto controls = layout->getControls();
            
            // Position ImGui window to match our panel
            ImGui::SetNextWindowPos(ImVec2(panel_x + 10, 20));
            ImGui::SetNextWindowSize(ImVec2(parameters_panel_width - 20, parameters_panel_width - 40));
            
            std::string window_title = "Parameters: " + effect_item.display_name;
            ImGui::Begin(window_title.c_str(), nullptr, 
                ImGuiWindowFlags_NoResize | 
                ImGuiWindowFlags_NoMove | 
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoTitleBar);
            
            // Add title text manually since we disabled the title bar
            ImGui::Text("Parameters: %s", effect_item.display_name.c_str());
            ImGui::Separator();
            
            // Create interactive controls
            for (size_t i = 0; i < controls.size(); i++) {
                const auto& control = controls[i];
                
                // Push unique ID for this control
                ImGui::PushID((effect_item.name + "_" + control.id + "_" + std::to_string(i)).c_str());
                
                // Ensure control state exists
                std::string state_key = effect_item.name + "_" + control.id;
                if (control_states.find(state_key) == control_states.end()) {
                    control_states[state_key] = ParameterControlState(control.id);
                    // Initialize with proper default value from the control range
                    control_states[state_key].int_value = control.range.default_val;
                    control_states[state_key].float_value = (float)control.range.default_val;
                }
                
                auto& state = control_states[state_key];
                bool changed = false;
                
                // Create appropriate ImGui control based on type
                switch (control.type) {
                    case avs::ControlType::SLIDER:
                        // Style the slider for better visibility
                        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(50, 50, 50, 255));        // Dark background
                        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(60, 60, 60, 255)); // Slightly lighter on hover
                        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(70, 70, 70, 255));   // Even lighter when active
                        ImGui::PushStyleColor(ImGuiCol_SliderGrab, IM_COL32(150, 150, 150, 255));   // Light gray grab handle
                        ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, IM_COL32(255, 255, 255, 255)); // White when dragging
                        
                        ImGui::Text("%s", control.text.c_str());
                        ImGui::SetNextItemWidth(200); // Force width for proper slider appearance
                        changed = ImGui::SliderInt(("##" + control.id).c_str(), 
                            &state.int_value, control.range.min, control.range.max, "%d");
                        
                        ImGui::PopStyleColor(5); // Pop all 5 style colors
                        
                        // Keep float value in sync for compatibility
                        if (changed) {
                            state.float_value = (float)state.int_value;
                        }
                        break;
                        
                    case avs::ControlType::CHECKBOX:
                        changed = ImGui::Checkbox(control.text.c_str(), &state.bool_value);
                        break;
                        
                    case avs::ControlType::RADIO_BUTTON:
                        // Group radio buttons by their Y position (same row)
                        if (ImGui::RadioButton(control.text.c_str(), state.bool_value)) {
                            // Turn off other radio buttons in the same group
                            for (const auto& other_control : controls) {
                                if (other_control.type == avs::ControlType::RADIO_BUTTON && 
                                    abs(other_control.y - control.y) < 5) {  // Same row
                                    std::string other_key = effect_item.name + "_" + other_control.id;
                                    if (other_key != state_key && 
                                        control_states.find(other_key) != control_states.end()) {
                                        control_states[other_key].bool_value = false;
                                    }
                                }
                            }
                            state.bool_value = true;
                            changed = true;
                        }
                        break;
                        
                    case avs::ControlType::COLOR_BUTTON:
                        {
                            float color[3] = {
                                state.color_value.r / 255.0f,
                                state.color_value.g / 255.0f, 
                                state.color_value.b / 255.0f
                            };
                            if (ImGui::ColorEdit3(control.text.c_str(), color)) {
                                state.color_value.set(
                                    color[0] * 255, 
                                    color[1] * 255, 
                                    color[2] * 255
                                );
                                changed = true;
                            }
                        }
                        break;
                        
                    case avs::ControlType::BUTTON:
                        if (ImGui::Button(control.text.c_str())) {
                            // Handle button press (e.g., reset values)
                            if (control.id == "reset" || control.text.find("Reset") != std::string::npos) {
                                // Reset all values for this effect
                                for (auto& pair : control_states) {
                                    if (pair.first.find(effect_item.name + "_") == 0) {
                                        pair.second.float_value = 0.5f;
                                        pair.second.bool_value = false;
                                        pair.second.int_value = 0;
                                    }
                                }
                                changed = true;
                            }
                        }
                        break;
                        
                    case avs::ControlType::TEXT_INPUT:
                        {
                            ImGui::Text("%s", control.text.c_str());
                            // Create a buffer for text input if it doesn't exist
                            static std::map<std::string, std::string> text_buffers;
                            std::string buffer_key = state_key + "_text";
                            if (text_buffers.find(buffer_key) == text_buffers.end()) {
                                text_buffers[buffer_key] = ""; // Initialize empty
                            }
                            
                            // ImGui needs a char buffer, so we'll use a fixed size buffer
                            static char input_buffer[1024];
                            strncpy(input_buffer, text_buffers[buffer_key].c_str(), sizeof(input_buffer)-1);
                            input_buffer[sizeof(input_buffer)-1] = '\0';
                            
                            if (control.h > 20) {
                                // Multi-line text input for taller controls
                                if (ImGui::InputTextMultiline("##textinput", input_buffer, sizeof(input_buffer), 
                                    ImVec2(-1, (float)control.h))) {
                                    text_buffers[buffer_key] = std::string(input_buffer);
                                    changed = true;
                                }
                            } else {
                                // Single-line text input
                                if (ImGui::InputText("##textinput", input_buffer, sizeof(input_buffer))) {
                                    text_buffers[buffer_key] = std::string(input_buffer);
                                    changed = true;
                                }
                            }
                        }
                        break;
                        
                    case avs::ControlType::DROPDOWN:
                        {
                            ImGui::Text("%s", control.text.c_str());
                            // Simple dropdown with placeholder options
                            static std::map<std::string, int> dropdown_selections;
                            std::string dropdown_key = state_key + "_dropdown";
                            if (dropdown_selections.find(dropdown_key) == dropdown_selections.end()) {
                                dropdown_selections[dropdown_key] = 0;
                            }
                            
                            const char* dropdown_items[] = { "Option 1", "Option 2", "Option 3" };
                            int current_item = dropdown_selections[dropdown_key];
                            
                            if (ImGui::Combo("##dropdown", &current_item, dropdown_items, IM_ARRAYSIZE(dropdown_items))) {
                                dropdown_selections[dropdown_key] = current_item;
                                state.int_value = current_item;
                                changed = true;
                            }
                        }
                        break;
                }
                
                // Pop the unique ID
                ImGui::PopID();
                
                // Update effect parameters if control changed
                if (changed) {
                    updateEffectParameters();
                }
            }
            
            ImGui::End();
            
        } else {
            ofSetColor(150);
            ofDrawBitmapString("No UI layout found for: " + effect_item.name, panel_x + 10, 40);
        }
    } else {
        // Draw title when no effect selected
        ofSetColor(is_active ? 255 : 180);
        ofDrawBitmapString("Parameters", panel_x + 10, 20);
        ofSetColor(150);
        ofDrawBitmapString("Select an effect to view parameters", panel_x + 10, 40);
    }
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
    ofDrawBitmapString("FPS: " + ofToString(ofGetFrameRate(), 1), visualization_x, visualization_y + visualization_size + 15);
    ofDrawBitmapString("Beat: " + std::string(visualizer.isBeat() ? "YES" : "NO"), visualization_x, visualization_y + visualization_size + 30);
}

void ofApp::keyPressed(int key) {
    switch(key) {
        case OF_KEY_LEFT:
            if (current_panel == AVAILABLE_PANEL) {
                current_panel = CHAIN_PANEL;
            } else if (current_panel == PARAMETERS_PANEL) {
                current_panel = AVAILABLE_PANEL;
            }
            break;
            
        case OF_KEY_RIGHT:
            if (current_panel == CHAIN_PANEL) {
                current_panel = AVAILABLE_PANEL;
            } else if (current_panel == AVAILABLE_PANEL) {
                current_panel = PARAMETERS_PANEL;
            }
            break;
            
        case OF_KEY_UP:
            if (ofGetKeyPressed(OF_KEY_SHIFT) && current_panel == CHAIN_PANEL) {
                moveEffectUp();
            } else if (current_panel == CHAIN_PANEL) {
                if (selected_effect_index > 0) {
                    selected_effect_index--;
                    initializeParameterDefaults();
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
                    initializeParameterDefaults();
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
        initializeParameterDefaults();
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
    
    for (auto& item : effect_chain) {  // Changed to non-const reference
        if (item.enabled) {
            auto effect = visualizer.addEffect(item.name);
            item.effect_ptr = effect;  // Store effect pointer
            
            // Set some default parameters for common effects
            if (item.name == "brightness" && effect) {
                effect->parameters().set_int("brightness", -1);
            }
        } else {
            item.effect_ptr = nullptr;  // Clear pointer for disabled effects
        }
    }
}

void ofApp::updateEffectParameters() {
    if (selected_effect_index >= 0 && selected_effect_index < effect_chain.size()) {
        const auto& effect_item = effect_chain[selected_effect_index];
        
        if (effect_item.effect_ptr) {
            // Update effect parameters based on control states
            for (const auto& pair : control_states) {
                if (pair.first.find(effect_item.name + "_") == 0) {
                    std::string param_id = pair.second.control_id;
                    const auto& state = pair.second;
                    
                    // Map control IDs to actual effect parameters
                    if (effect_item.name == "brightness") {
                        if (param_id == "enabled") {
                            effect_item.effect_ptr->parameters().set_bool("enabled", state.bool_value);
                        } else if (param_id == "red_adjust") {
                            // Use integer value directly
                            effect_item.effect_ptr->parameters().set_int("red_adjust", state.int_value);
                        } else if (param_id == "green_adjust") {
                            effect_item.effect_ptr->parameters().set_int("green_adjust", state.int_value);
                        } else if (param_id == "blue_adjust") {
                            effect_item.effect_ptr->parameters().set_int("blue_adjust", state.int_value);
                        } else if (param_id == "dissoc") {
                            effect_item.effect_ptr->parameters().set_bool("dissoc", state.bool_value);
                        } else if (param_id == "replace" && state.bool_value) {
                            effect_item.effect_ptr->parameters().set_string("blend", "replace");
                        } else if (param_id == "additive" && state.bool_value) {
                            effect_item.effect_ptr->parameters().set_string("blend", "additive");
                        } else if (param_id == "5050" && state.bool_value) {
                            effect_item.effect_ptr->parameters().set_string("blend", "5050");
                        }
                    } else if (effect_item.name == "oscilloscope") {
                        if (param_id == "color") {
                            // Set oscilloscope color with full alpha
                            uint32_t color = 0xFF000000 | (state.color_value.r << 16) | 
                                           (state.color_value.g << 8) | 
                                            state.color_value.b;
                            effect_item.effect_ptr->parameters().set_color("color", color);
                        } else if (param_id == "channel_left") {
                            if (state.bool_value) {
                                effect_item.effect_ptr->parameters().set_int("channel", 0);
                            }
                        } else if (param_id == "channel_right") {
                            if (state.bool_value) {
                                effect_item.effect_ptr->parameters().set_int("channel", 1);
                            }
                        } else if (param_id == "solid") {
                            effect_item.effect_ptr->parameters().set_bool("solid", state.bool_value);
                        }
                    } else if (effect_item.name == "clear") {
                        if (param_id == "color") {
                            uint32_t color = (state.color_value.r << 16) | 
                                           (state.color_value.g << 8) | 
                                            state.color_value.b;
                            effect_item.effect_ptr->parameters().set_int("color", color);
                        } else if (param_id == "blend_replace" && state.bool_value) {
                            effect_item.effect_ptr->parameters().set_string("blend", "replace");
                        } else if (param_id == "blend_additive" && state.bool_value) {
                            effect_item.effect_ptr->parameters().set_string("blend", "additive");
                        } else if (param_id == "blend_5050" && state.bool_value) {
                            effect_item.effect_ptr->parameters().set_string("blend", "5050");
                        } else if (param_id == "only_first") {
                            effect_item.effect_ptr->parameters().set_bool("only_first", state.bool_value);
                        }
                    }
                }
            }
        }
    }
}

void ofApp::initializeParameterDefaults() {
    if (selected_effect_index >= 0 && selected_effect_index < effect_chain.size()) {
        const auto& effect_item = effect_chain[selected_effect_index];
        
        // Initialize default values based on effect type
        if (effect_item.name == "brightness") {
            // Initialize all brightness controls
            std::vector<std::string> controls = {"enabled", "red_adjust", "green_adjust", "blue_adjust", "dissoc", "replace", "additive", "5050"};
            for (const std::string& ctrl : controls) {
                std::string key = effect_item.name + "_" + ctrl;
                if (control_states.find(key) == control_states.end()) {
                    control_states[key] = ParameterControlState(ctrl);
                    if (ctrl == "enabled") {
                        control_states[key].bool_value = true;
                    } else if (ctrl.find("_adjust") != std::string::npos) {
                        control_states[key].float_value = 0.0f;  // Will be set to proper default when layout is loaded
                    } else if (ctrl == "replace") {
                        control_states[key].bool_value = true;  // Default blend mode
                    } else {
                        control_states[key].bool_value = false;
                    }
                }
            }
        } else if (effect_item.name == "oscilloscope") {
            std::string color_key = effect_item.name + "_color";
            std::string left_key = effect_item.name + "_channel_left"; 
            std::string right_key = effect_item.name + "_channel_right";
            std::string solid_key = effect_item.name + "_solid";
            
            if (control_states.find(color_key) == control_states.end()) {
                control_states[color_key] = ParameterControlState("color");
                control_states[color_key].color_value = ofColor(255, 255, 255);
            }
            if (control_states.find(left_key) == control_states.end()) {
                control_states[left_key] = ParameterControlState("channel_left");
                control_states[left_key].bool_value = true;  // Default to left channel
            }
            if (control_states.find(right_key) == control_states.end()) {
                control_states[right_key] = ParameterControlState("channel_right");
                control_states[right_key].bool_value = false;
            }
            if (control_states.find(solid_key) == control_states.end()) {
                control_states[solid_key] = ParameterControlState("solid");
                control_states[solid_key].bool_value = false;
            }
        } else if (effect_item.name == "clear") {
            std::string color_key = effect_item.name + "_color";
            std::string replace_key = effect_item.name + "_blend_replace";
            std::string additive_key = effect_item.name + "_blend_additive";
            std::string five_key = effect_item.name + "_blend_5050";
            std::string first_key = effect_item.name + "_only_first";
            
            if (control_states.find(color_key) == control_states.end()) {
                control_states[color_key] = ParameterControlState("color");
                control_states[color_key].color_value = ofColor(0, 0, 0);  // Default black
            }
            if (control_states.find(replace_key) == control_states.end()) {
                control_states[replace_key] = ParameterControlState("blend_replace");
                control_states[replace_key].bool_value = true;  // Default blend mode
            }
            if (control_states.find(additive_key) == control_states.end()) {
                control_states[additive_key] = ParameterControlState("blend_additive");
                control_states[additive_key].bool_value = false;
            }
            if (control_states.find(five_key) == control_states.end()) {
                control_states[five_key] = ParameterControlState("blend_5050");
                control_states[five_key].bool_value = false;
            }
            if (control_states.find(first_key) == control_states.end()) {
                control_states[first_key] = ParameterControlState("only_first");
                control_states[first_key].bool_value = false;
            }
        }
        
        // Apply defaults to the effect
        updateEffectParameters();
    }
}

void ofApp::handleParameterMousePressed(int x, int y) {
    // Mouse handling is now managed by ImGui
}

void ofApp::handleParameterMouseDragged(int x, int y) {
    // Mouse handling is now managed by ImGui  
}

bool ofApp::isPointInParametersPanel(int x, int y) {
    int panel_x = chain_panel_width + available_panel_width + 40;
    return (x >= panel_x && x < panel_x + parameters_panel_width && 
            y >= 0 && y < parameters_panel_width);
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