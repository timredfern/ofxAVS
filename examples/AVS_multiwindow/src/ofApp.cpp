// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "ofApp.h"
#include "ofJson.h"
#include "ofAppGLFWWindow.h"
#include "AVSui.h"

// App settings file location
static std::string getAppSettingsPath() {
    return ofToDataPath("app_settings.json");
}

//--------------------------------------------------------------
void ofApp::setWindows(std::shared_ptr<ofAppBaseWindow> chain, std::shared_ptr<ofAppBaseWindow> output) {
    chain_window_ = chain;
    output_window_ = output;
}

//--------------------------------------------------------------
void ofApp::setup() {
    chain_gui_.setup(chain_window_);  // Explicit window for multi-window support
    avs.setup();

    // Set callback for "Params" menu item
    avs.setOpenParamsCallback([this](avs::Configurable* c) {
        openParamWindow(c);
    });

    // Load previous AVS session (or start with empty chain)
    if (!avs.loadSession()) {
        avs.addEffect("Brightness");
        avs.addEffect("Oscilloscope");
    }

    // Load app settings first (to get saved device names)
    loadAppSettings();

    // Setup audio devices
    setupAudioDevices();
}

//--------------------------------------------------------------
void ofApp::setupAudioDevices() {
    // Get device list
    audioDevices = soundStream.getDeviceList();
    inputDeviceIndices.clear();
    outputDeviceIndices.clear();

    ofLogNotice() << "Available audio devices:";
    for (size_t i = 0; i < audioDevices.size(); i++) {
        auto& device = audioDevices[i];
        std::string rates;
        for (auto r : device.sampleRates) rates += std::to_string(r) + " ";
        ofLogNotice() << "  " << device.deviceID << ": " << device.name
                      << " (in:" << device.inputChannels << " out:" << device.outputChannels << ")"
                      << (rates.empty() ? "" : " rates: " + rates);

        if (device.inputChannels > 0) {
            inputDeviceIndices.push_back(i);
        }
        if (device.outputChannels > 0) {
            outputDeviceIndices.push_back(i);
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

    selectedInputDevice = -1;
    selectedOutputDevice = -1;

    // Try to find saved input device
    if (!selectedInputDeviceName.empty()) {
        for (size_t i = 0; i < inputDeviceIndices.size(); i++) {
            if (audioDevices[inputDeviceIndices[i]].name == selectedInputDeviceName) {
                selectedInputDevice = i;
                break;
            }
        }
    }

    // Try to find saved output device
    if (!selectedOutputDeviceName.empty()) {
        for (size_t i = 0; i < outputDeviceIndices.size(); i++) {
            if (audioDevices[outputDeviceIndices[i]].name == selectedOutputDeviceName) {
                selectedOutputDevice = i;
                break;
            }
        }
    }

    // Auto-select output if not found
    if (selectedOutputDevice < 0 && !outputDeviceIndices.empty()) {
        selectedOutputDevice = 0;
    }

    // Auto-select input if not found
    if (selectedInputDevice < 0 && !inputDeviceIndices.empty()) {
        for (size_t i = 0; i < inputDeviceIndices.size(); i++) {
            auto& device = audioDevices[inputDeviceIndices[i]];
            if (supportsStandardRates(device)) {
                selectedInputDevice = i;
                break;
            }
        }
    }

    restartAudio();
}

//--------------------------------------------------------------
void ofApp::restartAudio() {
    if (audioInitialized) {
        soundStream.stop();
        soundStream.close();
        audioInitialized = false;
        hasAudioInput = false;
    }

    if (selectedOutputDevice < 0 || outputDeviceIndices.empty()) {
        ofLogWarning() << "No output device selected";
        return;
    }

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

    ofSoundDevice* outputDevice = &audioDevices[outputDeviceIndices[selectedOutputDevice]];
    ofSoundDevice* inputDevice = (selectedInputDevice >= 0 && selectedInputDevice < (int)inputDeviceIndices.size())
        ? &audioDevices[inputDeviceIndices[selectedInputDevice]]
        : nullptr;

    ofSoundStreamSettings settings;
    settings.bufferSize = 576;
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
        soundStream.setup(settings);
        audioInitialized = true;
        hasAudioInput = (inputDevice != nullptr);

        selectedOutputDeviceName = outputDevice->name;
        if (inputDevice) {
            selectedInputDeviceName = inputDevice->name;
        }

        ofLogNotice() << "Audio setup @ " << settings.sampleRate << "Hz";
    } catch (...) {
        ofLogError() << "Failed to setup audio stream";
    }
}

//--------------------------------------------------------------
void ofApp::update() {
    avs.update();
    cleanupInvalidParamWindows();
}

//--------------------------------------------------------------
void ofApp::draw() {
    // Chain window - effect tree and audio controls only
    ofBackground(40);

    chain_gui_.begin();

    // Draw chain panel using the public method
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(ofGetWidth(), ofGetHeight() - 140));
    ImGui::Begin("Chain", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    avs.drawChainPanel();
    ImGui::End();

    // Draw audio controls at bottom
    drawAudioControls();

    chain_gui_.end();
    chain_gui_.draw();  // Actually render
}

//--------------------------------------------------------------
void ofApp::drawOutput(ofEventArgs& args) {
    // Output window - visualization only
    ofBackground(0);

    int w = ofGetWidth();
    int h = ofGetHeight();
    avs.draw(0, 0, w, h);

    // Update title with FPS
    if (output_window_) {
        float fps = ofGetFrameRate();
        output_window_->setWindowTitle("AVS Output - " + ofToString(fps, 1) + " FPS");
    }
}

//--------------------------------------------------------------
void ofApp::keyPressedOutput(ofKeyEventArgs& args) {
    if (args.key == ' ') {
        // Toggle fullscreen on output window
        ofToggleFullscreen();
    }
}

//--------------------------------------------------------------
void ofApp::drawAudioControls() {
    ImGui::SetNextWindowPos(ImVec2(0, ofGetHeight() - 135));
    ImGui::SetNextWindowSize(ImVec2(ofGetWidth(), 135));

    ImGui::Begin("Audio", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    // Input device
    ImGui::Text("Input:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);

    const char* inputLabel = "None";
    if (selectedInputDevice >= 0 && selectedInputDevice < (int)inputDeviceIndices.size()) {
        inputLabel = audioDevices[inputDeviceIndices[selectedInputDevice]].name.c_str();
    }

    if (ImGui::BeginCombo("##input_device", inputLabel)) {
        if (ImGui::Selectable("None", selectedInputDevice < 0)) {
            if (selectedInputDevice >= 0) {
                selectedInputDevice = -1;
                restartAudio();
            }
        }
        for (size_t i = 0; i < inputDeviceIndices.size(); i++) {
            bool isSelected = ((int)i == selectedInputDevice);
            if (ImGui::Selectable(audioDevices[inputDeviceIndices[i]].name.c_str(), isSelected)) {
                if ((int)i != selectedInputDevice) {
                    selectedInputDevice = i;
                    restartAudio();
                }
            }
        }
        ImGui::EndCombo();
    }

    // Output device
    ImGui::Text("Output:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    if (ImGui::BeginCombo("##output_device",
        (selectedOutputDevice >= 0 && selectedOutputDevice < (int)outputDeviceIndices.size())
            ? audioDevices[outputDeviceIndices[selectedOutputDevice]].name.c_str()
            : "None")) {
        for (size_t i = 0; i < outputDeviceIndices.size(); i++) {
            bool isSelected = ((int)i == selectedOutputDevice);
            if (ImGui::Selectable(audioDevices[outputDeviceIndices[i]].name.c_str(), isSelected)) {
                if ((int)i != selectedOutputDevice) {
                    selectedOutputDevice = i;
                    restartAudio();
                }
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Separator();

    // Audio source toggle
    if (hasAudioInput) {
        if (ImGui::RadioButton("Microphone", !useFileInput)) {
            if (useFileInput) {
                isPlaying = false;
                useFileInput = false;
            }
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Sound File", useFileInput)) {
            useFileInput = true;
        }
    } else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Sound File (no mic)");
        useFileInput = true;
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    if (useFileInput) {
        if (audioFileBuffer.getNumFrames() > 0) {
            if (ImGui::Button(isPlaying ? "Stop" : "Play", ImVec2(60, 0))) {
                if (isPlaying) {
                    isPlaying = false;
                } else {
                    if (playbackPos >= audioFileBuffer.getNumFrames()) {
                        playbackPos = 0;
                    }
                    isPlaying = true;
                }
            }
            ImGui::SameLine();
            ImGui::Text("%s", loadedFileName.c_str());
            ImGui::SameLine();
            float progress = (float)playbackPos / audioFileBuffer.getNumFrames();
            ImGui::ProgressBar(progress, ImVec2(100, 0));
            if (ImGui::IsItemClicked()) {
                ImVec2 mousePos = ImGui::GetMousePos();
                ImVec2 itemPos = ImGui::GetItemRectMin();
                float seekPos = (mousePos.x - itemPos.x) / 100.0f;
                seekPos = ofClamp(seekPos, 0.0f, 1.0f);
                playbackPos = (size_t)(seekPos * audioFileBuffer.getNumFrames());
            }
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Drop audio file here");
        }
    } else {
        ImGui::Text("Mic Gain:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(150);
        ImGui::SliderFloat("##micgain", &micGain, 1.0f, 100.0f, "%.0fx");
    }

    ImGui::End();
}

//--------------------------------------------------------------
void ofApp::openParamWindow(avs::Configurable* configurable) {
    // Check if already open
    for (auto& pw : param_windows_) {
        if (pw->configurable == configurable) {
            return;
        }
    }

    // Create new window
    ofGLFWWindowSettings settings;
    settings.setSize(500, 460);
    settings.resizable = false;
    settings.title = configurable->get_display_name();
    if (chain_window_) {
        settings.shareContextWith = chain_window_;
    }

    auto window = ofCreateWindow(settings);

    auto info = std::make_unique<ParamWindowInfo>();
    info->window = window;
    info->gui = std::make_unique<ofxImGui::Gui>();
    // Don't setup here - defer to first draw when window context is active
    info->configurable = configurable;
    info->effect_id = reinterpret_cast<uintptr_t>(configurable);

    // Bind draw event - use member function that checks current window
    ofAddListener(window->events().draw, this, &ofApp::drawParamWindows);

    param_windows_.push_back(std::move(info));
}

//--------------------------------------------------------------
void ofApp::closeParamWindow(avs::Configurable* configurable) {
    for (auto it = param_windows_.begin(); it != param_windows_.end(); ) {
        if ((*it)->configurable == configurable) {
            // Remove listener before destroying window
            ofRemoveListener((*it)->window->events().draw, this, &ofApp::drawParamWindows);
            it = param_windows_.erase(it);
        } else {
            ++it;
        }
    }
}

//--------------------------------------------------------------
void ofApp::drawParamWindows(ofEventArgs& args) {
    // Find which param window is being drawn based on current window
    auto current_window = ofGetCurrentWindow();
    for (auto& info : param_windows_) {
        if (info->window == current_window) {
            drawParamWindow(*info);
            return;
        }
    }
}

//--------------------------------------------------------------
void ofApp::drawParamWindow(ParamWindowInfo& info) {
    // Validate effect still exists
    if (!isEffectValid(info.effect_id)) {
        return;  // Will be cleaned up in update()
    }

    // Deferred setup - do it on first draw when window context is active
    if (info.needs_setup) {
        info.gui->setup(nullptr, false, ImGuiConfigFlags_None, false, false);
        info.needs_setup = false;
    }

    ofBackground(40);

    info.gui->begin();

    avs_ui::renderParamWindowContent(info.configurable, ofGetWidth(), ofGetHeight());

    info.gui->end();
    info.gui->draw();  // Actually render
}

//--------------------------------------------------------------
void ofApp::cleanupInvalidParamWindows() {
    for (auto it = param_windows_.begin(); it != param_windows_.end(); ) {
        bool should_remove = false;

        // Check if effect was deleted
        if (!isEffectValid((*it)->effect_id)) {
            should_remove = true;
        }

        // Check if window was closed by user
        auto* glfw_window = dynamic_cast<ofAppGLFWWindow*>((*it)->window.get());
        if (glfw_window && glfw_window->getWindowShouldClose()) {
            should_remove = true;
        }

        if (should_remove) {
            ofRemoveListener((*it)->window->events().draw, this, &ofApp::drawParamWindows);
            it = param_windows_.erase(it);
        } else {
            ++it;
        }
    }
}

//--------------------------------------------------------------
bool ofApp::isEffectValid(uintptr_t id) {
    // Check if it's the root container
    if (reinterpret_cast<uintptr_t>(avs.getRoot()) == id) {
        return true;
    }
    // Check if it's the beat detector
    if (reinterpret_cast<uintptr_t>(avs.getBeatDetector()) == id) {
        return true;
    }
    // Check if it's an effect in the tree
    return findEffectById(avs.getRoot(), id) != nullptr;
}

//--------------------------------------------------------------
avs::EffectBase* ofApp::findEffectById(avs::EffectContainer* container, uintptr_t id) {
    for (size_t i = 0; i < container->child_count(); i++) {
        auto* effect = container->get_child(i);
        if (reinterpret_cast<uintptr_t>(effect) == id) {
            return effect;
        }
        if (auto* child_container = dynamic_cast<avs::EffectContainer*>(effect)) {
            if (auto* found = findEffectById(child_container, id)) {
                return found;
            }
        }
    }
    return nullptr;
}

//--------------------------------------------------------------
void ofApp::loadSoundFile(const std::string& path, bool autoPlay) {
    ofLogNotice() << "Loading sound file: " << path;

    if (ofxAudioDecoder::load(audioFileBuffer, path)) {
        loadedFileName = ofFilePath::getFileName(path);
        loadedFilePath = path;
        playbackPos = 0;
        if (autoPlay) {
            useFileInput = true;
            isPlaying = true;
        }

        if (audioFileBuffer.getSampleRate() != 44100) {
            float ratio = 44100.0f / audioFileBuffer.getSampleRate();
            audioFileBuffer.resample(ratio);
            audioFileBuffer.setSampleRate(44100);
        }

        ofLogNotice() << "Loaded: " << loadedFileName;
    } else {
        ofLogError() << "Failed to load sound file: " << path;
    }
}

//--------------------------------------------------------------
void ofApp::audioIn(ofSoundBuffer& buffer) {
    if (buffer.getNumFrames() == 0) return;

    if (!useFileInput || !isPlaying) {
        if (micGain != 1.0f) {
            ofSoundBuffer amplified = buffer;
            for (size_t i = 0; i < amplified.size(); i++) {
                amplified[i] = ofClamp(amplified[i] * micGain, -1.0f, 1.0f);
            }
            avs.audioIn(amplified);
        } else {
            avs.audioIn(buffer);
        }
    }
}

//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer& buffer) {
    if (buffer.getNumFrames() == 0 || buffer.getNumChannels() == 0) return;

    if (!useFileInput || !isPlaying || audioFileBuffer.getNumFrames() == 0) {
        buffer.set(0);
        return;
    }

    size_t numFrames = buffer.getNumFrames();
    size_t outChannels = buffer.getNumChannels();
    size_t fileChannels = audioFileBuffer.getNumChannels();
    size_t bufferSize = buffer.size();

    for (size_t i = 0; i < numFrames; i++) {
        if (playbackPos >= audioFileBuffer.getNumFrames()) {
            playbackPos = 0;
        }

        float left = audioFileBuffer.getSample(playbackPos, 0);
        float right = (fileChannels >= 2) ? audioFileBuffer.getSample(playbackPos, 1) : left;

        size_t idx0 = i * outChannels;
        size_t idx1 = idx0 + 1;
        if (outChannels >= 2 && idx1 < bufferSize) {
            buffer[idx0] = left;
            buffer[idx1] = right;
        } else if (outChannels == 1 && idx0 < bufferSize) {
            buffer[idx0] = (left + right) * 0.5f;
        }

        playbackPos++;
    }

    avs.audioIn(buffer);
}

//--------------------------------------------------------------
void ofApp::exit() {
    avs.saveSession();
    saveAppSettings();

    if (audioInitialized) {
        soundStream.stop();
        soundStream.close();
    }
}

//--------------------------------------------------------------
void ofApp::loadAppSettings() {
    std::string path = getAppSettingsPath();
    if (!ofFile::doesFileExist(path)) return;

    try {
        ofJson json = ofLoadJson(path);

        if (json.contains("input_device") && !json["input_device"].is_null()) {
            selectedInputDeviceName = json["input_device"].get<std::string>();
        }
        if (json.contains("output_device") && !json["output_device"].is_null()) {
            selectedOutputDeviceName = json["output_device"].get<std::string>();
        }
        if (json.contains("sound_file") && !json["sound_file"].is_null()) {
            std::string soundPath = json["sound_file"].get<std::string>();
            if (!soundPath.empty() && ofFile::doesFileExist(soundPath)) {
                loadSoundFile(soundPath, false);
            }
        }
        if (json.contains("mic_gain")) {
            micGain = json["mic_gain"].get<float>();
            micGain = ofClamp(micGain, 1.0f, 100.0f);
        }
    } catch (const std::exception& e) {
        ofLogWarning("ofApp") << "Failed to load app settings: " << e.what();
    }
}

//--------------------------------------------------------------
void ofApp::saveAppSettings() {
    ofJson json;
    json["sound_file"] = loadedFilePath;
    json["use_file_input"] = useFileInput;
    json["mic_gain"] = micGain;
    json["input_device"] = selectedInputDeviceName;
    json["output_device"] = selectedOutputDeviceName;

    std::string path = getAppSettingsPath();
    ofSaveJson(path, json);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    if (key == 'p' || key == 'P') {
        avs.toggleProfiling();
    }
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {
    if (dragInfo.files.size() > 0) {
        std::string path = dragInfo.files[0];
        std::string ext = ofFilePath::getFileExt(path);

        if (ext == "avs" || ext == "json") {
            if (avs.loadPreset(path)) {
                ofLogNotice() << "Loaded preset: " << ofFilePath::getFileName(path);
            } else {
                ofLogError() << "Failed to load preset: " << avs.getLastError();
            }
        } else {
            loadSoundFile(path);
        }
    }
}
