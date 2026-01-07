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
        ofLogNotice() << "  " << device.deviceID << ": " << device.name
                      << " (in:" << device.inputChannels << " out:" << device.outputChannels << ")";
    }

    ofSoundStreamSettings settings;
    settings.sampleRate = 44100;
    settings.bufferSize = 576;
    settings.setInListener(this);
    settings.setOutListener(this);

    // Try to find a device with both input and output
    for (auto& device : devices) {
        if (device.inputChannels > 0 && device.outputChannels >= 2) {
            settings.numInputChannels = 1;
            settings.numOutputChannels = 2;
            settings.setInDevice(device);
            settings.setOutDevice(device);
            try {
                soundStream.setup(settings);
                audioInitialized = true;
                ofLogNotice() << "Audio setup (in+out) with: " << device.name;
                break;
            } catch (...) {
                ofLogWarning() << "Failed: " << device.name;
            }
        }
    }

    // Fallback: separate input and output devices
    if (!audioInitialized) {
        ofSoundDevice* inputDevice = nullptr;
        ofSoundDevice* outputDevice = nullptr;

        for (auto& device : devices) {
            if (!inputDevice && device.inputChannels > 0) inputDevice = &device;
            if (!outputDevice && device.outputChannels >= 2) outputDevice = &device;
        }

        if (outputDevice) {
            settings.numInputChannels = inputDevice ? 1 : 0;
            settings.numOutputChannels = 2;
            if (inputDevice) settings.setInDevice(*inputDevice);
            settings.setOutDevice(*outputDevice);
            try {
                soundStream.setup(settings);
                audioInitialized = true;
                ofLogNotice() << "Audio setup with separate devices - out: " << outputDevice->name;
                if (inputDevice) ofLogNotice() << "  in: " << inputDevice->name;
            } catch (...) {
                ofLogWarning() << "Failed to setup audio";
            }
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
    // Use mic input when not playing file
    if (!useFileInput || !isPlaying) {
        avs.audioIn(buffer);
    }
}

//--------------------------------------------------------------
void ofApp::audioOut(ofSoundBuffer& buffer) {
    // Only output audio if playing a file
    if (!useFileInput || !isPlaying || audioFileBuffer.getNumFrames() == 0) {
        buffer.set(0);
        return;
    }

    size_t numFrames = buffer.getNumFrames();
    int outChannels = buffer.getNumChannels();
    int fileChannels = audioFileBuffer.getNumChannels();

    for (size_t i = 0; i < numFrames; i++) {
        if (playbackPos >= audioFileBuffer.getNumFrames()) {
            // Loop back to start
            playbackPos = 0;
        }

        float left = audioFileBuffer.getSample(playbackPos, 0);
        float right = (fileChannels >= 2) ? audioFileBuffer.getSample(playbackPos, 1) : left;

        // Write to output buffer
        if (outChannels >= 2) {
            buffer.getSample(i, 0) = left;
            buffer.getSample(i, 1) = right;
        } else if (outChannels == 1) {
            buffer.getSample(i, 0) = (left + right) * 0.5f;
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
