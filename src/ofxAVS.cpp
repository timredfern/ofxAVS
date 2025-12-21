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

    // Create effect and add directly to renderer
    auto new_effect = avs::PluginManager::instance().create_effect(effectName);
    if (new_effect) {
        renderer->add_effect(std::move(new_effect));

        // Add metadata to effect_chain
        effect_chain.push_back({effectName, displayName});

        // Select the newly added effect
        selected_effect_index = static_cast<int>(effect_chain.size()) - 1;
    }
}

void ofxAVS::removeEffectFromChain(int index) {
    if (index >= 0 && index < static_cast<int>(effect_chain.size())) {
        renderer->remove_effect(static_cast<size_t>(index));
        effect_chain.erase(effect_chain.begin() + index);

        // Adjust selected index
        if (selected_effect_index >= static_cast<int>(effect_chain.size())) {
            selected_effect_index = static_cast<int>(effect_chain.size()) - 1;
        }
    }
}

void ofxAVS::moveEffectUp(int index) {
    if (index > 0 && index < static_cast<int>(effect_chain.size())) {
        renderer->swap_effects(static_cast<size_t>(index), static_cast<size_t>(index - 1));
        std::swap(effect_chain[index], effect_chain[index - 1]);
        selected_effect_index = index - 1;
    }
}

void ofxAVS::moveEffectDown(int index) {
    if (index >= 0 && index < static_cast<int>(effect_chain.size()) - 1) {
        renderer->swap_effects(static_cast<size_t>(index), static_cast<size_t>(index + 1));
        std::swap(effect_chain[index], effect_chain[index + 1]);
        selected_effect_index = index + 1;
    }
}

void ofxAVS::setSelectedEffect(int index) {
    if (index >= -1 && index < static_cast<int>(effect_chain.size())) {
        selected_effect_index = index;
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
        if (selected_effect_index >= 0 && selected_effect_index < static_cast<int>(effect_chain.size())) {
            const auto& effect_item = effect_chain[selected_effect_index];

            // Get the actual effect from renderer and UI layout
            avs::EffectBase* effect = renderer->get_effect(static_cast<size_t>(selected_effect_index));
            const avs::EffectUILayout* layout = avs::PluginManager::instance().get_ui_layout(effect_item.name);

            if (layout && effect) {
                // Render UI directly modifying the renderer's effect
                avs_ui::renderImGui(*layout, effect);
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