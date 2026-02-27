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
#include <iomanip>

// Session file location (in app's data folder)
static std::string getSessionPath() {
    return ofToDataPath("session.json");
}

ofxAVS::ofxAVS() {
    // Initialize AVS log table for classic mode (base ~60 compression)
    // Formula: log(x * 60/255 + 1) / log(60) * 255
    for (int x = 0; x < 256; x++) {
        double a = log(x * 60.0 / 255.0 + 1.0) / log(60.0);
        int t = static_cast<int>(a * 255.0);
        if (t < 0) t = 0;
        if (t > 255) t = 255;
        logTable[x] = static_cast<unsigned char>(t);
    }
}

ofxAVS::~ofxAVS() {
    // Stop audio stream
    if (audio_initialized_) {
        sound_stream_.stop();
        sound_stream_.close();
    }

    // Clear renderer to ensure proper cleanup
    renderer.reset();

    // Clean up FFT
    if (fft_left_) {
        delete fft_left_;
        fft_left_ = nullptr;
    }
    if (fft_right_) {
        delete fft_right_;
        fft_right_ = nullptr;
    }
}

void ofxAVS::setup() {
    width = 600;
    height = 600;

    // Set resource path for FILE_DROPDOWN controls (Picture effect, etc.)
    // This looks for resources in bin/data/avs/
    avs::resource_path() = ofToDataPath("avs");

    // Initialize FFT based on mode (classic uses Hann like original Winamp)
    createFft();

    // Initialize beat detector
    beat_detector_ = std::make_unique<BeatDetector>();

    // Initialize renderer
    renderer = std::make_unique<avs::DefaultRenderer>(width, height);

    // Initialize pixels buffer (CPU) - texture allocated on first draw
    pixels.allocate(width, height, OF_PIXELS_BGRA);

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
    // Update MIDI file playback (push events to EventBus based on audio position)
    updateMidiPlayback();

    // Update live MIDI input (push events to EventBus from hardware)
    midi_input_.update();

    // Process beat detection
    // Classic mode: use AudioData (energy-based, processed here)
    // Modern mode: use raw FFT (spectral flux, processed in audioIn)
    bool isBeat;
    if (audio_classic_mode_) {
        isBeat = beat_detector_->process(current_audio_data);
    } else {
        isBeat = is_beat_;
    }

    // Render directly into pixels buffer (CPU - context independent)
    // Note: Renderer::render() calls EventBus::process_frame() internally
    renderer->render(current_audio_data, isBeat,
                     reinterpret_cast<uint32_t*>(pixels.getData()));
}

void ofxAVS::audioIn(ofSoundBuffer& buffer) {
    // Safety check - fft might not be initialized yet
    if (!fft_left_ || !fft_right_ || buffer.getNumFrames() == 0 || buffer.getNumChannels() == 0) {
        return;
    }

    // When using mic input (not file playback), apply mic gain
    // audioOut() calls this for file playback, so only apply gain for actual mic input
    bool applyMicGain = (!audio_use_file_ || !audio_is_playing_) && audio_mic_gain_ != 1.0f;

    memset(current_audio_data, 0, sizeof(avs::AudioData));

    int numChannels = buffer.getNumChannels();
    int numFrames = buffer.getNumFrames();
    int fftSize = audio_classic_mode_ ? FFT_SIZE_CLASSIC : FFT_SIZE_MODERN;

    static vector<float> fftSamplesLeft(FFT_SIZE_MODERN);
    static vector<float> fftSamplesRight(FFT_SIZE_MODERN);

    // Store raw audio in circular buffer (for modern mode frame-accurate resampling)
    for (int i = 0; i < numFrames; i++) {
        float left = buffer[i * numChannels];
        float right = (numChannels >= 2) ? buffer[i * numChannels + 1] : left;

        // Apply mic gain if needed
        if (applyMicGain) {
            left = ofClamp(left * audio_mic_gain_, -1.0f, 1.0f);
            right = ofClamp(right * audio_mic_gain_, -1.0f, 1.0f);
        }

        // Store in circular buffer
        raw_audio_left_[raw_audio_write_pos_] = left;
        raw_audio_right_[raw_audio_write_pos_] = right;
        raw_audio_write_pos_ = (raw_audio_write_pos_ + 1) % RAW_AUDIO_BUFFER_SIZE;

        // Prepare stereo FFT input
        if (i < fftSize) {
            fftSamplesLeft[i] = left;
            fftSamplesRight[i] = right;
        }
    }

    // Track how many samples we have available
    raw_audio_samples_available_ = std::min(raw_audio_samples_available_ + numFrames, RAW_AUDIO_BUFFER_SIZE);

    // Zero-pad FFT input if needed
    for (int i = numFrames; i < fftSize; i++) {
        fftSamplesLeft[i] = 0;
        fftSamplesRight[i] = 0;
    }

    // Compute stereo FFT spectrum
    fft_left_->setSignal(fftSamplesLeft);
    fft_right_->setSignal(fftSamplesRight);
    float* amplitudeLeft = fft_left_->getAmplitude();
    float* amplitudeRight = fft_right_->getAmplitude();
    int binSize = fft_left_->getBinSize();

    if (audio_classic_mode_) {
        // ========== CLASSIC MODE (Original Winamp) ==========
        // Fixed MIN_AUDIO_SAMPLES, arbitrary slice of audio

        int numSamples = std::min(numFrames, avs::MIN_AUDIO_SAMPLES);

        // Fill waveform directly from buffer (classic behavior)
        for (int i = 0; i < numSamples; i++) {
            float left = buffer[i * numChannels];
            float right = (numChannels >= 2) ? buffer[i * numChannels + 1] : left;
            if (applyMicGain) {
                left = ofClamp(left * audio_mic_gain_, -1.0f, 1.0f);
                right = ofClamp(right * audio_mic_gain_, -1.0f, 1.0f);
            }
            current_audio_data[avs::AUDIO_WAVEFORM][avs::AUDIO_LEFT][i] = static_cast<char>(left * 127.0f);
            current_audio_data[avs::AUDIO_WAVEFORM][avs::AUDIO_RIGHT][i] = static_cast<char>(right * 127.0f);
        }

        // 512-sample FFT → 256 bins → expand to MIN_AUDIO_SAMPLES → log table compression
        // Process left and right channels separately
        unsigned char spectrumRawLeft[avs::MIN_AUDIO_SAMPLES];
        unsigned char spectrumRawRight[avs::MIN_AUDIO_SAMPLES];

        // Process left channel
        int outIdx = 0;
        float lastValue = 0.0f;
        for (int x = 0; x < 256 && outIdx < 512; x++) {
            float mag = amplitudeLeft[x] * 128.0f;
            if (mag > 255.0f) mag = 255.0f;
            unsigned char smoothed = static_cast<unsigned char>((mag + lastValue) / 2.0f);
            spectrumRawLeft[outIdx++] = smoothed;
            spectrumRawLeft[outIdx++] = static_cast<unsigned char>(mag);
            lastValue = mag;
        }
        while (outIdx < avs::MIN_AUDIO_SAMPLES) {
            lastValue /= 2.0f;
            spectrumRawLeft[outIdx++] = static_cast<unsigned char>(lastValue);
        }

        // Process right channel
        outIdx = 0;
        lastValue = 0.0f;
        for (int x = 0; x < 256 && outIdx < 512; x++) {
            float mag = amplitudeRight[x] * 128.0f;
            if (mag > 255.0f) mag = 255.0f;
            unsigned char smoothed = static_cast<unsigned char>((mag + lastValue) / 2.0f);
            spectrumRawRight[outIdx++] = smoothed;
            spectrumRawRight[outIdx++] = static_cast<unsigned char>(mag);
            lastValue = mag;
        }
        while (outIdx < avs::MIN_AUDIO_SAMPLES) {
            lastValue /= 2.0f;
            spectrumRawRight[outIdx++] = static_cast<unsigned char>(lastValue);
        }

        // Apply log compression to both channels
        for (int i = 0; i < avs::MIN_AUDIO_SAMPLES; i++) {
            current_audio_data[avs::AUDIO_SPECTRUM][avs::AUDIO_LEFT][i] = static_cast<char>(logTable[spectrumRawLeft[i]]);
            current_audio_data[avs::AUDIO_SPECTRUM][avs::AUDIO_RIGHT][i] = static_cast<char>(logTable[spectrumRawRight[i]]);
        }
    } else {
        // ========== MODERN MODE ==========
        // Frame-accurate resampling: one frame's audio → screen width samples

        // Calculate how many raw samples represent one frame
        float fps = ofGetFrameRate();
        if (fps < 1.0f) fps = 60.0f;  // Fallback
        int samplesPerFrame = static_cast<int>(audio_sample_rate_ / fps);
        samplesPerFrame = std::min(samplesPerFrame, raw_audio_samples_available_);
        samplesPerFrame = std::min(samplesPerFrame, RAW_AUDIO_BUFFER_SIZE);

        // Target sample count: max(render_width, MIN_AUDIO_SAMPLES)
        int targetSamples = std::max(render_width_, avs::MIN_AUDIO_SAMPLES);
        targetSamples = std::min(targetSamples, avs::MAX_AUDIO_SAMPLES);

        // Resample last frame's audio to target sample count
        if (samplesPerFrame > 0) {
            // Calculate read position (start of last frame's audio)
            int readStart = (raw_audio_write_pos_ - samplesPerFrame + RAW_AUDIO_BUFFER_SIZE) % RAW_AUDIO_BUFFER_SIZE;

            for (int i = 0; i < targetSamples; i++) {
                // Map output sample to input sample with linear interpolation
                float srcPos = (float)i * (samplesPerFrame - 1) / (targetSamples - 1);
                int srcIdx = static_cast<int>(srcPos);
                float frac = srcPos - srcIdx;

                int idx0 = (readStart + srcIdx) % RAW_AUDIO_BUFFER_SIZE;
                int idx1 = (readStart + srcIdx + 1) % RAW_AUDIO_BUFFER_SIZE;

                // Linear interpolation
                float left = raw_audio_left_[idx0] * (1.0f - frac) + raw_audio_left_[idx1] * frac;
                float right = raw_audio_right_[idx0] * (1.0f - frac) + raw_audio_right_[idx1] * frac;

                // Convert to signed char
                current_audio_data[avs::AUDIO_WAVEFORM][avs::AUDIO_LEFT][i] = static_cast<char>(left * 127.0f);
                current_audio_data[avs::AUDIO_WAVEFORM][avs::AUDIO_RIGHT][i] = static_cast<char>(right * 127.0f);
            }
        }

        // Spectrum: resample to target sample count with dB scale and temporal smoothing
        // Process stereo - separate smoothing for left and right channels
        const float attack = 0.8f;
        const float decay = 0.4f;

        for (int i = 0; i < targetSamples; i++) {
            float srcPos = (float)i * (binSize - 1) / (targetSamples - 1);
            int srcIdx = static_cast<int>(srcPos);
            float frac = srcPos - srcIdx;
            if (srcIdx >= binSize - 1) {
                srcIdx = binSize - 2;
                frac = 1.0f;
            }

            // Process left channel
            float magLeft = amplitudeLeft[srcIdx] * (1.0f - frac) + amplitudeLeft[srcIdx + 1] * frac;
            float dbLeft = 20.0f * log10f(magLeft + 0.00001f);
            float normalizedLeft = (dbLeft + 80.0f) / 80.0f;
            if (normalizedLeft < 0) normalizedLeft = 0;
            if (normalizedLeft > 1) normalizedLeft = 1;

            if (normalizedLeft > smoothedSpectrumLeft_[i]) {
                smoothedSpectrumLeft_[i] += (normalizedLeft - smoothedSpectrumLeft_[i]) * attack;
            } else {
                smoothedSpectrumLeft_[i] += (normalizedLeft - smoothedSpectrumLeft_[i]) * decay;
            }

            // Process right channel
            float magRight = amplitudeRight[srcIdx] * (1.0f - frac) + amplitudeRight[srcIdx + 1] * frac;
            float dbRight = 20.0f * log10f(magRight + 0.00001f);
            float normalizedRight = (dbRight + 80.0f) / 80.0f;
            if (normalizedRight < 0) normalizedRight = 0;
            if (normalizedRight > 1) normalizedRight = 1;

            if (normalizedRight > smoothedSpectrumRight_[i]) {
                smoothedSpectrumRight_[i] += (normalizedRight - smoothedSpectrumRight_[i]) * attack;
            } else {
                smoothedSpectrumRight_[i] += (normalizedRight - smoothedSpectrumRight_[i]) * decay;
            }

            current_audio_data[avs::AUDIO_SPECTRUM][avs::AUDIO_LEFT][i] = static_cast<char>(smoothedSpectrumLeft_[i] * 255.0f);
            current_audio_data[avs::AUDIO_SPECTRUM][avs::AUDIO_RIGHT][i] = static_cast<char>(smoothedSpectrumRight_[i] * 255.0f);
        }

        // Modern beat detection: use raw FFT magnitudes (spectral flux)
        is_beat_ = beat_detector_->processModern(amplitudeLeft, amplitudeRight, binSize);
    }
}

void ofxAVS::setAudioData(const avs::AudioData& data) {
    std::memcpy(current_audio_data, data, sizeof(avs::AudioData));
}

void ofxAVS::draw(int x, int y, int w, int h) {
    // Track render width for audio resampling
    render_width_ = w;

    // Auto-resize if draw dimensions changed
    if (w != width || h != height) {
        resize(w, h);
    }

    // Allocate texture in current GL context if needed
    if (!texture.isAllocated()) {
        texture.allocate(pixels);
    }

    // Upload pixels to texture (must be in same context as draw)
    texture.loadData(pixels);
    texture.draw(x, y, w, h);

    // Draw FPS below output
    ofSetColor(255);
    ofDrawBitmapString("FPS: " + ofToString(ofGetFrameRate(), 1), x, y + h + 15);
}

void ofxAVS::resize(int w, int h) {
    if (w <= 0 || h <= 0) return;

    width = w;
    height = h;
    renderer->resize(w, h);
    pixels.allocate(w, h, OF_PIXELS_BGRA);
    texture.allocate(pixels);
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

void ofxAVS::saveEffectDialog(avs::EffectBase* effect) {
    if (!effect) return;

    std::string defaultName = effect->get_plugin_info().name + ".avsp";
    ofFileDialogResult result = ofSystemSaveDialog(defaultName, "Save Effect");
    if (result.bSuccess) {
        std::string path = result.getPath();
        // Ensure .avsp extension
        if (path.size() < 5 || path.substr(path.size() - 5) != ".avsp") {
            path += ".avsp";
        }
        if (avs::Preset::save_effect(path, effect)) {
            ofLogNotice("ofxAVS") << "Saved effect: " << path;
        } else {
            ofLogError("ofxAVS") << "Failed to save effect: " << avs::Preset::last_error();
        }
    }
}

void ofxAVS::loadEffectDialog(avs::EffectContainer* target, int insertIndex) {
    if (!target) {
        target = renderer ? renderer->root() : nullptr;
    }
    if (!target) return;

    ofFileDialogResult result = ofSystemLoadDialog("Load Effect", false, "");
    if (result.bSuccess) {
        std::string path = result.getPath();
        auto effect = avs::Preset::load_effect(path);
        if (effect) {
            if (insertIndex >= 0 && insertIndex <= static_cast<int>(target->child_count())) {
                target->insert_child(insertIndex, std::move(effect));
            } else {
                target->add_child(std::move(effect));
            }
            ofLogNotice("ofxAVS") << "Loaded effect: " << path;
        } else {
            ofLogError("ofxAVS") << "Failed to load effect: " << avs::Preset::last_error();
        }
    }
}

void ofxAVS::savePresetDialog() {
    ofFileDialogResult result = ofSystemSaveDialog("preset.avsp", "Save Preset");
    if (result.bSuccess) {
        std::string path = result.getPath();
        // Ensure .avsp extension
        if (path.size() < 5 || path.substr(path.size() - 5) != ".avsp") {
            path += ".avsp";
        }
        savePreset(path);
    }
}

void ofxAVS::loadPresetDialog() {
    ofFileDialogResult result = ofSystemLoadDialog("Load Preset", false, "");
    if (result.bSuccess) {
        loadPreset(result.getPath());
    }
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
        drawChainPanelContent();
    }
    ImGui::End();
    ImGui::PopStyleColor(3);
}

void ofxAVS::drawChainPanelContent() {
    // Style context menus with slightly lighter background for contrast
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.25f, 0.25f, 0.28f, 1.0f));

    // Get available width dynamically
    float availWidth = ImGui::GetContentRegionAvail().x;

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

    if (ImGui::Selectable(beat_label.c_str(), beat_selected, ImGuiSelectableFlags_None, ImVec2(availWidth - 15, 0))) {
        selected_ = beat_detector_.get();
    }
    if (beat_selected) {
        ImGui::PopStyleColor();
    }

    // Context menu for beat detector (only in multiwindow mode)
    if (on_open_params_callback_ && ImGui::BeginPopupContextItem("beat_context")) {
        if (ImGui::MenuItem("Params")) {
            on_open_params_callback_(beat_detector_.get());
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
        if (ImGui::Selectable("Effect chain", root_selected, ImGuiSelectableFlags_None, ImVec2(availWidth - 35, 0))) {
            selected_ = root;
        }
        if (root_selected) {
            ImGui::PopStyleColor();
        }

        // Context menu for root
        if (ImGui::BeginPopupContextItem("chain_context")) {
            drawAddEffectMenu(root);
            if (on_open_params_callback_) {
                ImGui::Separator();
                if (ImGui::MenuItem("Params")) {
                    on_open_params_callback_(root);
                }
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Save Preset...")) {
                savePresetDialog();
            }
            if (ImGui::MenuItem("Load Preset...")) {
                loadPresetDialog();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Load Effect...")) {
                loadEffectDialog(root, -1);
            }
            ImGui::EndPopup();
        }

        // Draw children if expanded
        if (is_expanded) {
            drawEffectTree(root, 1);
        }
    }

    ImGui::PopStyleColor();  // PopupBg
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
    float availWidth = ImGui::GetContentRegionAvail().x;
    float drop_width = availWidth - indent - 15;

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
            float selectable_width = show_profiling_ ? availWidth - indent - 105 : availWidth - indent - 35;
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
            float selectable_width = show_profiling_ ? availWidth - indent - 85 : 0;
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

    // Save/Load Effect (next to Add Effect)
    if (ImGui::MenuItem("Save Effect...")) {
        saveEffectDialog(effect);
    }
    if (ImGui::MenuItem("Load Effect...")) {
        // Load and insert after this effect
        if (parent) {
            loadEffectDialog(parent, index + 1);
        } else if (as_container) {
            loadEffectDialog(as_container, -1);
        }
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
    if (on_open_params_callback_) {
        ImGui::Separator();
        if (ImGui::MenuItem("Params")) {
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

    std::string title = (selected_ ? selected_->get_display_name() : "Parameters") + "###params_panel";
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

// ============================================================================
// Audio Management
// ============================================================================

void ofxAVS::setupAudio() {
    // Get device list
    audio_devices_ = sound_stream_.getDeviceList();
    audio_input_indices_.clear();
    audio_output_indices_.clear();

    ofLogNotice("ofxAVS") << "Available audio devices:";
    for (size_t i = 0; i < audio_devices_.size(); i++) {
        auto& device = audio_devices_[i];
        std::string rates;
        for (auto r : device.sampleRates) rates += std::to_string(r) + " ";
        ofLogNotice("ofxAVS") << "  " << device.deviceID << ": " << device.name
                              << " (in:" << device.inputChannels << " out:" << device.outputChannels << ")"
                              << (rates.empty() ? "" : " rates: " + rates);

        if (device.inputChannels > 0) {
            audio_input_indices_.push_back(i);
        }
        if (device.outputChannels > 0) {
            audio_output_indices_.push_back(i);
        }
    }

    // Helper to check if device supports standard sample rates
    auto supportsStandardRates = [](const ofSoundDevice& device) {
        if (device.sampleRates.empty()) return true;
        for (unsigned int rate : {44100u, 48000u, 96000u}) {
            if (std::find(device.sampleRates.begin(), device.sampleRates.end(), rate)
                != device.sampleRates.end()) {
                return true;
            }
        }
        return false;
    };

    // Select devices based on saved names or auto-detect
    audio_selected_input_ = -1;
    audio_selected_output_ = -1;

    // Try to find saved input device
    if (!audio_input_device_name_.empty()) {
        for (size_t i = 0; i < audio_input_indices_.size(); i++) {
            if (audio_devices_[audio_input_indices_[i]].name == audio_input_device_name_) {
                audio_selected_input_ = i;
                break;
            }
        }
    }

    // Try to find saved output device
    if (!audio_output_device_name_.empty()) {
        for (size_t i = 0; i < audio_output_indices_.size(); i++) {
            if (audio_devices_[audio_output_indices_[i]].name == audio_output_device_name_) {
                audio_selected_output_ = i;
                break;
            }
        }
    }

    // Auto-select output if not found
    if (audio_selected_output_ < 0 && !audio_output_indices_.empty()) {
        audio_selected_output_ = 0;
    }

    // Auto-select input if not found (prefer one with standard sample rates)
    if (audio_selected_input_ < 0 && !audio_input_indices_.empty()) {
        for (size_t i = 0; i < audio_input_indices_.size(); i++) {
            auto& device = audio_devices_[audio_input_indices_[i]];
            if (supportsStandardRates(device)) {
                audio_selected_input_ = i;
                break;
            }
        }
    }

    restartAudio();
}

void ofxAVS::restartAudio() {
    // Stop existing stream
    if (audio_initialized_) {
        sound_stream_.stop();
        sound_stream_.close();
        audio_initialized_ = false;
        audio_has_input_ = false;
    }

    if (audio_selected_output_ < 0 || audio_output_indices_.empty()) {
        ofLogWarning("ofxAVS") << "No output device selected";
        return;
    }

    // Helper to find a common sample rate between two devices
    auto findCommonRate = [](const ofSoundDevice* dev1, const ofSoundDevice* dev2) -> unsigned int {
        std::vector<unsigned int> commonRates = {44100, 48000, 96000, 22050, 16000, 8000};
        for (unsigned int rate : commonRates) {
            bool dev1Ok = !dev1 || dev1->sampleRates.empty() ||
                std::find(dev1->sampleRates.begin(), dev1->sampleRates.end(), rate) != dev1->sampleRates.end();
            bool dev2Ok = !dev2 || dev2->sampleRates.empty() ||
                std::find(dev2->sampleRates.begin(), dev2->sampleRates.end(), rate) != dev2->sampleRates.end();
            if (dev1Ok && dev2Ok) return rate;
        }
        return 44100;
    };

    ofSoundDevice* outputDevice = &audio_devices_[audio_output_indices_[audio_selected_output_]];
    ofSoundDevice* inputDevice = (audio_selected_input_ >= 0 && audio_selected_input_ < (int)audio_input_indices_.size())
        ? &audio_devices_[audio_input_indices_[audio_selected_input_]]
        : nullptr;

    ofSoundStreamSettings settings;
    settings.bufferSize = avs::MIN_AUDIO_SAMPLES;
    settings.setOutListener(this);
    settings.sampleRate = findCommonRate(outputDevice, inputDevice);
    settings.numOutputChannels = std::min(2u, outputDevice->outputChannels);
    settings.setOutDevice(*outputDevice);

    if (inputDevice) {
        settings.numInputChannels = 1;
        settings.setInDevice(*inputDevice);
        settings.setInListener(this);
    } else {
        settings.numInputChannels = 0;
    }

    try {
        sound_stream_.setup(settings);
        audio_initialized_ = true;
        audio_has_input_ = (inputDevice != nullptr);

        // Update saved device names
        audio_output_device_name_ = outputDevice->name;
        if (inputDevice) {
            audio_input_device_name_ = inputDevice->name;
        }

        ofLogNotice("ofxAVS") << "Audio setup @ " << settings.sampleRate << "Hz";
        ofLogNotice("ofxAVS") << "  Output: " << outputDevice->name;
        if (inputDevice) {
            ofLogNotice("ofxAVS") << "  Input: " << inputDevice->name;
        }
    } catch (...) {
        ofLogError("ofxAVS") << "Failed to setup audio stream";
    }
}

void ofxAVS::createFft() {
    if (fft_left_) {
        delete fft_left_;
        fft_left_ = nullptr;
    }
    if (fft_right_) {
        delete fft_right_;
        fft_right_ = nullptr;
    }

    if (audio_classic_mode_) {
        // Original Winamp: 512 samples, Hann window
        fft_left_ = ofxFft::create(FFT_SIZE_CLASSIC, OF_FFT_WINDOW_HANN);
        fft_right_ = ofxFft::create(FFT_SIZE_CLASSIC, OF_FFT_WINDOW_HANN);
    } else {
        // Modern: 2048 samples, Hamming window
        fft_left_ = ofxFft::create(FFT_SIZE_MODERN, OF_FFT_WINDOW_HAMMING);
        fft_right_ = ofxFft::create(FFT_SIZE_MODERN, OF_FFT_WINDOW_HAMMING);
        memset(smoothedSpectrumLeft_, 0, sizeof(smoothedSpectrumLeft_));
        memset(smoothedSpectrumRight_, 0, sizeof(smoothedSpectrumRight_));
    }
}

void ofxAVS::loadSoundFile(const std::string& path, bool autoPlay) {
    ofLogNotice("ofxAVS") << "Loading sound file: " << path;

    // Clear any loaded MIDI file - audio files loaded directly don't have paired MIDI
    if (midi_file_.isLoaded()) {
        midi_file_.clear();
        midi_loaded_filepath_.clear();
        midi_event_index_ = 0;
        avs::EventBus::instance().reset();
        ofLogNotice("ofxAVS") << "Cleared MIDI file (loading audio directly)";
    }

    if (AudioFile::load(audio_file_buffer_, path)) {
        audio_loaded_filename_ = ofFilePath::getFileName(path);
        audio_loaded_filepath_ = path;
        audio_playback_pos_ = 0;
        if (autoPlay) {
            audio_use_file_ = true;
            audio_is_playing_ = true;
        }

        // Resample to 44100 if needed
        if (audio_file_buffer_.getSampleRate() != 44100) {
            float ratio = 44100.0f / audio_file_buffer_.getSampleRate();
            audio_file_buffer_.resample(ratio);
            audio_file_buffer_.setSampleRate(44100);
        }

        ofLogNotice("ofxAVS") << "Loaded: " << audio_loaded_filename_
                              << " (" << audio_file_buffer_.getNumFrames() << " frames, "
                              << audio_file_buffer_.getNumChannels() << " channels)";
    } else {
        ofLogError("ofxAVS") << "Failed to load sound file: " << path;
    }
}

void ofxAVS::togglePlayback() {
    if (audio_use_file_ && audio_file_buffer_.getNumFrames() > 0) {
        if (audio_is_playing_) {
            audio_is_playing_ = false;
        } else {
            if (audio_playback_pos_ >= audio_file_buffer_.getNumFrames()) {
                audio_playback_pos_ = 0;
                midi_event_index_ = 0;  // Reset MIDI playback too
            }
            audio_is_playing_ = true;
        }
    }
}

void ofxAVS::toggleMidiFileDebug() {
    midi_debug_ = !midi_debug_;
    ofLogNotice("ofxAVS") << "MIDI file debug " << (midi_debug_ ? "ON" : "OFF");
}

void ofxAVS::loadMidiFile(const std::string& path) {
    ofLogNotice("ofxAVS") << "Loading MIDI file: " << path;

    if (midi_file_.load(path)) {
        midi_loaded_filepath_ = path;
        midi_event_index_ = 0;
        ofLogNotice("ofxAVS") << "Loaded MIDI: " << midi_file_.getEvents().size()
                              << " events, duration: " << midi_file_.getDuration() << "s"
                              << ", tempo: " << midi_file_.getTempo() << " BPM";
    } else {
        ofLogError("ofxAVS") << "Failed to load MIDI file: " << midi_file_.getError();
    }
}

bool ofxAVS::loadCatalogue(const std::string& jsonPath) {
    ofLogNotice("ofxAVS") << "Loading catalogue: " << jsonPath;

    ofJson json;
    try {
        ofFile file(jsonPath);
        if (!file.exists()) {
            ofLogError("ofxAVS") << "Catalogue file not found: " << jsonPath;
            return false;
        }
        file >> json;
    } catch (const std::exception& e) {
        ofLogError("ofxAVS") << "Failed to parse catalogue JSON: " << e.what();
        return false;
    }

    // Get directory of catalogue file for relative paths
    std::string baseDir = ofFilePath::getEnclosingDirectory(jsonPath);

    // Load audio file
    if (json.contains("audio")) {
        std::string audioPath = json["audio"].get<std::string>();
        // Handle relative paths
        if (!ofFilePath::isAbsolute(audioPath)) {
            audioPath = baseDir + audioPath;
        }
        loadSoundFile(audioPath, false);  // Don't auto-play yet
    }

    // Load MIDI file
    if (json.contains("midi")) {
        std::string midiPath = json["midi"].get<std::string>();
        // Handle relative paths
        if (!ofFilePath::isAbsolute(midiPath)) {
            midiPath = baseDir + midiPath;
        }
        loadMidiFile(midiPath);
    }

    // Start playback
    if (audio_file_buffer_.getNumFrames() > 0) {
        audio_playback_pos_ = 0;
        midi_event_index_ = 0;
        audio_use_file_ = true;
        audio_is_playing_ = true;
    }

    return true;
}

void ofxAVS::updateMidiPlayback() {
    if (!midi_file_.isLoaded() || !audio_is_playing_ || !audio_use_file_) {
        return;
    }

    const auto& events = midi_file_.getEvents();
    if (midi_event_index_ >= events.size()) {
        return;
    }

    // Calculate current playback time in seconds
    double currentTime = static_cast<double>(audio_playback_pos_) / 44100.0;

    // Process all events up to current time
    while (midi_event_index_ < events.size() && events[midi_event_index_].time <= currentTime) {
        const auto& evt = events[midi_event_index_];

        // Print MIDI events (toggle with 'M' key)
        if (midi_debug_) {
            const char* typeStr = "???";
            switch (evt.type()) {
                case avs::MidiFileEvent::NOTE_ON: typeStr = "NOTE_ON"; break;
                case avs::MidiFileEvent::NOTE_OFF: typeStr = "NOTE_OFF"; break;
                case avs::MidiFileEvent::CONTROL_CHANGE: typeStr = "CC"; break;
                case avs::MidiFileEvent::PITCH_BEND: typeStr = "PITCH"; break;
                case avs::MidiFileEvent::PROGRAM_CHANGE: typeStr = "PROG"; break;
            }
            ofLogNotice("MIDI") << std::fixed << std::setprecision(3) << evt.time << "s "
                                << typeStr << " ch=" << evt.channel()
                                << " d1=" << (int)evt.data1 << " d2=" << (int)evt.data2;
        }

        // Push to EventBus
        avs::Event busEvent;
        busEvent.channel = evt.channel();
        busEvent.data1 = evt.data1;
        busEvent.data2 = evt.data2;
        busEvent.timestamp = evt.time;

        switch (evt.type()) {
            case avs::MidiFileEvent::NOTE_ON:
                busEvent.type = avs::Event::Type::MIDI_NOTE_ON;
                avs::EventBus::instance().push_event(busEvent);
                break;
            case avs::MidiFileEvent::NOTE_OFF:
                busEvent.type = avs::Event::Type::MIDI_NOTE_OFF;
                avs::EventBus::instance().push_event(busEvent);
                break;
            case avs::MidiFileEvent::CONTROL_CHANGE:
                busEvent.type = avs::Event::Type::MIDI_CC;
                avs::EventBus::instance().push_event(busEvent);
                break;
            case avs::MidiFileEvent::PITCH_BEND:
                // Pitch bend: combine data1 (LSB) and data2 (MSB) into 14-bit value
                busEvent.type = avs::Event::Type::MIDI_PITCH_BEND;
                busEvent.data1 = (evt.data2 << 7) | evt.data1;  // 14-bit value
                avs::EventBus::instance().push_event(busEvent);
                break;
        }

        midi_event_index_++;
    }
}

void ofxAVS::audioOut(ofSoundBuffer& buffer) {
    if (buffer.getNumFrames() == 0 || buffer.getNumChannels() == 0) {
        return;
    }

    // Only output audio if playing a file
    if (!audio_use_file_ || !audio_is_playing_ || audio_file_buffer_.getNumFrames() == 0) {
        buffer.set(0);
        return;
    }

    size_t numFrames = buffer.getNumFrames();
    size_t outChannels = buffer.getNumChannels();
    size_t fileChannels = audio_file_buffer_.getNumChannels();
    size_t bufferSize = buffer.size();

    for (size_t i = 0; i < numFrames; i++) {
        if (audio_playback_pos_ >= audio_file_buffer_.getNumFrames()) {
            audio_playback_pos_ = 0;  // Loop
            midi_event_index_ = 0;    // Reset MIDI too
        }

        float left = audio_file_buffer_.getSample(audio_playback_pos_, 0);
        float right = (fileChannels >= 2) ? audio_file_buffer_.getSample(audio_playback_pos_, 1) : left;

        size_t idx0 = i * outChannels;
        size_t idx1 = idx0 + 1;
        if (outChannels >= 2 && idx1 < bufferSize) {
            buffer[idx0] = left;
            buffer[idx1] = right;
        } else if (outChannels == 1 && idx0 < bufferSize) {
            buffer[idx0] = (left + right) * 0.5f;
        }

        audio_playback_pos_++;
    }

    // Pass audio to visualization
    audioIn(buffer);
}

void ofxAVS::loadAudioSettings() {
    std::string path = ofToDataPath("audio_settings.json");
    if (!ofFile::doesFileExist(path)) return;

    try {
        ofJson json = ofLoadJson(path);

        if (json.contains("input_device") && !json["input_device"].is_null()) {
            audio_input_device_name_ = json["input_device"].get<std::string>();
        }
        if (json.contains("output_device") && !json["output_device"].is_null()) {
            audio_output_device_name_ = json["output_device"].get<std::string>();
        }
        if (json.contains("sound_file") && !json["sound_file"].is_null()) {
            std::string soundPath = json["sound_file"].get<std::string>();
            if (!soundPath.empty() && ofFile::doesFileExist(soundPath)) {
                loadSoundFile(soundPath, false);
            }
        }
        if (json.contains("midi_file") && !json["midi_file"].is_null()) {
            std::string midiPath = json["midi_file"].get<std::string>();
            if (!midiPath.empty() && ofFile::doesFileExist(midiPath)) {
                loadMidiFile(midiPath);
            }
        }
        if (json.contains("mic_gain")) {
            audio_mic_gain_ = json["mic_gain"].get<float>();
            audio_mic_gain_ = ofClamp(audio_mic_gain_, 1.0f, 100.0f);
        }
        if (json.contains("use_file_input")) {
            audio_use_file_ = json["use_file_input"].get<bool>();
        }
        if (json.contains("midi_debug")) {
            midi_debug_ = json["midi_debug"].get<bool>();
        }
        // Live MIDI input settings
        if (json.contains("midi_input_device") && !json["midi_input_device"].is_null()) {
            midi_input_device_name_ = json["midi_input_device"].get<std::string>();
            if (!midi_input_device_name_.empty()) {
                midi_input_.openDevice(midi_input_device_name_);
            }
        }
        if (json.contains("midi_input_channel")) {
            midi_input_channel_ = json["midi_input_channel"].get<int>();
            midi_input_.setChannel(midi_input_channel_);
        }
        if (json.contains("midi_input_debug")) {
            midi_input_.setDebugEnabled(json["midi_input_debug"].get<bool>());
        }
        if (json.contains("classic_audio")) {
            audio_classic_mode_ = json["classic_audio"].get<bool>();
            createFft();  // Recreate FFT for loaded mode
        }
        ofLogNotice("ofxAVS") << "Loaded audio settings";
    } catch (const std::exception& e) {
        ofLogWarning("ofxAVS") << "Failed to load audio settings: " << e.what();
    }
}

void ofxAVS::saveAudioSettings() {
    ofJson json;
    json["sound_file"] = audio_loaded_filepath_;
    json["midi_file"] = midi_loaded_filepath_;
    json["midi_debug"] = midi_debug_;
    json["use_file_input"] = audio_use_file_;
    json["mic_gain"] = audio_mic_gain_;
    json["input_device"] = audio_input_device_name_;
    json["output_device"] = audio_output_device_name_;
    // Live MIDI input settings
    json["midi_input_device"] = midi_input_device_name_;
    json["midi_input_channel"] = midi_input_channel_;
    json["midi_input_debug"] = midi_input_.isDebugEnabled();
    // Audio processing mode
    json["classic_audio"] = audio_classic_mode_;

    std::string path = ofToDataPath("audio_settings.json");
    if (ofSaveJson(path, json)) {
        ofLogNotice("ofxAVS") << "Saved audio settings";
    } else {
        ofLogWarning("ofxAVS") << "Failed to save audio settings";
    }
}

void ofxAVS::drawAudioUI() {
    float availWidth = ImGui::GetContentRegionAvail().x;
    bool wideLayout = availWidth > 400;
    float comboWidth = wideLayout ? 250.0f : availWidth - 60;

    // Row 1: Input device
    ImGui::Text("Input:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(comboWidth);

    const char* inputLabel = "None";
    if (audio_selected_input_ >= 0 && audio_selected_input_ < (int)audio_input_indices_.size()) {
        inputLabel = audio_devices_[audio_input_indices_[audio_selected_input_]].name.c_str();
    }

    if (ImGui::BeginCombo("##input_device", inputLabel)) {
        if (ImGui::Selectable("None", audio_selected_input_ < 0)) {
            if (audio_selected_input_ >= 0) {
                audio_selected_input_ = -1;
                restartAudio();
            }
        }
        if (audio_selected_input_ < 0) ImGui::SetItemDefaultFocus();

        for (size_t i = 0; i < audio_input_indices_.size(); i++) {
            bool isSelected = ((int)i == audio_selected_input_);
            if (ImGui::Selectable(audio_devices_[audio_input_indices_[i]].name.c_str(), isSelected)) {
                if ((int)i != audio_selected_input_) {
                    audio_selected_input_ = i;
                    restartAudio();
                }
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Output device - same line if wide, new line if narrow
    if (wideLayout) {
        ImGui::SameLine();
    }
    ImGui::Text("Output:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(comboWidth);
    if (ImGui::BeginCombo("##output_device",
        (audio_selected_output_ >= 0 && audio_selected_output_ < (int)audio_output_indices_.size())
            ? audio_devices_[audio_output_indices_[audio_selected_output_]].name.c_str()
            : "None")) {
        for (size_t i = 0; i < audio_output_indices_.size(); i++) {
            bool isSelected = ((int)i == audio_selected_output_);
            if (ImGui::Selectable(audio_devices_[audio_output_indices_[i]].name.c_str(), isSelected)) {
                if ((int)i != audio_selected_output_) {
                    audio_selected_output_ = i;
                    restartAudio();
                }
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Classic audio checkbox
    ImGui::SameLine();
    if (ImGui::Checkbox("Classic", &audio_classic_mode_)) {
        createFft();  // Recreate FFT for new mode
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    // Audio source toggle
    if (audio_has_input_) {
        if (ImGui::RadioButton("Microphone", !audio_use_file_)) {
            if (audio_use_file_) {
                audio_is_playing_ = false;
                audio_use_file_ = false;
            }
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Sound File", audio_use_file_)) {
            audio_use_file_ = true;
        }
    } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Sound File (no mic)");
        audio_use_file_ = true;
    }

    // Player controls - same line if wide, new line if narrow
    if (wideLayout) {
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
    }

    if (audio_use_file_) {
        if (audio_file_buffer_.getNumFrames() > 0) {
            if (ImGui::Button(audio_is_playing_ ? "Stop" : "Play", ImVec2(60, 0))) {
                togglePlayback();
            }
            ImGui::SameLine();

            // Progress bar takes available width minus space for percentage
            float percentWidth = 45.0f;  // Space for "100%"
            float progressWidth = ImGui::GetContentRegionAvail().x - percentWidth;
            if (progressWidth < 50.0f) progressWidth = 50.0f;

            float progress = (float)audio_playback_pos_ / audio_file_buffer_.getNumFrames();

            // Draw progress bar with empty overlay text (we'll draw our own)
            ImGui::ProgressBar(progress, ImVec2(progressWidth, 0), "");

            // Get progress bar rect to overlay filename
            ImVec2 barMin = ImGui::GetItemRectMin();
            ImVec2 barMax = ImGui::GetItemRectMax();

            // Overlay filename text centered vertically, left-aligned with padding
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 textPos(barMin.x + 4, barMin.y + (barMax.y - barMin.y - ImGui::GetTextLineHeight()) * 0.5f);

            // Clip text to progress bar bounds
            drawList->PushClipRect(barMin, barMax, true);
            drawList->AddText(textPos, ImGui::GetColorU32(ImGuiCol_Text), audio_loaded_filename_.c_str());
            drawList->PopClipRect();

            // Invisible button over progress bar for click/drag interaction
            ImGui::SetCursorScreenPos(barMin);
            ImGui::InvisibleButton("##seekbar", ImVec2(barMax.x - barMin.x, barMax.y - barMin.y));

            // Handle click or drag on progress bar
            if (ImGui::IsItemActive()) {
                ImVec2 mousePos = ImGui::GetMousePos();
                float seekPos = (mousePos.x - barMin.x) / progressWidth;
                seekPos = ofClamp(seekPos, 0.0f, 1.0f);
                audio_playback_pos_ = (size_t)(seekPos * audio_file_buffer_.getNumFrames());

                // Sync MIDI playback to new position
                if (midi_file_.isLoaded()) {
                    double seekTime = seekPos * (audio_file_buffer_.getNumFrames() / 44100.0);
                    const auto& events = midi_file_.getEvents();
                    // Find first event at or after seek time
                    midi_event_index_ = 0;
                    for (size_t i = 0; i < events.size(); i++) {
                        if (events[i].time >= seekTime) {
                            midi_event_index_ = i;
                            break;
                        }
                        midi_event_index_ = events.size();  // Past end if none found
                    }
                    // Clear MIDI state (all notes off)
                    avs::EventBus::instance().reset();
                }
            }

            // Percentage after progress bar
            ImGui::SetCursorScreenPos(ImVec2(barMax.x + 4, barMin.y));
            ImGui::Text("%3.0f%%", progress * 100.0f);
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Drop audio file here");
        }
    } else {
        ImGui::Text("Mic Gain:");
        ImGui::SameLine();
        float gainWidth = wideLayout ? 200.0f : std::min(availWidth - 80, 120.0f);
        ImGui::SetNextItemWidth(gainWidth);
        ImGui::SliderFloat("##micgain", &audio_mic_gain_, 1.0f, 100.0f, "%.0fx");
    }

    ImGui::Spacing();
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::Spacing();

    // MIDI Input
    ImGui::Text("MIDI:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(comboWidth);

    const char* midiLabel = "None";
    std::string midiDeviceName;
    if (midi_input_.isOpen()) {
        midiDeviceName = midi_input_.getDeviceName();
        midiLabel = midiDeviceName.c_str();
    }

    // Cache device list (refreshed when combo opens)
    static std::vector<std::string> midiDevices;
    if (ImGui::BeginCombo("##midi_device", midiLabel)) {
        // Refresh device list each time combo opens
        midiDevices = midi_input_.getDeviceList();

        if (ImGui::Selectable("None", !midi_input_.isOpen())) {
            midi_input_.closeDevice();
            midi_input_device_name_.clear();
        }
        if (!midi_input_.isOpen()) ImGui::SetItemDefaultFocus();

        for (size_t i = 0; i < midiDevices.size(); i++) {
            bool isSelected = midi_input_.isOpen() && midi_input_.getDeviceName() == midiDevices[i];
            if (ImGui::Selectable(midiDevices[i].c_str(), isSelected)) {
                if (midi_input_.openDevice(static_cast<int>(i))) {
                    midi_input_device_name_ = midiDevices[i];
                }
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // MIDI Channel
    if (wideLayout) {
        ImGui::SameLine();
    }
    ImGui::Text("Ch:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(70);

    const char* channelLabels[] = {
        "Omni", "1", "2", "3", "4", "5", "6", "7", "8",
        "9", "10", "11", "12", "13", "14", "15", "16"
    };
    int currentChannel = midi_input_.getChannel();
    if (ImGui::BeginCombo("##midi_channel", channelLabels[currentChannel])) {
        for (int i = 0; i <= 16; i++) {
            bool isSelected = (i == currentChannel);
            if (ImGui::Selectable(channelLabels[i], isSelected)) {
                midi_input_.setChannel(i);
                midi_input_channel_ = i;
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Debug button
    ImGui::SameLine();
    if (ImGui::Button("Debug")) {
        midi_input_.setDebugEnabled(!midi_input_.isDebugEnabled());
    }
}

void ofxAVS::drawMidiDebugWindow() {
    if (!midi_input_.isDebugEnabled()) return;

    ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);
    bool open = true;
    if (ImGui::Begin("MIDI Debug", &open)) {
        // Device info
        if (midi_input_.isOpen()) {
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Device: %s", midi_input_.getDeviceName().c_str());
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No device connected");
        }

        // Channel filter display
        int ch = midi_input_.getChannel();
        ImGui::SameLine();
        if (ch == 0) {
            ImGui::Text("| Ch: Omni");
        } else {
            ImGui::Text("| Ch: %d", ch);
        }

        ImGui::Separator();

        // Controls
        bool autoScroll = midi_input_.isAutoScroll();
        if (ImGui::Checkbox("Auto-scroll", &autoScroll)) {
            midi_input_.setAutoScroll(autoScroll);
        }
        ImGui::SameLine();
        if (ImGui::Button("Clear")) {
            midi_input_.clearDebugLog();
        }

        ImGui::Separator();

        // Log window
        ImGui::BeginChild("MidiLog", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
        const auto& log = midi_input_.getDebugLog();
        for (const auto& entry : log) {
            ImGui::Text("[%.2f] %s", entry.timestamp, entry.message.c_str());
        }
        if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
    }
    ImGui::End();

    if (!open) {
        midi_input_.setDebugEnabled(false);
    }
}