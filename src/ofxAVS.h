// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "ofMain.h"
#include "ofxImGui.h"
#include "ofxFft.h"
#include "core/renderer.h"
#include "core/plugin_manager.h"
#include "core/effect_base.h"
#include "core/effect_container.h"
#include "core/beat_detector.h"
#include "core/configurable.h"
#include "effects/effect_list_root.h"
#include <vector>
#include <unordered_set>
#include <memory>

// FFT mode selection:
// Define AVS_ENHANCED_FFT for modern processing (2048 samples, smoothing, dB scale)
// Undefine for original Winamp behavior (512 samples, log table, no smoothing)
//#define AVS_ENHANCED_FFT

// Available effect info
struct AvailableEffectInfo {
    std::string name;
    std::string display_name;
    std::string category;
    std::string description;
};

class ofxAVS {
public:
    ofxAVS();
    ~ofxAVS();

    // Setup and audio
    void setup();
    void update();
    void audioIn(ofSoundBuffer& buffer);  // Process audio with FFT
    void setAudioData(const avs::AudioData& data);  // Direct access if needed

    // Rendering
    void draw(int x, int y, int width, int height);
    void drawUI();

    // Effect chain management
    void addEffect(const std::string& effectName, avs::EffectContainer* parent = nullptr);
    void removeEffect(avs::EffectBase* effect);
    void duplicateEffect(avs::EffectBase* effect);
    void moveEffectUp(avs::EffectBase* effect);
    void moveEffectDown(avs::EffectBase* effect);

    // Preset loading/saving (supports .avs binary and .json formats)
    bool loadPreset(const std::string& path);
    bool savePreset(const std::string& path);
    const std::string& getLastError() const;

    // UI panel management - now uses Configurable interface for effects and settings
    void setSelected(avs::Configurable* item) { selected_ = item; }
    avs::Configurable* getSelected() const { return selected_; }

    // Beat detector access
    avs::BeatDetector* getBeatDetector() { return beat_detector_.get(); }

    // Effect access
    const std::vector<AvailableEffectInfo>& getAvailableEffects() const { return available_effects; }

private:
    // Core AVS components
    std::unique_ptr<avs::DefaultRenderer> renderer;
    ofTexture texture;
    ofPixels pixels;
    int width, height;
    avs::AudioData current_audio_data;

    // FFT for spectrum analysis
    ofxFft* fft;
#ifdef AVS_ENHANCED_FFT
    static const int FFT_SIZE = 2048;  // Higher resolution for enhanced mode
    float smoothedSpectrum[576];       // Temporal smoothing buffer
#else
    static const int FFT_SIZE = 512;   // Original Winamp FFT size
    unsigned char logTable[256];       // AVS log compression table
#endif

    // Beat detector
    std::unique_ptr<avs::BeatDetector> beat_detector_;

    // Effect UI state
    std::vector<AvailableEffectInfo> available_effects;
    avs::Configurable* selected_ = nullptr;  // Can be effect, beat detector, or other settings

    // Track collapsed containers for tree view
    std::unordered_set<avs::EffectContainer*> collapsed_containers_;

    // Last error message for preset operations
    std::string last_error_;

    // Current preset name (for display)
    std::string current_preset_name_ = "AVS";

    // UI panel dimensions
    int chain_panel_width = 280;
    int chain_panel_height = 480;
    int parameters_panel_width = 500;  // Fits 480-wide child + padding
    int parameters_panel_height = 480;

    // Internal methods
    void initializeAvailableEffects();

    // UI rendering methods
    void drawChainPanel();
    void drawParametersPanel();

    // Tree view helpers
    void drawEffectTree(avs::EffectContainer* container, int depth = 0);
    avs::EffectContainer* findParentContainer(avs::EffectBase* effect, avs::EffectContainer* searchIn = nullptr);

    // Popup menu helpers
    void drawAddEffectMenu(avs::EffectContainer* targetContainer);
    void drawAddEffectMenuInsertAfter(avs::EffectContainer* container, int afterIndex);
    void drawEffectContextMenu(avs::EffectBase* effect);

    // Effect insertion helper
    void insertEffect(const std::string& effectName, avs::EffectContainer* container, size_t index);

    // Keyboard navigation helpers
    void handleEffectListKeyboard();
    avs::EffectBase* getNextVisibleEffect(avs::EffectBase* current);
    avs::EffectBase* getPrevVisibleEffect(avs::EffectBase* current);
    void buildVisibleEffectList(avs::EffectContainer* container, std::vector<avs::EffectBase*>& list);
};