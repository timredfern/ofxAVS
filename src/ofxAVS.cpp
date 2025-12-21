// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "ofxAVS.h"
#include "AVSui.h"
#include "core/plugin_manager.h"
#include "core/builtin_effects.h"

ofxAVS::ofxAVS() {
    
}

ofxAVS::~ofxAVS() {
    // Clear effect chain to ensure proper cleanup
    renderer.reset();
    effect_chain.clear();
    control_states.clear();
}

void ofxAVS::setup() {
    width = 600;
    height = 600;
    
    // Initialize renderer
    renderer = std::make_unique<avs::DefaultRenderer>(width, height);
    
    // Initialize framebuffers
    framebuffer.resize(width * height, 0);
    output_buffer.resize(width * height, 0);
    
    // Initialize texture
    pixels.allocate(width, height, OF_PIXELS_RGBA);
    texture.allocate(pixels);
    
    // Register built-in effects
    avs::register_builtin_effects();
    
    // Initialize available effects list
    initializeAvailableEffects();
    
    // Add default effects to see something
    addEffectToChain("Clear");
    addEffectToChain("Oscilloscope");
}

void ofxAVS::update() {
    // Always render effects (even with silent audio data)
    renderer->render(current_audio_data, false, output_buffer.data());
    
    // Update texture
    std::memcpy(pixels.getData(), output_buffer.data(), width * height * sizeof(uint32_t));
    texture.loadData(pixels);
}

void ofxAVS::setAudioData(const avs::AudioData& data) {
    std::memcpy(current_audio_data, data, sizeof(avs::AudioData));
}

void ofxAVS::draw(int x, int y, int w, int h) {
    texture.draw(x, y, w, h);
}

void ofxAVS::drawUI() {
    drawChainPanel();
    drawAvailableEffectsPanel();
    drawParametersPanel();
}

void ofxAVS::addEffectToChain(const std::string& effectName) {
    // Find display name from available effects
    std::string displayName = effectName;
    for (const auto& effect : available_effects) {
        if (effect.name == effectName) {
            displayName = effect.display_name;
            break;
        }
    }
    
    effect_chain.emplace_back();
    auto& item = effect_chain.back();
    item.name = effectName;
    item.display_name = displayName;
    item.effect = avs::PluginManager::instance().create_effect(effectName);
    
    rebuildEffectChain();
    
    // Select the newly added effect
    selected_effect_index = static_cast<int>(effect_chain.size()) - 1;
    initializeParameterDefaults();
}

void ofxAVS::removeEffectFromChain(int index) {
    if (index >= 0 && index < effect_chain.size()) {
        effect_chain.erase(effect_chain.begin() + index);
        rebuildEffectChain();
        
        // Adjust selected index
        if (selected_effect_index >= effect_chain.size()) {
            selected_effect_index = static_cast<int>(effect_chain.size()) - 1;
        }
    }
}

void ofxAVS::moveEffectUp(int index) {
    if (index > 0 && index < effect_chain.size()) {
        std::swap(effect_chain[index], effect_chain[index - 1]);
        rebuildEffectChain();
        selected_effect_index = index - 1;
    }
}

void ofxAVS::moveEffectDown(int index) {
    if (index >= 0 && index < effect_chain.size() - 1) {
        std::swap(effect_chain[index], effect_chain[index + 1]);
        rebuildEffectChain();
        selected_effect_index = index + 1;
    }
}

void ofxAVS::setSelectedEffect(int index) {
    if (index >= -1 && index < effect_chain.size()) {
        selected_effect_index = index;
        if (index >= 0) {
            initializeParameterDefaults();
        }
    }
}

void ofxAVS::initializeAvailableEffects() {
    available_effects.clear();
    
    // Get all registered effects from plugin manager
    auto& pm = avs::PluginManager::instance();
    auto effect_names = pm.available_effects();
    
    for (const auto& name : effect_names) {
        AvailableEffectInfo info;
        info.name = name;
        
        // Get effect info which contains description
        auto plugin_info = pm.get_effect_info(name);
        info.display_name = plugin_info.name.empty() ? name : plugin_info.name;
        info.description = plugin_info.description.empty() ? "AVS effect" : plugin_info.description;
        
        // Try to get UI layout for parameter count
        const avs::EffectUILayout* layout = pm.get_ui_layout(name);
        if (layout) {
            auto controls = layout->getControls();
            if (!controls.empty()) {
                info.description += " (" + std::to_string(controls.size()) + " parameters)";
            }
        }
        
        available_effects.push_back(info);
    }
}

void ofxAVS::rebuildEffectChain() {
    renderer->clear_effects();
    
    for (auto& effect_item : effect_chain) {
        if (effect_item.effect) {
            // Clone the effect for the renderer since renderer takes ownership
            auto cloned_effect = avs::PluginManager::instance().create_effect(effect_item.name);
            if (cloned_effect) {
                // Copy parameters from UI effect to renderer effect
                // TODO: Implement parameter synchronization
                renderer->add_effect(std::move(cloned_effect));
            }
        }
    }
    
    updateEffectParameters();
}

void ofxAVS::updateEffectParameters() {
    // TODO: Implement parameter updates through renderer interface
    // For now, parameters will be set during effect creation
}

void ofxAVS::initializeParameterDefaults() {
    if (selected_effect_index >= 0 && selected_effect_index < effect_chain.size()) {
        const auto& effect_item = effect_chain[selected_effect_index];
        
        // Generic initialization based on UI layout
        const avs::EffectUILayout* layout = avs::PluginManager::instance().get_ui_layout(effect_item.name);
        if (layout) {
            auto controls = layout->getControls();
            for (const auto& control : controls) {
                std::string key = effect_item.name + "_" + control.id;
                if (control_states.find(key) == control_states.end()) {
                    control_states[key] = ParameterControlState(control.id);
                    
                    // Set defaults based on control type and range
                    switch (control.type) {
                        case avs::ControlType::SLIDER:
                            control_states[key].int_value = control.range.default_val;
                            control_states[key].float_value = (float)control.range.default_val;
                            break;
                        case avs::ControlType::CHECKBOX:
                        case avs::ControlType::RADIO_BUTTON:
                            control_states[key].bool_value = false;
                            break;
                        case avs::ControlType::COLOR_BUTTON:
                            control_states[key].color_value = ofColor(255, 255, 255);
                            break;
                        default:
                            break;
                    }
                }
            }
        }
        
        // Apply defaults to the effect
        updateEffectParameters();
    }
}

void ofxAVS::drawChainPanel() {
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(chain_panel_width, 400));
    
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    
    if (ImGui::Begin("Effect Chain", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        for (int i = 0; i < effect_chain.size(); i++) {
            const auto& item = effect_chain[i];
            bool is_selected = (i == selected_effect_index);

            // Selection highlighting
            if (is_selected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            }
            
            // Effect button - use index for unique ID
            std::string label = item.display_name + "##" + std::to_string(i);
            if (ImGui::Button(label.c_str(), ImVec2(chain_panel_width-15, 25))) {
                setSelectedEffect(i);
            }
            
            if (is_selected) {
                ImGui::PopStyleColor();
            }
            
            // Context menu for effect management
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                ImGui::OpenPopup(("effect_menu_" + std::to_string(i)).c_str());
            }
            
            if (ImGui::BeginPopup(("effect_menu_" + std::to_string(i)).c_str())) {
                if (ImGui::MenuItem("Move Up", nullptr, false, i > 0)) {
                    moveEffectUp(i);
                }
                if (ImGui::MenuItem("Move Down", nullptr, false, i < effect_chain.size() - 1)) {
                    moveEffectDown(i);
                }
                if (ImGui::MenuItem("Remove")) {
                    removeEffectFromChain(i);
                }
                ImGui::EndPopup();
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(3);
}

void ofxAVS::drawAvailableEffectsPanel() {
    int panel_x = chain_panel_width + 20;
    ImGui::SetNextWindowPos(ImVec2(panel_x, 10));
    ImGui::SetNextWindowSize(ImVec2(available_panel_width, 400));
    
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    
    if (ImGui::Begin("Available Effects", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        for (const auto& effect : available_effects) {
            if (ImGui::Button((effect.display_name + " +").c_str(), ImVec2(available_panel_width-15, 25))) {
                addEffectToChain(effect.name);
            }
            
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", effect.description.c_str());
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(3);
}


void ofxAVS::drawParametersPanel() {
    int panel_x = chain_panel_width + available_panel_width + 30;
    ImGui::SetNextWindowPos(ImVec2(panel_x, 10));
    ImGui::SetNextWindowSize(ImVec2(parameters_panel_width, 400));
    
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    
    if (ImGui::Begin("Parameters", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        if (selected_effect_index >= 0 && selected_effect_index < effect_chain.size()) {
            const auto& effect_item = effect_chain[selected_effect_index];
            
            // Get UI layout for this effect
            const avs::EffectUILayout* layout = avs::PluginManager::instance().get_ui_layout(effect_item.name);
            
            if (layout && effect_item.effect) {
                // Use the stored effect instance for UI rendering
                avs_ui::renderImGui(*layout, effect_item.effect.get());
            } else {
                ImGui::Text("No parameters available");
            }
        } else {
            ImGui::Text("Select an effect to see its parameters");
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(3);
}