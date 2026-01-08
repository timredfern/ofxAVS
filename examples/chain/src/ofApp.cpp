// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetWindowTitle("AVS Chain Example");
    ofSetWindowShape(1520, 640);  // Wider to fit SuperScope UI (233x214)

    gui.setup();
    avs.setup();

    // Setup audio - configure for both input (mic) and output (file playback)
    vector<ofSoundDevice> devices = soundStream.getDeviceList();

    ofLogNotice() << "Available audio devices:";
    for (auto& device : devices) {
        std::string rates;
        for (auto r : device.sampleRates) rates += std::to_string(r) + " ";
        ofLogNotice() << "  " << device.deviceID << ": " << device.name
                      << " (in:" << device.inputChannels << " out:" << device.outputChannels << ")"
                      << (rates.empty() ? "" : " rates: " + rates);
    }

    // Find devices - prefer combo device, otherwise separate
    ofSoundDevice* comboDevice = nullptr;
    ofSoundDevice* outputOnlyDevice = nullptr;
    ofSoundDevice* inputOnlyDevice = nullptr;

    // First pass: find output device (prefer stereo, but accept mono)
    for (auto& device : devices) {
        if (device.inputChannels > 0 && device.outputChannels >= 1) {
            if (!comboDevice) comboDevice = &device;
        } else if (device.outputChannels >= 1) {
            if (!outputOnlyDevice) outputOnlyDevice = &device;
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

    // Second pass: find input device that works with our output
    for (auto& device : devices) {
        if (device.inputChannels > 0 && device.outputChannels == 0) {
            // Skip if same name as output device (e.g., Sony XM4 mic when using Sony XM4 output)
            if (outputOnlyDevice && device.name == outputOnlyDevice->name) {
                continue;
            }
            // Skip if doesn't support standard sample rates (e.g., Bluetooth mics)
            if (!supportsStandardRates(device)) {
                ofLogNotice() << "  Skipping " << device.name << " (no standard sample rates)";
                continue;
            }
            if (!inputOnlyDevice) inputOnlyDevice = &device;
        }
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

    ofSoundStreamSettings settings;
    settings.bufferSize = 576;
    settings.setOutListener(this);

    // Strategy 1: Use combo device (same device for in+out)
    if (comboDevice) {
        settings.sampleRate = findCommonRate(comboDevice, nullptr);
        settings.numOutputChannels = std::min(2u, comboDevice->outputChannels);
        settings.numInputChannels = 1;
        settings.setOutDevice(*comboDevice);
        settings.setInDevice(*comboDevice);
        settings.setInListener(this);
        try {
            soundStream.setup(settings);
            audioInitialized = true;
            hasAudioInput = true;
            ofLogNotice() << "Audio setup (combo): " << comboDevice->name
                          << " @ " << settings.sampleRate << "Hz, " << settings.numOutputChannels << "ch";
        } catch (...) {
            ofLogWarning() << "Failed with combo device: " << comboDevice->name;
        }
    }

    // Strategy 2: Output + separate mic
    if (!audioInitialized && outputOnlyDevice && inputOnlyDevice) {
        settings.sampleRate = findCommonRate(outputOnlyDevice, inputOnlyDevice);
        settings.numOutputChannels = std::min(2u, outputOnlyDevice->outputChannels);
        settings.numInputChannels = 1;
        settings.setOutDevice(*outputOnlyDevice);
        settings.setInDevice(*inputOnlyDevice);
        settings.setInListener(this);

        try {
            soundStream.setup(settings);
            audioInitialized = true;
            hasAudioInput = true;
            ofLogNotice() << "Audio setup @ " << settings.sampleRate << "Hz, " << settings.numOutputChannels << "ch out";
            ofLogNotice() << "  out: " << outputOnlyDevice->name;
            ofLogNotice() << "  in: " << inputOnlyDevice->name;
        } catch (...) {
            ofLogWarning() << "Failed with output + input";
        }
    }

    // Strategy 3: Output only
    if (!audioInitialized && outputOnlyDevice) {
        settings.sampleRate = findCommonRate(outputOnlyDevice, nullptr);
        settings.numOutputChannels = std::min(2u, outputOnlyDevice->outputChannels);
        settings.numInputChannels = 0;
        settings.setOutDevice(*outputOnlyDevice);
        try {
            soundStream.setup(settings);
            audioInitialized = true;
            hasAudioInput = false;
            ofLogNotice() << "Audio setup (output only): " << outputOnlyDevice->name
                          << " @ " << settings.sampleRate << "Hz, " << settings.numOutputChannels << "ch";
        } catch (...) {
            ofLogWarning() << "Failed with output device";
        }
    }

    if (!audioInitialized) {
        ofLogWarning() << "No audio device available";
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
    // Position at bottom left
    ImGui::SetNextWindowPos(ImVec2(10, 640 - 80));
    ImGui::SetNextWindowSize(ImVec2(400, 70));

    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::Begin("Audio", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

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

            // Show file name and progress
            ImGui::Text("%s", loadedFileName.c_str());
            ImGui::SameLine();

            float progress = (float)playbackPos / audioFileBuffer.getNumFrames();
            ImGui::ProgressBar(progress, ImVec2(100, 0));

            // Click on progress bar to seek
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
        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Listening to microphone...");
    }

    ImGui::End();
    ImGui::PopStyleColor(2);
}

//--------------------------------------------------------------
void ofApp::loadSoundFile(const std::string& path) {
    ofLogNotice() << "Loading sound file: " << path;

    if (ofxAudioDecoder::load(audioFileBuffer, path)) {
        loadedFileName = ofFilePath::getFileName(path);
        playbackPos = 0;
        useFileInput = true;
        isPlaying = true;

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
        avs.audioIn(buffer);
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
    if (audioInitialized) {
        soundStream.stop();
        soundStream.close();
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
