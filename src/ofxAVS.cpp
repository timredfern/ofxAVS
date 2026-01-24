// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "ofxAVS.h"
#include "AVSui.h"
#include "core/plugin_manager.h"
#include "core/builtin_effects.h"
#include "core/preset.h"
#include "core/ui.h"  // For resource_path()
#include <cmath>
#include <map>

// Session file location (in app's data folder)
static std::string getSessionPath() {
    return ofToDataPath("session.json");
}

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
    // Clear renderer to ensure proper cleanup
    renderer.reset();

    // Clean up FFT
    if (fft) {
        delete fft;
        fft = nullptr;
    }
}

void ofxAVS::setup() {
    width = 600;
    height = 600;

    // Set resource path for FILE_DROPDOWN controls (Picture effect, etc.)
    // This looks for resources in bin/data/avs/
    avs::resource_path() = ofToDataPath("avs");

    // Initialize FFT
#ifdef AVS_ENHANCED_FFT
    fft = ofxFft::create(FFT_SIZE, OF_FFT_WINDOW_HAMMING);
#else
    // Original Winamp used Hann window
    fft = ofxFft::create(FFT_SIZE, OF_FFT_WINDOW_HANN);
#endif

    // Initialize beat detector
    beat_detector_ = std::make_unique<avs::BeatDetector>();

    // Initialize renderer
    renderer = std::make_unique<avs::DefaultRenderer>(width, height);

    // Initialize texture - use BGRA to match our uint32_t ARGB format
    pixels.allocate(width, height, OF_PIXELS_BGRA);
    texture.allocate(pixels);

    // Register built-in effects
    avs::register_builtin_effects();

    // Initialize available effects list
    initializeAvailableEffects();
}

bool ofxAVS::loadSession() {
    std::string sessionPath = getSessionPath();
    if (!ofFile::doesFileExist(sessionPath)) {
        return false;
    }
    if (renderer->root()->load_preset(sessionPath)) {
        ofLogNotice("ofxAVS") << "Loaded session from " << sessionPath;
        return true;
    }
    ofLogWarning("ofxAVS") << "Failed to load session: " << avs::Preset::last_error();
    return false;
}

bool ofxAVS::saveSession() {
    std::string sessionPath = getSessionPath();
    ofDirectory::createDirectory(ofToDataPath(""), false, true);
    if (renderer->root()->save_preset(sessionPath)) {
        ofLogNotice("ofxAVS") << "Saved session to " << sessionPath;
        return true;
    }
    ofLogWarning("ofxAVS") << "Failed to save session: " << avs::Preset::last_error();
    return false;
}

void ofxAVS::update() {
    // Process beat detection
    bool isBeat = beat_detector_->process(current_audio_data);

    // Render directly into pixels buffer (no intermediate copy)
    renderer->render(current_audio_data, isBeat,
                     reinterpret_cast<uint32_t*>(pixels.getData()));
    texture.loadData(pixels);
}

void ofxAVS::audioIn(ofSoundBuffer& buffer) {
    // Safety check - fft might not be initialized yet
    if (!fft || buffer.getNumFrames() == 0 || buffer.getNumChannels() == 0) {
        return;
    }

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
    drawChainPanelInternal();
    drawParametersPanel();
    avs_ui::renderExpressionHelpPopup();  // Render help popup if open
}

void ofxAVS::addEffect(const std::string& effectName, avs::EffectContainer* parent) {
    // Use root if no parent specified
    if (!parent) {
        parent = renderer->root();
    }

    // Create effect and add to container
    auto new_effect = avs::PluginManager::instance().create_effect(effectName);
    if (new_effect) {
        avs::EffectBase* effect_ptr = new_effect.get();
        parent->add_child(std::move(new_effect));

        // Select the newly added effect
        selected_ = effect_ptr;
    }
}

void ofxAVS::cleanupPointersToEffect(avs::EffectBase* effect) {
    // Clear selected_ if it points to this effect
    if (selected_ == effect) {
        selected_ = nullptr;
    }

    // Remove from collapsed_containers_ if it's a container
    if (auto* container = dynamic_cast<avs::EffectContainer*>(effect)) {
        collapsed_containers_.erase(container);

        // Recursively clean up children
        for (size_t i = 0; i < container->child_count(); i++) {
            cleanupPointersToEffect(container->get_child(i));
        }
    }
}

void ofxAVS::removeEffect(avs::EffectBase* effect) {
    if (!effect) return;

    // Clean up UI pointers before removal
    cleanupPointersToEffect(effect);

    // Use library method to remove (don't need return value here, keyboard handler handles selection)
    renderer->root()->remove_effect(effect);
}

void ofxAVS::duplicateEffect(avs::EffectBase* effect) {
    if (!effect) return;

    // Find parent container
    avs::EffectContainer* parent = findParentContainer(effect);
    if (!parent) return;

    int index = parent->find_child_index(effect);
    if (index < 0) return;

    // Get effect type name from plugin info
    const std::string& effect_name = effect->get_plugin_info().name;
    auto new_effect = avs::PluginManager::instance().create_effect(effect_name);
    if (!new_effect) return;

    // Copy parameters from source to new effect
    new_effect->parameters().copy_from(effect->parameters());

    // Insert after the current effect
    avs::EffectBase* new_effect_ptr = new_effect.get();
    parent->insert_child(static_cast<size_t>(index + 1), std::move(new_effect));

    // Select the new duplicate
    selected_ = new_effect_ptr;
}

void ofxAVS::moveEffectUp(avs::EffectBase* effect) {
    if (!effect) return;

    avs::EffectContainer* parent = findParentContainer(effect);
    if (!parent) return;

    parent->move_child_up(static_cast<size_t>(parent->find_child_index(effect)));
}

void ofxAVS::moveEffectDown(avs::EffectBase* effect) {
    if (!effect) return;

    avs::EffectContainer* parent = findParentContainer(effect);
    if (!parent) return;

    parent->move_child_down(static_cast<size_t>(parent->find_child_index(effect)));
}

bool ofxAVS::loadPreset(const std::string& path) {
    if (!renderer || !renderer->root()) {
        last_error_ = "Renderer not initialized";
        return false;
    }

    // Clear selection since effects will be replaced
    selected_ = nullptr;

    bool success = renderer->root()->load_preset(path);
    if (!success) {
        last_error_ = avs::Preset::last_error();
    } else {
        last_error_.clear();
        // Extract preset name from filename (without extension)
        current_preset_name_ = ofFilePath::getBaseName(path);
        ofLogNotice("ofxAVS") << "Loaded preset: " << path;
    }
    return success;
}

bool ofxAVS::savePreset(const std::string& path) {
    if (!renderer || !renderer->root()) {
        last_error_ = "Renderer not initialized";
        return false;
    }

    bool success = renderer->root()->save_preset(path);
    if (!success) {
        last_error_ = avs::Preset::last_error();
    } else {
        last_error_.clear();
        ofLogNotice("ofxAVS") << "Saved preset: " << path;
    }
    return success;
}

const std::string& ofxAVS::getLastError() const {
    return last_error_;
}

avs::EffectContainer* ofxAVS::findParentContainer(avs::EffectBase* effect, avs::EffectContainer* searchIn) {
    if (!effect) return nullptr;

    // Start search from root if not specified
    if (!searchIn) {
        searchIn = renderer->root();
    }

    // Check if effect is a direct child
    for (size_t i = 0; i < searchIn->child_count(); i++) {
        if (searchIn->get_child(i) == effect) {
            return searchIn;
        }

        // Check if child is a container and recurse
        auto* child_container = dynamic_cast<avs::EffectContainer*>(searchIn->get_child(i));
        if (child_container) {
            auto* found = findParentContainer(effect, child_container);
            if (found) return found;
        }
    }

    return nullptr;
}

void ofxAVS::initializeAvailableEffects() {
    available_effects.clear();
    
    // Get all registered effects from plugin manager
    auto& pm = avs::PluginManager::instance();
    auto effect_names = pm.available_effects();
    
    for (const auto& name : effect_names) {
        AvailableEffectInfo info;
        info.name = name;
        
        // Get effect info which contains description and category
        auto plugin_info = pm.get_effect_info(name);
        info.display_name = plugin_info.name.empty() ? name : plugin_info.name;
        info.category = plugin_info.category.empty() ? "Misc" : plugin_info.category;
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

void ofxAVS::drawChainPanelInternal() {
    ImGui::SetNextWindowPos(ImVec2(10, 10));
    ImGui::SetNextWindowSize(ImVec2(chain_panel_width, chain_panel_height));

    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));

    if (ImGui::Begin(current_preset_name_.c_str(), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        // Handle keyboard navigation
        handleEffectListKeyboard();

        // Draw Beat detector (always first, not expandable)
        bool beat_selected = (selected_ == beat_detector_.get());
        if (beat_selected) {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.4f, 0.4f, 0.6f, 1.0f));
        }

        // Show BPM info in the label if available
        std::string beat_label = "Beat detector";
        if (beat_detector_->getBpm() > 0) {
            beat_label += " (" + std::to_string(beat_detector_->getBpm()) + " BPM)";
        }

        if (ImGui::Selectable(beat_label.c_str(), beat_selected, ImGuiSelectableFlags_None, ImVec2(chain_panel_width - 30, 0))) {
            selected_ = beat_detector_.get();
        }
        if (beat_selected) {
            ImGui::PopStyleColor();
        }

        // Context menu for beat detector
        if (ImGui::BeginPopupContextItem("beat_context")) {
            if (ImGui::MenuItem("Params")) {
                if (on_open_params_callback_) {
                    on_open_params_callback_(beat_detector_.get());
                }
            }
            ImGui::EndPopup();
        }

        ImGui::Separator();

        // Draw the root container as "Effect chain"
        avs::EffectListRoot* root = renderer->root();
        if (root) {
            bool root_selected = (selected_ == root);
            bool is_expanded = collapsed_containers_.find(root) == collapsed_containers_.end();

            // Arrow button for expand/collapse
            if (ImGui::ArrowButton("##chain_arrow", is_expanded ? ImGuiDir_Down : ImGuiDir_Right)) {
                if (is_expanded) {
                    collapsed_containers_.insert(root);
                } else {
                    collapsed_containers_.erase(root);
                }
            }
            ImGui::SameLine();

            // Effect chain selectable
            if (root_selected) {
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.4f, 0.4f, 0.6f, 1.0f));
            }
            if (ImGui::Selectable("Effect chain", root_selected, ImGuiSelectableFlags_None, ImVec2(chain_panel_width - 50, 0))) {
                selected_ = root;
            }
            if (root_selected) {
                ImGui::PopStyleColor();
            }

            // Context menu for root
            if (ImGui::BeginPopupContextItem("chain_context")) {
                drawAddEffectMenu(root);
                ImGui::Separator();
                if (ImGui::MenuItem("Params")) {
                    if (on_open_params_callback_) {
                        on_open_params_callback_(root);
                    }
                }
                ImGui::EndPopup();
            }

            // Draw children if expanded
            if (is_expanded) {
                drawEffectTree(root, 1);
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(3);
}

// Drag-drop payload for effect reordering
struct EffectDragPayload {
    avs::EffectBase* effect;
    avs::EffectContainer* source_container;
    size_t source_index;
};

// Draw a drop zone line between effects
static void drawDropZone(avs::EffectContainer* container, size_t insert_index, float indent, float width) {
    std::string zone_id = "##dropzone_" + std::to_string(reinterpret_cast<uintptr_t>(container)) + "_" + std::to_string(insert_index);

    // Small invisible button as drop target
    ImGui::Indent(indent);
    ImGui::InvisibleButton(zone_id.c_str(), ImVec2(width, 4));

    bool is_drop_target = ImGui::BeginDragDropTarget();
    if (is_drop_target) {
        // Draw visible line when hovering
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(min.x, (min.y + max.y) * 0.5f),
            ImVec2(max.x, (min.y + max.y) * 0.5f),
            IM_COL32(100, 150, 255, 255), 2.0f);

        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EFFECT_DRAG")) {
            EffectDragPayload* data = (EffectDragPayload*)payload->Data;
            auto moved = data->source_container->take_child(data->source_index);
            if (moved) {
                // Adjust insert index if moving within same container and source was before target
                size_t adjusted_index = insert_index;
                if (data->source_container == container && data->source_index < insert_index) {
                    adjusted_index--;
                }
                container->insert_child(adjusted_index, std::move(moved));
            }
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::Unindent(indent);
}

// Helper to format effect timing for display
static std::string formatTiming(double us) {
    double ms = us / 1000.0;
    char buf[32];
    if (ms < 0.01) {
        snprintf(buf, sizeof(buf), "(<0.01ms)");
    } else if (ms < 1.0) {
        snprintf(buf, sizeof(buf), "(%.2fms)", ms);
    } else if (ms < 10.0) {
        snprintf(buf, sizeof(buf), "(%.1fms)", ms);
    } else {
        snprintf(buf, sizeof(buf), "(%.0fms)", ms);
    }
    return buf;
}

void ofxAVS::drawEffectTree(avs::EffectContainer* container, int depth) {
    float indent = depth * 20.0f;
    float drop_width = chain_panel_width - indent - 30;

    // Drop zone before first effect
    drawDropZone(container, 0, indent, drop_width);

    for (size_t i = 0; i < container->child_count(); i++) {
        avs::EffectBase* effect = container->get_child(i);
        if (!effect) continue;

        ImGui::Indent(indent);

        // Check if this is a container
        auto* child_container = dynamic_cast<avs::EffectContainer*>(effect);
        bool is_selected = (selected_ == effect);

        // Generate unique ID
        std::string id = "##effect_" + std::to_string(reinterpret_cast<uintptr_t>(effect));

        if (child_container) {
            // Container: draw arrow + name
            bool is_expanded = collapsed_containers_.find(child_container) == collapsed_containers_.end();

            if (ImGui::ArrowButton(("##arrow" + id).c_str(), is_expanded ? ImGuiDir_Down : ImGuiDir_Right)) {
                if (is_expanded) {
                    collapsed_containers_.insert(child_container);
                } else {
                    collapsed_containers_.erase(child_container);
                }
            }
            ImGui::SameLine();

            if (is_selected) {
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.4f, 0.4f, 0.6f, 1.0f));
            }
            std::string label = effect->get_plugin_info().name + id;
            float selectable_width = show_profiling_ ? chain_panel_width - indent - 120 : chain_panel_width - indent - 50;
            if (ImGui::Selectable(label.c_str(), is_selected, ImGuiSelectableFlags_None, ImVec2(selectable_width, 0))) {
                selected_ = effect;
            }
            if (is_selected) {
                ImGui::PopStyleColor();
            }

            // Drag source
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                EffectDragPayload payload = {effect, container, i};
                ImGui::SetDragDropPayload("EFFECT_DRAG", &payload, sizeof(payload));
                ImGui::Text("%s", effect->get_plugin_info().name.c_str());
                ImGui::EndDragDropSource();
            }

            // Context menu
            if (ImGui::BeginPopupContextItem()) {
                drawEffectContextMenu(effect);
                ImGui::EndPopup();
            }

            // Show timing after selectable (so it doesn't affect context menu)
            if (show_profiling_) {
                ImGui::SameLine();
                ImGui::TextDisabled("%s", formatTiming(effect->get_last_render_time_us()).c_str());
            }

            // Recurse if expanded
            if (is_expanded) {
                ImGui::Unindent(indent);
                drawEffectTree(child_container, depth + 1);
                ImGui::Indent(indent);
            }
        } else {
            // Regular effect: just name
            if (is_selected) {
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.4f, 0.4f, 0.6f, 1.0f));
            }
            std::string label = effect->get_plugin_info().name + id;
            float selectable_width = show_profiling_ ? chain_panel_width - indent - 100 : 0;
            if (ImGui::Selectable(label.c_str(), is_selected, ImGuiSelectableFlags_None, ImVec2(selectable_width, 0))) {
                selected_ = effect;
            }
            if (is_selected) {
                ImGui::PopStyleColor();
            }

            // Drag source
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                EffectDragPayload payload = {effect, container, i};
                ImGui::SetDragDropPayload("EFFECT_DRAG", &payload, sizeof(payload));
                ImGui::Text("%s", effect->get_plugin_info().name.c_str());
                ImGui::EndDragDropSource();
            }

            // Context menu
            if (ImGui::BeginPopupContextItem()) {
                drawEffectContextMenu(effect);
                ImGui::EndPopup();
            }

            // Show timing after selectable (so it doesn't affect context menu)
            if (show_profiling_) {
                ImGui::SameLine();
                ImGui::TextDisabled("%s", formatTiming(effect->get_last_render_time_us()).c_str());
            }
        }

        ImGui::Unindent(indent);

        // Drop zone after this effect
        drawDropZone(container, i + 1, indent, drop_width);
    }
}

void ofxAVS::drawAddEffectMenu(avs::EffectContainer* targetContainer) {
    if (ImGui::BeginMenu("Add Effect")) {
        // Group effects by category
        std::map<std::string, std::vector<const AvailableEffectInfo*>> categories;

        for (const auto& effect : available_effects) {
            categories[effect.category].push_back(&effect);
        }

        // Draw categorized menu
        for (const auto& [category, effects] : categories) {
            if (ImGui::BeginMenu(category.c_str())) {
                for (const auto* effect : effects) {
                    if (ImGui::MenuItem(effect->display_name.c_str())) {
                        addEffect(effect->name, targetContainer);
                    }
                    if (ImGui::IsItemHovered() && !effect->description.empty()) {
                        ImGui::SetTooltip("%s", effect->description.c_str());
                    }
                }
                ImGui::EndMenu();
            }
        }
        ImGui::EndMenu();
    }
}

void ofxAVS::drawAddEffectMenuInsertAfter(avs::EffectContainer* container, int afterIndex) {
    if (ImGui::BeginMenu("Add Effect")) {
        // Group effects by category
        std::map<std::string, std::vector<const AvailableEffectInfo*>> categories;

        for (const auto& effect : available_effects) {
            categories[effect.category].push_back(&effect);
        }

        // Draw categorized menu
        for (const auto& [category, effects] : categories) {
            if (ImGui::BeginMenu(category.c_str())) {
                for (const auto* effect : effects) {
                    if (ImGui::MenuItem(effect->display_name.c_str())) {
                        insertEffect(effect->name, container, static_cast<size_t>(afterIndex + 1));
                    }
                    if (ImGui::IsItemHovered() && !effect->description.empty()) {
                        ImGui::SetTooltip("%s", effect->description.c_str());
                    }
                }
                ImGui::EndMenu();
            }
        }
        ImGui::EndMenu();
    }
}

void ofxAVS::insertEffect(const std::string& effectName, avs::EffectContainer* container, size_t index) {
    auto new_effect = avs::PluginManager::instance().create_effect(effectName);
    if (new_effect) {
        avs::EffectBase* effect_ptr = new_effect.get();
        container->insert_child(index, std::move(new_effect));
        selected_ = effect_ptr;
    }
}

void ofxAVS::drawEffectContextMenu(avs::EffectBase* effect) {
    avs::EffectContainer* parent = findParentContainer(effect);
    int index = parent ? parent->find_child_index(effect) : -1;

    // Check if this effect is a container
    auto* as_container = dynamic_cast<avs::EffectContainer*>(effect);

    // Add Effect submenu
    if (as_container) {
        // For containers: add inside the container
        drawAddEffectMenu(as_container);
    } else if (parent) {
        // For regular effects: insert after this effect
        drawAddEffectMenuInsertAfter(parent, index);
    }
    ImGui::Separator();

    if (ImGui::MenuItem("x2 (Duplicate)")) {
        duplicateEffect(effect);
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Move Up", nullptr, false, index > 0)) {
        moveEffectUp(effect);
    }
    if (ImGui::MenuItem("Move Down", nullptr, false, parent && index < static_cast<int>(parent->child_count()) - 1)) {
        moveEffectDown(effect);
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Params")) {
        if (on_open_params_callback_) {
            on_open_params_callback_(effect);
        }
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Remove")) {
        removeEffect(effect);
    }
}

void ofxAVS::drawParametersPanel() {
    int panel_x = chain_panel_width + 20;
    ImGui::SetNextWindowPos(ImVec2(panel_x, 10));
    ImGui::SetNextWindowSize(ImVec2(parameters_panel_width, parameters_panel_height));

    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));

    std::string title = selected_ ? selected_->get_display_name() : "Parameters";
    if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
        if (selected_) {
            // Get UI layout from the Configurable interface
            const avs::EffectUILayout& layout = selected_->get_ui_layout();

            if (!layout.getControls().empty()) {
                // Render UI using the Configurable's parameters
                avs_ui::renderImGui(layout, selected_);
            } else {
                ImGui::Text("No parameters available");
            }
        } else {
            ImGui::Text("Select an item to see its parameters");
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(3);
}

void ofxAVS::buildVisibleEffectList(avs::EffectContainer* container, std::vector<avs::EffectBase*>& list) {
    // Add the container itself (except root which is handled separately)
    if (container != renderer->root()) {
        list.push_back(container);
    }

    // If collapsed, don't add children
    if (collapsed_containers_.find(container) != collapsed_containers_.end()) {
        return;
    }

    // Add visible children
    for (size_t i = 0; i < container->child_count(); i++) {
        avs::EffectBase* child = container->get_child(i);
        if (!child) continue;

        auto* child_container = dynamic_cast<avs::EffectContainer*>(child);
        if (child_container) {
            buildVisibleEffectList(child_container, list);
        } else {
            list.push_back(child);
        }
    }
}

avs::EffectBase* ofxAVS::getNextVisibleEffect(avs::EffectBase* current) {
    std::vector<avs::EffectBase*> visible;
    visible.push_back(renderer->root());  // Root is always first
    buildVisibleEffectList(renderer->root(), visible);

    for (size_t i = 0; i < visible.size(); i++) {
        if (visible[i] == current && i + 1 < visible.size()) {
            return visible[i + 1];
        }
    }
    return current;  // No next, stay on current
}

avs::EffectBase* ofxAVS::getPrevVisibleEffect(avs::EffectBase* current) {
    std::vector<avs::EffectBase*> visible;
    visible.push_back(renderer->root());  // Root is always first
    buildVisibleEffectList(renderer->root(), visible);

    for (size_t i = 1; i < visible.size(); i++) {
        if (visible[i] == current) {
            return visible[i - 1];
        }
    }
    return current;  // No prev, stay on current
}

void ofxAVS::handleEffectListKeyboard() {
    // Only handle if AVS window is focused
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
        return;
    }

    bool shift = ImGui::GetIO().KeyShift;

    // Cast selected to effect if it is one (for navigation purposes)
    auto* selected_effect = dynamic_cast<avs::EffectBase*>(selected_);

    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        if (shift) {
            // Shift+Up: collapse current container
            auto* container = dynamic_cast<avs::EffectContainer*>(selected_);
            if (container) {
                collapsed_containers_.insert(container);
            }
        } else {
            // Up: move to previous item
            if (selected_ == beat_detector_.get()) {
                // Already at top, do nothing
            } else if (selected_ == renderer->root()) {
                // Move from root to beat detector
                selected_ = beat_detector_.get();
            } else if (selected_effect) {
                auto* prev = getPrevVisibleEffect(selected_effect);
                if (prev == renderer->root() && selected_effect == renderer->root()->get_child(0)) {
                    // First effect, could go to root or beat detector
                    selected_ = renderer->root();
                } else {
                    selected_ = prev;
                }
            } else {
                selected_ = beat_detector_.get();
            }
        }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        if (shift) {
            // Shift+Down: expand current container
            auto* container = dynamic_cast<avs::EffectContainer*>(selected_);
            if (container) {
                collapsed_containers_.erase(container);
            }
        } else {
            // Down: move to next item
            if (selected_ == beat_detector_.get()) {
                // Move from beat detector to root
                selected_ = renderer->root();
            } else if (selected_effect) {
                selected_ = getNextVisibleEffect(selected_effect);
            } else {
                selected_ = beat_detector_.get();
            }
        }
    }

    // Delete key: remove selected effect
    if (ImGui::IsKeyPressed(ImGuiKey_Delete) || ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
        if (selected_effect && selected_effect != renderer->root()) {
            // Get siblings before removal (effect will be destroyed)
            auto* next = renderer->root()->get_sibling_after(selected_effect);
            auto* prev = renderer->root()->get_sibling_before(selected_effect);
            auto* parent = renderer->root()->find_parent_of(selected_effect);

            // Clean up UI pointers and remove
            cleanupPointersToEffect(selected_effect);
            renderer->root()->remove_effect(selected_effect);

            // Select next sibling, prev sibling, or parent
            selected_ = next ? next : (prev ? prev : (parent ? parent : renderer->root()));
        }
    }
}