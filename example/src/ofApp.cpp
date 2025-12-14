#include "ofApp.h"

void ofApp::setup() {
    ofSetWindowTitle("ofxAVS Example");
    ofSetFrameRate(60);
    
    // Audio setup
    sample_rate = 44100;
    buffer_size = 512;
    num_input_channels = 1; // Most built-in mics are mono
    
    soundStream.printDeviceList();
    
    ofSoundStreamSettings settings;
    settings.setInListener(this);
    settings.sampleRate = sample_rate;
    settings.numOutputChannels = 0;
    settings.numInputChannels = num_input_channels;
    settings.bufferSize = buffer_size;
    soundStream.setup(settings);
    
    // Visualizer setup
    visualizer.setup(512, 512);
    
    // Demo effects setup for layered visualization
    effect_names = {"clear", "oscilloscope", "blur", "transform"};
    current_effect = 0;
    auto_cycle_effects = false; // Disable auto cycling for manual control
    effect_cycle_time = 5.0f;
    last_effect_change = 0;
    
    // Set up layered effect chain for classic AVS feedback look:
    // 1. Transform effect with slight scaling (creates feedback/trails)
    // 2. Oscilloscope to draw new audio waveform
    setupLayeredEffects();
    
    ofLogNotice() << "ofxAVS Example Started - Layered Effects Mode";
    ofLogNotice() << "Effect Chain: Transform (1.05x scale) + Oscilloscope";
    ofLogNotice() << "Press keys 1-4 to change effects";
    ofLogNotice() << "Press SPACE to toggle auto-cycling";
    ofLogNotice() << "Press 'c' to clear effects";
    ofLogNotice() << "Press 'l' to reload layered effects";
}

void ofApp::update() {
    // Auto-cycle effects
    if (auto_cycle_effects && ofGetElapsedTimef() - last_effect_change > effect_cycle_time) {
        current_effect = (current_effect + 1) % effect_names.size();
        visualizer.clearEffects();
        visualizer.addEffect(effect_names[current_effect]);
        last_effect_change = ofGetElapsedTimef();
        ofLogNotice() << "Auto-switched to effect: " << effect_names[current_effect];
    }
    
    visualizer.update();
}

void ofApp::draw() {
    ofBackground(0);
    
    // Draw visualizer centered
    float viz_size = 400;
    float x = (ofGetWidth() - viz_size) / 2;
    float y = (ofGetHeight() - viz_size) / 2;
    
    visualizer.draw(x, y, viz_size, viz_size);
    
    // Draw UI
    ofSetColor(255);
    ofDrawBitmapString("ofxAVS Example - Layered Effects Demo", 20, 30);
    ofDrawBitmapString("Effect Chain: Transform (1.05x scale) + Oscilloscope", 20, 50);
    ofDrawBitmapString("Auto-cycle: " + std::string(auto_cycle_effects ? "ON" : "OFF"), 20, 70);
    ofDrawBitmapString("Beat Detected: " + std::string(visualizer.isBeat() ? "YES" : "NO"), 20, 90);
    
    ofDrawBitmapString("Controls:", 20, 130);
    ofDrawBitmapString("1-4: Select single effect", 20, 150);
    ofDrawBitmapString("l: Reload layered effects", 20, 170);
    ofDrawBitmapString("c: Clear effects", 20, 190);
    ofDrawBitmapString("r: Add random effect", 20, 210);
    ofDrawBitmapString("SPACE: Toggle auto-cycle", 20, 230);
    
    // Draw audio info
    ofDrawBitmapString("Audio Input: " + std::to_string(num_input_channels) + " channels", 20, ofGetHeight() - 60);
    ofDrawBitmapString("Sample Rate: " + std::to_string(sample_rate) + " Hz", 20, ofGetHeight() - 40);
    ofDrawBitmapString("Buffer Size: " + std::to_string(buffer_size), 20, ofGetHeight() - 20);
}

void ofApp::keyPressed(int key) {
    switch(key) {
        case '1':
        case '2':  
        case '3':
        case '4':
            current_effect = key - '1';
            if (current_effect < effect_names.size()) {
                visualizer.clearEffects();
                visualizer.addEffect(effect_names[current_effect]);
                auto_cycle_effects = false;
                ofLogNotice() << "Switched to effect: " << effect_names[current_effect];
            }
            break;
            
        case ' ':
            auto_cycle_effects = !auto_cycle_effects;
            last_effect_change = ofGetElapsedTimef();
            ofLogNotice() << "Auto-cycle: " << (auto_cycle_effects ? "ON" : "OFF");
            break;
            
        case 'c':
            visualizer.clearEffects();
            ofLogNotice() << "Cleared all effects";
            break;
            
        case 'r':
            visualizer.clearEffects();
            current_effect = ofRandom(effect_names.size());
            visualizer.addEffect(effect_names[current_effect]);
            ofLogNotice() << "Random effect: " << effect_names[current_effect];
            break;
            
        case 'l':
            setupLayeredEffects();
            ofLogNotice() << "Reloaded layered effects";
            break;
    }
}

void ofApp::audioIn(ofSoundBuffer& buffer) {
    // Pass audio data to visualizer
    visualizer.audioReceived(buffer.getBuffer().data(), buffer.getNumFrames(), buffer.getNumChannels());
}

void ofApp::setupLayeredEffects() {
    // Clear any existing effects
    visualizer.clearEffects();
    
    // Create the classic AVS feedback effect:
    // 1. Clear effect that only clears on first frame (enables feedback)
    // 2. Transform effect with slight scaling creates trails/feedback
    // 3. Oscilloscope draws fresh audio waveform on top
    
    // Add clear effect configured for feedback (only clear first frame)
    visualizer.addClearEffect(true); // only_first=true enables feedback
    
    // Add transform effect with scaling to create feedback trails
    // x = x * 1.05, y = y * 1.05 (slight zoom/scale)
    visualizer.addTransformEffect("x * 1.05", "y * 1.05");
    
    // Add oscilloscope to draw audio waveform on top
    visualizer.addEffect("oscilloscope");
    
    ofLogNotice() << "Layered effects configured:";
    ofLogNotice() << "  1. Clear: only_first=true (feedback enabled)";
    ofLogNotice() << "  2. Transform: x = x * 1.05, y = y * 1.05 (feedback scaling)";
    ofLogNotice() << "  3. Oscilloscope: Audio waveform visualization";
}

void ofApp::keyReleased(int key) {}
void ofApp::mouseMoved(int x, int y) {}
void ofApp::mouseDragged(int x, int y, int button) {}
void ofApp::mousePressed(int x, int y, int button) {}
void ofApp::mouseReleased(int x, int y, int button) {}
void ofApp::mouseEntered(int x, int y) {}
void ofApp::mouseExited(int x, int y) {}
void ofApp::windowResized(int w, int h) {}
void ofApp::dragEvent(ofDragInfo dragInfo) {}
void ofApp::gotMessage(ofMessage msg) {}