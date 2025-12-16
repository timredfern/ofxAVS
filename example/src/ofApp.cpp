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
    
    // Demo effects setup for oscilloscope + dynamic movement
    effect_names = {"clear", "oscilloscope", "dynamic_movement", "transform"};
    current_effect = 0;
    auto_cycle_effects = false; // Disable auto cycling for manual control
    effect_cycle_time = 5.0f;
    last_effect_change = 0;
    
    // Set up oscilloscope with dynamic movement and feedback:
    // 1. Clear effect for feedback control (partial clear)
    // 2. Oscilloscope to draw audio waveform
    // 3. Dynamic movement effect for grid-based transformations
    setupOscilloscopeDynamicMovement();
    
    ofLogNotice() << "ofxAVS Example Started - Oscilloscope + Dynamic Movement";
    ofLogNotice() << "Effect Chain: Clear (feedback) + Oscilloscope + Dynamic Movement";
    ofLogNotice() << "Press keys 1-4 to change effects";
    ofLogNotice() << "Press SPACE to toggle auto-cycling";
    ofLogNotice() << "Press 'c' to clear effects";
    ofLogNotice() << "Press 'd' to reload oscilloscope + dynamic movement";
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
    ofDrawBitmapString("ofxAVS Example - Oscilloscope + Dynamic Movement", 20, 30);
    ofDrawBitmapString("Effect Chain: Clear (feedback) + Oscilloscope + Dynamic Movement", 20, 50);
    ofDrawBitmapString("FPS: " + ofToString(ofGetFrameRate(), 1), 20, 70);
    ofDrawBitmapString("Auto-cycle: " + std::string(auto_cycle_effects ? "ON" : "OFF"), 20, 90);
    ofDrawBitmapString("Beat Detected: " + std::string(visualizer.isBeat() ? "YES" : "NO"), 20, 110);
    
    ofDrawBitmapString("Controls:", 20, 150);
    ofDrawBitmapString("1-4: Select single effect", 20, 170);
    ofDrawBitmapString("d: Reload oscilloscope + dynamic movement", 20, 190);
    ofDrawBitmapString("c: Clear effects", 20, 210);
    ofDrawBitmapString("r: Add random effect", 20, 230);
    ofDrawBitmapString("SPACE: Toggle auto-cycle", 20, 250);
    
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
            
        case 'd':
            setupOscilloscopeDynamicMovement();
            ofLogNotice() << "Reloaded oscilloscope + dynamic movement";
            break;
    }
}

void ofApp::audioIn(ofSoundBuffer& buffer) {
    // Pass audio data to visualizer
    visualizer.audioReceived(buffer.getBuffer().data(), buffer.getNumFrames(), buffer.getNumChannels());
}

void ofApp::setupOscilloscopeDynamicMovement() {
    // Clear any existing effects
    visualizer.clearEffects();
    
    // Create classic AVS-style effect chain:
    // 1. Clear effect with feedback (partial clear creates trails)
    // 2. Oscilloscope draws audio waveform 
    // 3. Dynamic Movement effect applies grid-based transformations
    
    // Add clear effect configured for feedback (only clear first frame)
    visualizer.addClearEffect(true); // only_first=true enables feedback trails
    
    // Add oscilloscope to draw audio waveform
    visualizer.addEffect("oscilloscope");
    
    // Add dynamic movement effect with classic spiral transformation
    visualizer.addEffect("dynamic_movement");
    
    ofLogNotice() << "Oscilloscope + Dynamic Movement configured:";
    ofLogNotice() << "  1. Clear: only_first=true (feedback trails enabled)";
    ofLogNotice() << "  2. Oscilloscope: Audio waveform visualization";
    ofLogNotice() << "  3. Dynamic Movement: Grid-based transformations (spiral effect)";
    ofLogNotice() << "     - Default script: d=d*0.95; r=r+0.1 (classic spiral)";
    ofLogNotice() << "     - Grid-based evaluation for authentic AVS stepping artifacts";
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