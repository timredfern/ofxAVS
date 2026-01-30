// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "ofApp.h"
#include "ofAppGLFWWindow.h"
#include "AVSui.h"

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

    // Setup audio (loads saved settings automatically)
    avs.loadAudioSettings();
    avs.setupAudio();
}

//--------------------------------------------------------------
void ofApp::update() {
    cleanupInvalidParamWindows();
}

//--------------------------------------------------------------
void ofApp::draw() {
    ofBackground(40);

    chain_gui_.begin();

    float margin = 5;
    float gap = 5;
    float audioHeight = 130;
    float w = ofGetWidth();
    float h = ofGetHeight();
    float panelWidth = w - margin * 2;

    // Chain panel - no title bar, consistent margin
    float chainHeight = h - audioHeight - margin * 2 - gap;
    ImGui::SetNextWindowPos(ImVec2(margin, margin));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, chainHeight));
    ImGui::Begin("##chain", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
    avs.drawChainPanel();
    ImGui::End();

    // Audio panel - with title
    ImGui::SetNextWindowPos(ImVec2(margin, margin * 2 + chainHeight + gap));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, audioHeight));
    ImGui::Begin("Audio", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse);
    avs.drawAudioUI();
    ImGui::End();

    chain_gui_.end();
    chain_gui_.draw();
}

//--------------------------------------------------------------
void ofApp::drawOutput(ofEventArgs& args) {
    // Output window - AVS update and draw in same context
    avs.update();

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
void ofApp::openParamWindow(avs::Configurable* configurable) {
    // Check if already open
    for (auto& pw : param_windows_) {
        if (pw->configurable == configurable) {
            return;
        }
    }

    // Create new window (no context sharing needed - just ImGui UI)
    ofGLFWWindowSettings settings;
    settings.setSize(500, 460);
    settings.resizable = false;
    settings.title = configurable->get_display_name();

    auto window = ofCreateWindow(settings);

    auto info = std::make_unique<ParamWindowInfo>();
    info->window = window;
    info->gui = std::make_unique<ofxImGui::Gui>();
    info->configurable = configurable;
    info->effect_id = reinterpret_cast<uintptr_t>(configurable);

    ofAddListener(window->events().draw, this, &ofApp::drawParamWindows);
    ofAddListener(window->events().exit, this, &ofApp::onParamWindowClose);

    param_windows_.push_back(std::move(info));
}

//--------------------------------------------------------------
void ofApp::onParamWindowClose(ofEventArgs& args) {
    auto current_window = ofGetCurrentWindow();
    for (auto& info : param_windows_) {
        if (info->window == current_window && !info->marked_for_removal) {
            info->marked_for_removal = true;
            ofRemoveListener(info->window->events().draw, this, &ofApp::drawParamWindows);
            ofRemoveListener(info->window->events().exit, this, &ofApp::onParamWindowClose);
            return;
        }
    }
}

//--------------------------------------------------------------
void ofApp::closeParamWindow(avs::Configurable* configurable) {
    for (auto it = param_windows_.begin(); it != param_windows_.end(); ) {
        if ((*it)->configurable == configurable) {
            ofRemoveListener((*it)->window->events().draw, this, &ofApp::drawParamWindows);
            ofRemoveListener((*it)->window->events().exit, this, &ofApp::onParamWindowClose);
            it = param_windows_.erase(it);
        } else {
            ++it;
        }
    }
}

//--------------------------------------------------------------
void ofApp::drawParamWindows(ofEventArgs& args) {
    auto current_window = ofGetCurrentWindow();
    for (auto& info : param_windows_) {
        if (info->window == current_window) {
            if (info->marked_for_removal) {
                return;
            }
            drawParamWindow(*info);
            return;
        }
    }
}

//--------------------------------------------------------------
void ofApp::drawParamWindow(ParamWindowInfo& info) {
    if (!isEffectValid(info.effect_id)) {
        return;
    }

    if (info.needs_setup) {
        info.gui->setup(nullptr, false, ImGuiConfigFlags_None, false, false);
        info.needs_setup = false;
    }

    ofBackground(40);

    info.gui->begin();
    avs_ui::renderParamWindowContent(info.configurable, ofGetWidth(), ofGetHeight());
    info.gui->end();
    info.gui->draw();
}

//--------------------------------------------------------------
void ofApp::cleanupInvalidParamWindows() {
    // First pass: erase windows that were marked (by exit event or effect deletion)
    param_windows_.erase(
        std::remove_if(param_windows_.begin(), param_windows_.end(),
            [](const std::unique_ptr<ParamWindowInfo>& info) {
                return info->marked_for_removal;
            }),
        param_windows_.end());

    // Second pass: mark windows for removal if effect was deleted
    for (auto& info : param_windows_) {
        if (info->marked_for_removal) continue;

        if (!isEffectValid(info->effect_id)) {
            info->marked_for_removal = true;
            ofRemoveListener(info->window->events().draw, this, &ofApp::drawParamWindows);
            ofRemoveListener(info->window->events().exit, this, &ofApp::onParamWindowClose);
        }
    }
}

//--------------------------------------------------------------
bool ofApp::isEffectValid(uintptr_t id) {
    if (reinterpret_cast<uintptr_t>(avs.getRoot()) == id) {
        return true;
    }
    if (reinterpret_cast<uintptr_t>(avs.getBeatDetector()) == id) {
        return true;
    }
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
void ofApp::exit() {
    avs.saveSession();
    avs.saveAudioSettings();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    if (key == ' ') {
        avs.togglePlayback();
    }
    if (key == 'p' || key == 'P') {
        avs.toggleProfiling();
    }
    if (key == 'm' || key == 'M') {
        avs.toggleMidiDebug();
    }
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {
    if (dragInfo.files.empty()) return;

    std::string path = dragInfo.files[0];
    std::string ext = ofFilePath::getFileExt(path);

    // Preset files
    if (ext == "avs" || ext == "avsp") {
        if (avs.loadPreset(path)) {
            ofLogNotice() << "Loaded preset: " << ofFilePath::getFileName(path);
        } else {
            ofLogError() << "Failed to load preset: " << avs.getLastError();
        }
    }
    // MIDI files
    else if (ext == "mid" || ext == "midi") {
        avs.setMidiDebug(true);
        avs.loadMidiFile(path);
    }
    // Catalogue files (audio + MIDI synced)
    else if (ext == "avsc") {
        avs.setMidiDebug(true);
        avs.loadCatalogue(path);
    }
    // Audio files
    else {
        avs.loadSoundFile(path);
    }
}
