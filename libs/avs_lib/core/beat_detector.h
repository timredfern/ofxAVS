// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License - see LICENSE file in repository root

#pragma once

#include "configurable.h"
#include "effect_base.h"  // For AudioData typedef
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace avs {

// Beat detector implementing original AVS algorithm from bpm.cpp
//
// IMPLEMENTATION STATUS:
// - [x] Stage 1: Energy-based onset detection (main.cpp algorithm)
// - [x] Stage 2: BPM tracking and beat prediction (bpm.cpp refineBeat)
// - [x] UI: Mode selection (Standard/Advanced)
// - [x] UI: Sticky beats, Only sticky options
// - [x] UI: Reset/Adapt on new song
// - [x] UI: Input/Output beat indicator sliders (visual only, animate on beats)
// - [x] UI: 2x, /2, Reset buttons
// - [ ] UI: BPM and Confidence read-only displays (shown in tree label instead)
// - [x] UI: Stick/Unstick toggle buttons
// - [ ] Song change detection (requires integration with audio source)
//
// The original AVS used GetTickCount() for timing; we use a frame counter
// approximating 60fps (~17ms per frame).
class BeatDetector : public Configurable {
public:
    BeatDetector();

    // Process audio data and return whether this frame is a beat
    bool process(const AudioData& visdata);

    // Reset all state
    void reset();

    // Current detected BPM (0 if not yet determined)
    int getBpm() const { return prediction_bpm_; }

    // Confidence in current BPM (0-100)
    int getConfidence() const { return confidence_; }

    // Whether we're locked to a tempo
    bool isSticky() const { return sticked_; }

    // Manual beat adjustment
    void doubleBeat();
    void halfBeat();

    // Configurable interface
    std::string get_display_name() const override { return "Beat detector"; }
    const EffectUILayout& get_ui_layout() const override { return ui_layout_; }
    ParameterGroup& parameters() override { return parameters_; }
    const ParameterGroup& parameters() const override { return parameters_; }

private:
    ParameterGroup parameters_;
    static const EffectUILayout ui_layout_;

    // Stage 1: Energy detection state
    int beat_peak1_ = 0;
    int beat_peak2_ = 0;
    int beat_peak1_peak_ = 0;
    int beat_cnt_ = 0;

    // Stage 2: BPM tracking state
    static constexpr int HIST_SIZE = 8;
    static constexpr int MIN_BPM = 30;
    static constexpr int MAX_BPM = 300;

    struct BeatEntry {
        uint32_t tick_count = 0;
        bool is_real = false;  // true = detected, false = predicted
    };

    BeatEntry history_[HIST_SIZE];
    int smoother_[HIST_SIZE];
    int half_discriminated_[HIST_SIZE];

    uint32_t last_tc_ = 0;
    uint32_t prediction_last_tc_ = 0;
    uint32_t avg_ = 0;

    int bpm_ = 0;
    int prediction_bpm_ = 0;
    int confidence_ = 0;
    int last_bpm_ = 0;

    int sm_ptr_ = 0;
    int hd_pos_ = 0;
    int insertion_count_ = 0;
    int half_count_ = 0;
    int double_count_ = 0;

    bool sticked_ = 0;
    int sticky_confidence_count_ = 0;
    int best_confidence_ = 0;

    // Frame counter for timing (in lieu of GetTickCount)
    uint32_t frame_count_ = 0;

    // Beat-synced slider animation (visual indicator)
    int in_slide_ = 0;
    int out_slide_ = 0;
    int in_inc_ = 1;
    int out_inc_ = 1;

    // Internal methods
    int detectRawBeat(const AudioData& visdata);
    int refineBeat(int raw_beat);
    bool historyStep(uint32_t tc, bool is_real);
    void calcBpm();
    void insertHistory(uint32_t tc, bool is_real, int pos);
    bool readyToLearn() const;
    bool readyToGuess() const;
    void newBpm(int bpm);
    int getSmoothedBpm() const;

    void initParameters();
};

} // namespace avs
