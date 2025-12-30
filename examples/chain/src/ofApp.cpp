// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetWindowTitle("AVS Chain Example");
    ofSetWindowShape(1500, 640);

    audioInitialized = false;

    gui.setup();
    avs.setup();

    // Setup audio input (microphone)
    vector<ofSoundDevice> devices = soundStream.getDeviceList();

    ofSoundStreamSettings settings;
    settings.sampleRate = 44100;
    settings.numInputChannels = 1;
    settings.numOutputChannels = 0;
    settings.bufferSize = 576;
    settings.setInListener(this);

    for (auto& device : devices) {
        if (device.inputChannels > 0) {
            settings.setInDevice(device);
            try {
                soundStream.setup(settings);
                audioInitialized = true;
                ofLogNotice() << "Audio setup with: " << device.name;
                break;
            } catch (...) {
                ofLogWarning() << "Failed: " << device.name;
            }
        }
    }

    if (!audioInitialized) {
        ofLogWarning() << "No audio input available";
    }

    // Configure sound player for looping
    soundPlayer.setLoop(true);
}

//--------------------------------------------------------------
void ofApp::update(){
    // Update sound system (required for ofSoundPlayer)
    ofSoundUpdate();

    // Note: File playback plays audio but doesn't provide visualization data.
    // ofSoundGetSpectrum() exists but isn't implemented on macOS.
    // To visualize file audio, would need to decode and stream through ofSoundStream.

    avs.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(20);
    avs.draw(880, 20, 600, 600);

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
            soundPlayer.stop();
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
        if (soundPlayer.isLoaded()) {
            // Play/Stop button
            if (ImGui::Button(soundPlayer.isPlaying() ? "Stop" : "Play", ImVec2(60, 0))) {
                if (soundPlayer.isPlaying()) {
                    soundPlayer.stop();
                } else {
                    soundPlayer.play();
                }
            }
            ImGui::SameLine();

            // Show file name and progress
            ImGui::Text("%s", loadedFileName.c_str());
            ImGui::SameLine();

            float progress = soundPlayer.getPosition();
            ImGui::ProgressBar(progress, ImVec2(150, 0));

            // Click on progress bar to seek
            if (ImGui::IsItemClicked()) {
                ImVec2 mousePos = ImGui::GetMousePos();
                ImVec2 itemPos = ImGui::GetItemRectMin();
                float seekPos = (mousePos.x - itemPos.x) / 150.0f;
                seekPos = ofClamp(seekPos, 0.0f, 1.0f);
                soundPlayer.setPosition(seekPos);
            }
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Drop audio file here (.wav, .mp3, .ogg, .aiff)");
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

    if (soundPlayer.load(path)) {
        loadedFileName = ofFilePath::getFileName(path);
        useFileInput = true;
        soundPlayer.play();
        ofLogNotice() << "Loaded: " << loadedFileName;
    } else {
        ofLogError() << "Failed to load sound file: " << path;
    }
}

//--------------------------------------------------------------
void ofApp::audioIn(ofSoundBuffer& buffer) {
    // Only use mic input if not using file
    // Note: When using file playback, spectrum comes from ofSoundGetSpectrum()
    // which is called internally by OF. Waveform visualization won't work
    // with file playback - only spectrum-based effects will respond.
    if (!useFileInput) {
        avs.audioIn(buffer);
    }
}

//--------------------------------------------------------------
void ofApp::exit(){
    soundPlayer.stop();
    if (audioInitialized) {
        soundStream.stop();
        soundStream.close();
        audioInitialized = false;
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if (key == ' ') {
        if (useFileInput && soundPlayer.isLoaded()) {
            if (soundPlayer.isPlaying()) {
                soundPlayer.stop();
            } else {
                soundPlayer.play();
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
        std::string ext = ofToLower(ofFilePath::getFileExt(path));

        // Check for supported audio formats
        if (ext == "wav" || ext == "mp3" || ext == "ogg" || ext == "aiff" || ext == "aif" || ext == "flac") {
            loadSoundFile(path);
        } else {
            ofLogWarning() << "Unsupported audio format: " << ext;
        }
    }
}

void ofApp::gotMessage(ofMessage msg){}
