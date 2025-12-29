// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "ofxAVS.h"
#include "AVSui.h"
#include "core/plugin_manager.h"
#include "core/builtin_effects.h"
#include <cmath>

ofxAVS::ofxAVS() : fft(nullptr) {
#ifdef AVS_ENHANCED_FFT
    memset(smoothedSpectrum, 0, sizeof(smoothedSpectrum));
#else
    // Initialize AVS log table (base ~60 compression)
    // Formula: log(x * 60/255 + 1) / log(60) * 255
    for (int x = 0; x < 256; x++) {
        double a = log(x * 60.0 / 255.0 + 1.0) / log(60.0);
        int t = static_cast<int>(a * 255.0);
        if (t < 0) t = 0;
        if (t > 255) t = 255;
        logTable[x] = static_cast<unsigned char>(t);
    }
#endif
}

ofxAVS::~ofxAVS() {
    // Clear effect chain to ensure proper cleanup
    renderer.reset();
    effect_chain.clear();

    // Clean up FFT
    if (fft) {
        delete fft;
        fft = nullptr;
    }
}

void ofxAVS::setup() {
    width = 600;
    height = 600;

    // Initialize FFT
#ifdef AVS_ENHANCED_FFT
    fft = ofxFft::create(FFT_SIZE, OF_FFT_WINDOW_HAMMING);
#else
    // Original Winamp used Hann window
    fft = ofxFft::create(FFT_SIZE, OF_FFT_WINDOW_HANN);
#endif

    // Initialize renderer
    renderer = std::make_unique<avs::DefaultRenderer>(width, height);

    // Initialize texture - use BGRA to match our uint32_t ARGB format
    pixels.allocate(width, height, OF_PIXELS_BGRA);
    texture.allocate(pixels);
    
    // Register built-in effects
    avs::register_builtin_effects();
    
    // Initialize available effects list
    initializeAvailableEffects();
    
    // Add default effects to see something
    addEffectToChain("Brightness");
    addEffectToChain("Oscilloscope");
}

void ofxAVS::update() {
    // Render directly into pixels buffer (no intermediate copy)
    renderer->render(current_audio_data, false,
                     reinterpret_cast<uint32_t*>(pixels.getData()));
    texture.loadData(pixels);
}

void ofxAVS::audioIn(ofSoundBuffer& buffer) {
    memset(current_audio_data, 0, sizeof(avs::AudioData));

    int numChannels = buffer.getNumChannels();
    int numSamples = std::min(static_cast<int>(buffer.getNumFrames()), 576);

    static vector<float> fftSamples(FFT_SIZE);

    // Process waveform and prepare FFT input
    for (int i = 0; i < numSamples; i++) {
        float left = buffer[i * numChannels];
        float right = (numChannels >= 2) ? buffer[i * numChannels + 1] : left;

        // Waveform: convert float [-1, 1] to signed char [-128, 127]
        current_audio_data[0][0][i] = static_cast<char>(left * 127.0f);
        current_audio_data[0][1][i] = static_cast<char>(right * 127.0f);

        // Mix to mono for FFT
        if (i < FFT_SIZE) {
            fftSamples[i] = (left + right) * 0.5f;
        }
    }

    // Zero-pad FFT input if needed
    for (int i = numSamples; i < FFT_SIZE; i++) {
        fftSamples[i] = 0;
    }

    // Compute FFT spectrum
    fft->setSignal(fftSamples);
    float* amplitude = fft->getAmplitude();
    int binSize = fft->getBinSize();

#ifdef AVS_ENHANCED_FFT
    // ========== ENHANCED MODE ==========
    // Higher resolution FFT with smoothing and dB scale

    // Smoothing constants
    const float attack = 0.8f;   // How fast values rise
    const float decay = 0.4f;    // How fast values fall (slower = smoother)

    // Convert spectrum to AVS format (576 bins, 0-255 unsigned char)
    // Use linear interpolation and temporal smoothing
    for (int i = 0; i < 576; i++) {
        // Map output bin to FFT bin with interpolation
        float srcPos = (float)i * (binSize - 1) / 575.0f;
        int srcIdx = (int)srcPos;
        float frac = srcPos - srcIdx;
        if (srcIdx >= binSize - 1) {
            srcIdx = binSize - 2;
            frac = 1.0f;
        }

        // Linear interpolation between adjacent bins
        float mag = amplitude[srcIdx] * (1.0f - frac) + amplitude[srcIdx + 1] * frac;

        // Log scale (dB) with adjusted range
        float db = 20.0f * log10f(mag + 0.00001f);
        float normalized = (db + 80.0f) / 80.0f;  // Wider dynamic range
        if (normalized < 0) normalized = 0;
        if (normalized > 1) normalized = 1;

        // Temporal smoothing (attack/decay envelope)
        float target = normalized;
        if (target > smoothedSpectrum[i]) {
            smoothedSpectrum[i] = smoothedSpectrum[i] + (target - smoothedSpectrum[i]) * attack;
        } else {
            smoothedSpectrum[i] = smoothedSpectrum[i] + (target - smoothedSpectrum[i]) * decay;
        }

        unsigned char val = static_cast<unsigned char>(smoothedSpectrum[i] * 255.0f);
        current_audio_data[1][0][i] = static_cast<char>(val);
        current_audio_data[1][1][i] = static_cast<char>(val);
    }

#else
    // ========== ORIGINAL WINAMP MODE ==========
    // 512-sample FFT → 256 bins → expand to 576 → log table compression

    // Temporary buffer for Winamp-style spectrum (before log compression)
    unsigned char spectrumRaw[576];
    int outIdx = 0;
    float lastValue = 0.0f;

    // Process 256 FFT bins, output 2 values each (512 total)
    for (int x = 0; x < 256 && outIdx < 512; x++) {
        // ofxFft normalizes amplitude by 2/windowSum (≈ 1/128 for 512-sample Hann)
        // A full-scale sinusoid gives amplitude ≈ 2.0 after normalization
        // Winamp's /16 divisor was empirically chosen for typical audio levels
        // Scale by 128 to map normalized amplitudes to 0-255 range
        float mag = amplitude[x] * 128.0f;

        // Clamp to 255
        if (mag > 255.0f) mag = 255.0f;

        // Output smoothed value (average with previous)
        unsigned char smoothed = static_cast<unsigned char>((mag + lastValue) / 2.0f);
        spectrumRaw[outIdx++] = smoothed;

        // Output raw value
        spectrumRaw[outIdx++] = static_cast<unsigned char>(mag);

        lastValue = mag;
    }

    // Fill remaining slots (576 - 512 = 64) with decaying values
    while (outIdx < 576) {
        lastValue /= 2.0f;
        spectrumRaw[outIdx++] = static_cast<unsigned char>(lastValue);
    }

    // Apply AVS log table compression
    for (int i = 0; i < 576; i++) {
        unsigned char compressed = logTable[spectrumRaw[i]];
        current_audio_data[1][0][i] = static_cast<char>(compressed);
        current_audio_data[1][1][i] = static_cast<char>(compressed);
    }

#endif
}

void ofxAVS::setAudioData(const avs::AudioData& data) {
    std::memcpy(current_audio_data, data, sizeof(avs::AudioData));
}

void ofxAVS::draw(int x, int y, int w, int h) {
    texture.draw(x, y, w, h);

    // Draw FPS below output
    ofSetColor(255);
    ofDrawBitmapString("FPS: " + ofToString(ofGetFrameRate(), 1), x, y + h + 15);
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

void ofxAVS::duplicateEffect(int index) {
    if (index >= 0 && index < static_cast<int>(effect_chain.size())) {
        // Get the source effect
        auto* source_effect = renderer->get_effect(static_cast<size_t>(index));
        if (!source_effect) return;

        // Create a new effect of the same type
        const std::string& effect_name = effect_chain[index].name;
        auto new_effect = avs::PluginManager::instance().create_effect(effect_name);
        if (!new_effect) return;

        // Copy parameters from source to new effect
        new_effect->parameters().copy_from(source_effect->parameters());

        // Insert after the current effect
        size_t insert_pos = static_cast<size_t>(index) + 1;
        renderer->insert_effect(insert_pos, std::move(new_effect));

        // Update effect chain metadata
        effect_chain.insert(effect_chain.begin() + insert_pos, effect_chain[index]);

        // Select the new duplicate
        selected_effect_index = static_cast<int>(insert_pos);
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
                if (ImGui::MenuItem("x2 (Duplicate)")) {
                    duplicateEffect(i);
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Move Up", nullptr, false, i > 0)) {
                    moveEffectUp(i);
                }
                if (ImGui::MenuItem("Move Down", nullptr, false, i < effect_chain.size() - 1)) {
                    moveEffectDown(i);
                }
                ImGui::Separator();
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
    ImGui::SetNextWindowSize(ImVec2(parameters_panel_width, parameters_panel_height));
    
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