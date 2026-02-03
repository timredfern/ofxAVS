// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "ofMain.h"
#include "ofxImGui.h"
#include "ofxFft.h"
#include "ofxAudioDecoder.h"
#include "core/renderer.h"
#include "core/plugin_manager.h"
#include "core/effect_base.h"
#include "core/effect_container.h"
#include "core/beat_detector.h"
#include "core/configurable.h"
#include "core/event_bus.h"
#include "effects/effect_list_root.h"
#include "MidiFile.h"
#include <vector>
#include <unordered_set>
#include <memory>
#include <functional>

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

class ofxAVS : public ofBaseSoundInput, public ofBaseSoundOutput {
public:
    ofxAVS();
    ~ofxAVS();

    // Setup and audio
    void setup();
    void update();
    void setupAudio();  // Initialize audio devices
    void audioIn(ofSoundBuffer& buffer) override;   // Process audio with FFT
    void audioOut(ofSoundBuffer& buffer) override;  // File playback output
    void setAudioData(const avs::AudioData& data);  // Direct access if needed

    // Audio file loading
    void loadSoundFile(const std::string& path, bool autoPlay = true);
    void togglePlayback();  // Play/pause
    bool isPlaying() const { return audio_is_playing_; }

    // MIDI file loading and catalogue
    void loadMidiFile(const std::string& path);
    bool loadCatalogue(const std::string& jsonPath);  // Load JSON with audio+MIDI paths
    void setMidiDebug(bool enabled) { midi_debug_ = enabled; }
    void toggleMidiDebug();

    // Audio settings persistence
    void loadAudioSettings();
    void saveAudioSettings();

    // Audio UI - draws responsive audio controls panel
    void drawAudioUI();

    // Session persistence (call explicitly - not automatic)
    bool loadSession();  // Load from data/session.json
    bool saveSession();  // Save to data/session.json

    // Root effect list access
    avs::EffectListRoot* getRoot() { return renderer->root(); }

    // Rendering
    void draw(int x, int y, int width, int height);
    void resize(int width, int height);  // Resize internal framebuffer
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

    // Single effect save/load with file dialogs
    void saveEffectDialog(avs::EffectBase* effect);
    void loadEffectDialog(avs::EffectContainer* target, int insertIndex = -1);

    // Global preset save/load with file dialogs
    void savePresetDialog();
    void loadPresetDialog();

    // UI panel management - now uses Configurable interface for effects and settings
    void setSelected(avs::Configurable* item) { selected_ = item; }
    avs::Configurable* getSelected() const { return selected_; }

    // Beat detector access
    avs::BeatDetector* getBeatDetector() { return beat_detector_.get(); }

    // Effect access
    const std::vector<AvailableEffectInfo>& getAvailableEffects() const { return available_effects; }

    // Profiling - toggle timing display with 'P' key
    void toggleProfiling() { show_profiling_ = !show_profiling_; }
    bool isProfilingEnabled() const { return show_profiling_; }

    // Callback for opening parameter windows (for multi-window apps)
    using OpenParamsCallback = std::function<void(avs::Configurable*)>;
    void setOpenParamsCallback(OpenParamsCallback cb) { on_open_params_callback_ = cb; }

    // Draw only the chain panel content (for multi-window apps that handle params separately)
    // Call this inside your own ImGui window - it draws content only, no window creation
    void drawChainPanel() { drawChainPanelContent(); }

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

    // Profiling
    bool show_profiling_ = false;

    // Callback for "Params" menu item
    OpenParamsCallback on_open_params_callback_;

    // Internal methods
    void initializeAvailableEffects();

    // UI rendering methods
    void drawChainPanelInternal();  // Creates window + content (for standard example)
    void drawChainPanelContent();   // Content only (for multiwindow - caller creates window)
    void drawParametersPanel();

    // Tree view helpers
    void drawEffectTree(avs::EffectContainer* container, int depth = 0);
    avs::EffectContainer* findParentContainer(avs::EffectBase* effect, avs::EffectContainer* searchIn = nullptr);
    void cleanupPointersToEffect(avs::EffectBase* effect);

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

    // Audio management
    ofSoundStream sound_stream_;
    bool audio_initialized_ = false;
    bool audio_has_input_ = false;

    // Audio device selection
    std::vector<ofSoundDevice> audio_devices_;
    std::vector<int> audio_input_indices_;    // Indices into audio_devices_ for input-capable devices
    std::vector<int> audio_output_indices_;   // Indices into audio_devices_ for output-capable devices
    int audio_selected_input_ = -1;           // Index into audio_input_indices_ (-1 = none)
    int audio_selected_output_ = -1;          // Index into audio_output_indices_
    std::string audio_input_device_name_;     // For persistence
    std::string audio_output_device_name_;    // For persistence

    // Sound file playback
    ofSoundBuffer audio_file_buffer_;
    size_t audio_playback_pos_ = 0;
    bool audio_use_file_ = false;
    bool audio_is_playing_ = false;
    std::string audio_loaded_filename_;
    std::string audio_loaded_filepath_;       // Full path for persistence
    float audio_mic_gain_ = 1.0f;             // Microphone gain (1x to 100x)

    void restartAudio();

    // MIDI file playback
    avs::MidiFile midi_file_;
    size_t midi_event_index_ = 0;            // Next event to process
    bool midi_debug_ = false;                // Print MIDI events to console
    std::string midi_loaded_filepath_;       // Currently loaded MIDI file

    void updateMidiPlayback();               // Called from update() to process MIDI events
};