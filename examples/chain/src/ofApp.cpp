#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetWindowTitle("AVS Chain Example");
    ofSetWindowShape(1420, 640);
    
    // Initialize audio flag
    audioInitialized = false;
    
    // Setup ImGui
    gui.setup();
    
    // Setup AVS
    avs.setup();
    
    // Setup audio input - try to find a working device
    vector<ofSoundDevice> devices = soundStream.getDeviceList();
    
    ofSoundStreamSettings settings;
    settings.sampleRate = 44100;
    settings.numInputChannels = 1;  // Start with mono
    settings.numOutputChannels = 0;
    settings.bufferSize = 256;
    settings.setInListener(this);
    
    // Try to find a working input device
    bool audioSetup = false;
    for (auto& device : devices) {
        if (device.inputChannels > 0) {
            settings.setInDevice(device);
            try {
                soundStream.setup(settings);
                audioSetup = true;
                audioInitialized = true;
                ofLogNotice() << "Audio setup successful with device: " << device.name;
                break;
            } catch (...) {
                ofLogWarning() << "Failed to setup audio with device: " << device.name;
            }
        }
    }
    
    if (!audioSetup) {
        ofLogWarning() << "No audio input available - running without audio";
    }
}

//--------------------------------------------------------------
void ofApp::update(){
    avs.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(20);
    
    // Draw visualization directly
    avs.draw(800, 20, 600, 600);
    
    // Draw UI panels only
    gui.begin();
    avs.drawUI();
    gui.end();
}

//--------------------------------------------------------------
void ofApp::audioIn(ofSoundBuffer& buffer) {
    processAudioData(buffer);
}

//--------------------------------------------------------------
void ofApp::processAudioData(ofSoundBuffer& buffer) {
    // Convert OF audio buffer to AVS format
    avs::AudioData audioData;
    memset(&audioData, 0, sizeof(avs::AudioData));
    
    int buffer_samples = static_cast<int>(buffer.size()) / buffer.getNumChannels();
    int samples = std::min(buffer_samples, 576);
    
    for (int i = 0; i < samples; i++) {
        if (buffer.getNumChannels() >= 1) {
            audioData[0][0][i] = static_cast<char>(buffer[i * buffer.getNumChannels()] * 127.0f);
        }
        if (buffer.getNumChannels() >= 2) {
            audioData[0][1][i] = static_cast<char>(buffer[i * buffer.getNumChannels() + 1] * 127.0f);
        } else {
            audioData[0][1][i] = audioData[0][0][i]; // Mono to stereo
        }
    }
    
    // Simple spectrum placeholder (copy waveform data)
    for (int i = 0; i < 576; i++) {
        audioData[1][0][i] = audioData[0][0][i];
        audioData[1][1][i] = audioData[0][1][i];
    }
    
    avs.setAudioData(audioData);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::exit(){
    if (audioInitialized) {
        soundStream.stop();
        soundStream.close();
        audioInitialized = false;
    }
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}