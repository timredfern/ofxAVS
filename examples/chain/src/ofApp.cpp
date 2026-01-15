// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "ofApp.h"
#include "ofJson.h"

// App settings file location
static std::string getAppSettingsPath() {
    return ofToDataPath("app_settings.json");
}

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetWindowTitle("AVS Chain Example");
    ofSetWindowShape(1520, 640);  // Wider to fit SuperScope UI (233x214)

    gui.setup();
    avs.setup();

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

        // Build device index lists
        if (device.inputChannels > 0) {
            inputDeviceIndices.push_back(i);
        }
        if (device.outputChannels > 0) {
            outputDeviceIndices.push_back(i);
        }
    }

    // Helper to check if device supports standard sample rates
    auto supportsStandardRates = [](const ofSoundDevice& device) {
        if (device.sampleRates.empty()) return true;  // Assume yes if not reported
        for (unsigned int rate : {44100u, 48000u, 96000u}) {
            if (std::find(device.sampleRates.begin(), device.sampleRates.end(), rate)
                != device.sampleRates.end()) {
                return true;
            }
        }
        return false;
    };

    // Select devices based on saved names or auto-detect
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
        selectedOutputDevice = 0;  // First available output
    }

    // Auto-select input if not found (prefer one with standard sample rates)
    if (selectedInputDevice < 0 && !inputDeviceIndices.empty()) {
        for (size_t i = 0; i < inputDeviceIndices.size(); i++) {
            auto& device = audioDevices[inputDeviceIndices[i]];
            if (supportsStandardRates(device)) {
                selectedInputDevice = i;
                break;
            }
        }
    }

    // Initialize audio stream
    restartAudio();
}

//--------------------------------------------------------------
void ofApp::restartAudio() {
    // Stop existing stream
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
        return 44100;  // fallback
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

        // Update saved device names
        selectedOutputDeviceName = outputDevice->name;
        if (inputDevice) {
            selectedInputDeviceName = inputDevice->name;
        }

        ofLogNotice() << "Audio setup @ " << settings.sampleRate << "Hz";
        ofLogNotice() << "  Output: " << outputDevice->name;
        if (inputDevice) {
            ofLogNotice() << "  Input: " << inputDevice->name;
        }
    } catch (...) {
        ofLogError() << "Failed to setup audio stream";
    }
}

//--------------------------------------------------------------
void ofApp::update(){
    avs.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(0);
    avs.draw(900, 20, 600, 600);  // Moved right to fit wider parameters panel

    gui.begin();
    avs.drawUI();
    drawAudioControls();
    gui.end();
}

//--------------------------------------------------------------
void ofApp::drawAudioControls() {
    // Position at bottom left - larger window for device selectors
    ImGui::SetNextWindowPos(ImVec2(10, 640 - 135));
    ImGui::SetNextWindowSize(ImVec2(660, 75));

    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::Begin("Audio", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    // Device selection row - Input first, then Output
    ImGui::Text("Input:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(270);

    // Build input device label (with "None" option)
    const char* inputLabel = "None";
    if (selectedInputDevice >= 0 && selectedInputDevice < (int)inputDeviceIndices.size()) {
        inputLabel = audioDevices[inputDeviceIndices[selectedInputDevice]].name.c_str();
    }

    if (ImGui::BeginCombo("##input_device", inputLabel)) {
        // None option
        if (ImGui::Selectable("None", selectedInputDevice < 0)) {
            if (selectedInputDevice >= 0) {
                selectedInputDevice = -1;
                restartAudio();
            }
        }
        if (selectedInputDevice < 0) ImGui::SetItemDefaultFocus();

        // Input devices
        for (size_t i = 0; i < inputDeviceIndices.size(); i++) {
            bool isSelected = ((int)i == selectedInputDevice);
            if (ImGui::Selectable(audioDevices[inputDeviceIndices[i]].name.c_str(), isSelected)) {
                if ((int)i != selectedInputDevice) {
                    selectedInputDevice = i;
                    restartAudio();
                }
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    ImGui::Text("Output:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(270);
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
            if (isSelected) ImGui::SetItemDefaultFocus();
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
        // No mic available - file input only
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Sound File (no mic)");
        useFileInput = true;
    }

    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();

    if (useFileInput) {
        if (audioFileBuffer.getNumFrames() > 0) {
            // Play/Stop button
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

            // Show file name (truncated if needed)
            ImGui::SetNextItemWidth(250);
            ImGui::Text("%s", loadedFileName.c_str());
            ImGui::SameLine();

            // Progress bar
            float progress = (float)playbackPos / audioFileBuffer.getNumFrames();
            ImGui::ProgressBar(progress, ImVec2(150, 0));

            // Click on progress bar to seek
            if (ImGui::IsItemClicked()) {
                ImVec2 mousePos = ImGui::GetMousePos();
                ImVec2 itemPos = ImGui::GetItemRectMin();
                float seekPos = (mousePos.x - itemPos.x) / 150.0f;
                seekPos = ofClamp(seekPos, 0.0f, 1.0f);
                playbackPos = (size_t)(seekPos * audioFileBuffer.getNumFrames());
            }
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Drop audio file here");
        }
    } else {
        ImGui::Text("Mic Gain:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        ImGui::SliderFloat("##micgain", &micGain, 1.0f, 100.0f, "%.0fx");
    }

    ImGui::End();
    ImGui::PopStyleColor(2);
}

//--------------------------------------------------------------
void ofApp::loadSoundFile(const std::string& path, bool autoPlay) {
    ofLogNotice() << "Loading sound file: " << path;

    if (ofxAudioDecoder::load(audioFileBuffer, path)) {
        loadedFileName = ofFilePath::getFileName(path);
        loadedFilePath = path;  // Store full path for persistence
        playbackPos = 0;
        if (autoPlay) {
            useFileInput = true;
            isPlaying = true;
        }

        // Resample to 44100 if needed
        if (audioFileBuffer.getSampleRate() != 44100) {
            float ratio = 44100.0f / audioFileBuffer.getSampleRate();
            audioFileBuffer.resample(ratio);
            audioFileBuffer.setSampleRate(44100);
        }

        ofLogNotice() << "Loaded: " << loadedFileName
                      << " (" << audioFileBuffer.getNumFrames() << " frames, "
                      << audioFileBuffer.getNumChannels() << " channels)";
    } else {
        ofLogError() << "Failed to load sound file: " << path;
    }
}

//--------------------------------------------------------------
void ofApp::audioIn(ofSoundBuffer& buffer) {
    static bool firstCall = true;
    if (firstCall) {
        ofLogNotice() << "audioIn first call - frames:" << buffer.getNumFrames()
                      << " channels:" << buffer.getNumChannels();
        firstCall = false;
    }
    // Safety check
    if (buffer.getNumFrames() == 0) {
        return;
    }
    // Use mic input when not playing file
    if (!useFileInput || !isPlaying) {
        // Apply mic gain if not 1.0
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
    static bool firstCall = true;
    if (firstCall) {
        ofLogNotice() << "audioOut first call - frames:" << buffer.getNumFrames()
                      << " channels:" << buffer.getNumChannels()
                      << " size:" << buffer.size();
        firstCall = false;
    }
    // Safety check
    if (buffer.getNumFrames() == 0 || buffer.getNumChannels() == 0) {
        return;
    }

    // Only output audio if playing a file
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
            // Loop back to start
            playbackPos = 0;
        }

        float left = audioFileBuffer.getSample(playbackPos, 0);
        float right = (fileChannels >= 2) ? audioFileBuffer.getSample(playbackPos, 1) : left;

        // Write to output buffer using array access with bounds check
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

    // Pass audio to AVS for visualization
    avs.audioIn(buffer);
}

//--------------------------------------------------------------
void ofApp::exit(){
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

        // Load audio device names (before setupAudioDevices is called)
        if (json.contains("input_device") && !json["input_device"].is_null()) {
            selectedInputDeviceName = json["input_device"].get<std::string>();
        }
        if (json.contains("output_device") && !json["output_device"].is_null()) {
            selectedOutputDeviceName = json["output_device"].get<std::string>();
        }

        // Load sound file (but don't auto-play - mic is default on startup)
        if (json.contains("sound_file") && !json["sound_file"].is_null()) {
            std::string soundPath = json["sound_file"].get<std::string>();
            if (!soundPath.empty() && ofFile::doesFileExist(soundPath)) {
                loadSoundFile(soundPath, false);  // Load but don't play, mic stays active
            }
        }
        if (json.contains("mic_gain")) {
            micGain = json["mic_gain"].get<float>();
            micGain = ofClamp(micGain, 1.0f, 100.0f);
        }
        ofLogNotice("ofApp") << "Loaded app settings from " << path;
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
    if (ofSaveJson(path, json)) {
        ofLogNotice("ofApp") << "Saved app settings to " << path;
    } else {
        ofLogWarning("ofApp") << "Failed to save app settings";
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if (key == ' ') {
        if (useFileInput && audioFileBuffer.getNumFrames() > 0) {
            if (isPlaying) {
                isPlaying = false;
            } else {
                if (playbackPos >= audioFileBuffer.getNumFrames()) {
                    playbackPos = 0;
                }
                isPlaying = true;
            }
        }
    }
    if (key == 'p' || key == 'P') {
        avs.toggleProfiling();
    }
}

void ofApp::keyReleased(int key){}
void ofApp::mouseMoved(int x, int y){}
void ofApp::mouseDragged(int x, int y, int button){}
void ofApp::mousePressed(int x, int y, int button){}
void ofApp::mouseReleased(int x, int y, int button){}
void ofApp::mouseEntered(int x, int y){}
void ofApp::mouseExited(int x, int y){}
void ofApp::windowResized(int w, int h){}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
    if (dragInfo.files.size() > 0) {
        std::string path = dragInfo.files[0];
        std::string ext = ofFilePath::getFileExt(path);

        // Check if it's a preset file
        if (ext == "avs" || ext == "json") {
            if (avs.loadPreset(path)) {
                ofLogNotice() << "Loaded preset: " << ofFilePath::getFileName(path);
            } else {
                ofLogError() << "Failed to load preset: " << avs.getLastError();
            }
        } else {
            // Assume it's an audio file
            loadSoundFile(path);
        }
    }
}

void ofApp::gotMessage(ofMessage msg){}
