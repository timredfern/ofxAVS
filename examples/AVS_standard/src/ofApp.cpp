// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetWindowTitle("AVS_standard");
    ofSetWindowShape(1450, 640);  // Consistent gutters around output

    gui.setup();
    avs.setup();

    // Load previous AVS session (or start with empty chain)
    if (!avs.loadSession()) {
        avs.addEffect("Brightness");
        avs.addEffect("Oscilloscope");
    }

    // Setup audio (loads saved settings automatically)
    avs.loadAudioSettings();
    avs.setupAudio();
}

//--------------------------------------------------------------
void ofApp::update(){
    avs.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(0);
    avs.draw(830, 20, 600, 600);

    gui.begin();
    avs.drawUI();
    drawAudioPanel();
    gui.end();
}

//--------------------------------------------------------------
void ofApp::drawAudioPanel() {
    ImGui::SetNextWindowPos(ImVec2(10, 640 - 135));
    ImGui::SetNextWindowSize(ImVec2(660, 75));

    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::Begin("Audio", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    avs.drawAudioUI();

    ImGui::End();
    ImGui::PopStyleColor(2);
}

//--------------------------------------------------------------
void ofApp::exit(){
    avs.saveSession();
    avs.saveAudioSettings();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if (key == ' ') {
        avs.togglePlayback();
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
            avs.loadSoundFile(path);
        }
    }
}

void ofApp::gotMessage(ofMessage msg){}
