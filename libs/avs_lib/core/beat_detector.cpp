// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License - see LICENSE file in repository root

#include "beat_detector.h"
#include <cmath>
#include <cstdlib>

namespace avs {

// UI Layout for beat detector settings - matches original AVS IDD_GCFG_BPM
const EffectUILayout BeatDetector::ui_layout_ = {
    {
        // Mode selection
        {
            .id = "mode",
            .text = "Detection mode",
            .type = ControlType::RADIO_GROUP,
            .x = 0, .y = 0, .w = 200, .h = 30,
            .radio_options = {
                {"Standard", 0, 0, 80, 12},
                {"Advanced", 0, 14, 80, 12}
            },
            .default_val = 1  // Advanced mode default
        },
        // Behavior options
        {
            .id = "sticky_beats",
            .text = "Sticky beats",
            .type = ControlType::CHECKBOX,
            .x = 0, .y = 35, .w = 80, .h = 12,
            .default_val = 1
        },
        {
            .id = "only_sticky",
            .text = "Only output when sticky",
            .type = ControlType::CHECKBOX,
            .x = 85, .y = 35, .w = 120, .h = 12,
            .default_val = 0
        },
        // Note: "On new song" relates to Winamp playlist integration - not implemented
        {
            .id = "on_new_song",
            .text = "On new song",
            .type = ControlType::RADIO_GROUP,
            .x = 0, .y = 52, .w = 200, .h = 30,
            .radio_options = {
                {"Reset", 0, 0, 60, 12},
                {"Adapt", 65, 0, 60, 12}
            },
            .default_val = 0  // Reset default
        },
        // Stick/Unstick buttons from original res.rc (Keep/Readapt)
        {
            .id = "stick",
            .text = "Keep",
            .type = ControlType::BUTTON,
            .x = 34, .y = 70, .w = 29, .h = 10,
            .default_val = 0
        },
        {
            .id = "unstick",
            .text = "Readapt",
            .type = ControlType::BUTTON,
            .x = 66, .y = 70, .w = 29, .h = 10,
            .default_val = 0
        },
        // Beat indicator sliders (visual only - animate on beats)
        {
            .id = "sensitivity_in",
            .text = "Input beats",
            .type = ControlType::SLIDER,
            .x = 0, .y = 87, .w = 200, .h = 16,
            .range = {0, 8},
            .default_val = 0
        },
        {
            .id = "sensitivity_out",
            .text = "Output beats",
            .type = ControlType::SLIDER,
            .x = 0, .y = 108, .w = 200, .h = 16,
            .range = {0, 8},
            .default_val = 0
        },
        // Beat adjustment buttons (dynamically added in original)
        {
            .id = "double_beat",
            .text = "2x",
            .type = ControlType::BUTTON,
            .x = 0, .y = 130, .w = 40, .h = 20,
            .default_val = 0
        },
        {
            .id = "half_beat",
            .text = "/2",
            .type = ControlType::BUTTON,
            .x = 45, .y = 130, .w = 40, .h = 20,
            .default_val = 0
        },
        {
            .id = "reset",
            .text = "Reset",
            .type = ControlType::BUTTON,
            .x = 90, .y = 130, .w = 50, .h = 20,
            .default_val = 0
        }
    }
};

BeatDetector::BeatDetector() {
    initParameters();
    reset();
}

void BeatDetector::initParameters() {
    for (const auto& control : ui_layout_.getControls()) {
        switch (control.type) {
            case ControlType::CHECKBOX:
                parameters_.add_parameter(std::make_shared<Parameter>(
                    control.id, ParameterType::BOOL, control.default_val != 0));
                break;
            case ControlType::SLIDER:
                parameters_.add_parameter(std::make_shared<Parameter>(
                    control.id, ParameterType::INT, control.default_val,
                    control.range.min, control.range.max));
                break;
            case ControlType::RADIO_GROUP:
                parameters_.add_parameter(std::make_shared<Parameter>(
                    control.id, ParameterType::INT, control.default_val,
                    0, static_cast<int>(control.radio_options.size()) - 1));
                break;
            case ControlType::BUTTON:
                parameters_.add_parameter(std::make_shared<Parameter>(
                    control.id, ParameterType::BOOL, false));
                break;
            default:
                break;
        }
    }
    // Hidden parameter for UI visibility control
    parameters_.add_parameter(std::make_shared<Parameter>(
        "is_sticked", ParameterType::BOOL, false));
}

void BeatDetector::reset() {
    beat_peak1_ = 0;
    beat_peak2_ = 0;
    beat_peak1_peak_ = 0;
    beat_cnt_ = 0;

    std::memset(history_, 0, sizeof(history_));
    std::memset(smoother_, 0, sizeof(smoother_));
    std::memset(half_discriminated_, 0, sizeof(half_discriminated_));

    last_tc_ = 0;
    prediction_last_tc_ = 0;
    avg_ = 0;

    bpm_ = 0;
    prediction_bpm_ = 0;
    confidence_ = 0;
    last_bpm_ = 0;

    sm_ptr_ = 0;
    hd_pos_ = 0;
    insertion_count_ = 0;
    half_count_ = 0;
    double_count_ = 0;

    sticked_ = false;
    sticky_confidence_count_ = 0;
    best_confidence_ = 0;

    frame_count_ = 0;

    in_slide_ = 0;
    out_slide_ = 0;
    in_inc_ = 1;
    out_inc_ = 1;
}

bool BeatDetector::process(const AudioData& visdata) {
    // Handle button presses
    if (parameters_.get_bool("double_beat", false)) {
        parameters_.set_bool("double_beat", false);
        doubleBeat();
    }
    if (parameters_.get_bool("half_beat", false)) {
        parameters_.set_bool("half_beat", false);
        halfBeat();
    }
    if (parameters_.get_bool("reset", false)) {
        parameters_.set_bool("reset", false);
        reset();
    }
    if (parameters_.get_bool("stick", false)) {
        parameters_.set_bool("stick", false);
        sticked_ = true;
        sticky_confidence_count_ = 0;
    }
    if (parameters_.get_bool("unstick", false)) {
        parameters_.set_bool("unstick", false);
        sticked_ = false;
        sticky_confidence_count_ = 0;
    }
    // Update sticked state for UI visibility control
    parameters_.set_bool("is_sticked", sticked_);

    // Increment frame counter (used as tick count proxy)
    // Assuming ~60fps, each frame is ~16.67ms
    frame_count_ += 17;  // Approximate ms per frame

    // Stage 1: Raw energy-based beat detection
    int raw_beat = detectRawBeat(visdata);

    // Animate input slider on raw beat
    if (raw_beat) {
        in_slide_ += in_inc_;
        if (in_slide_ <= 0 || in_slide_ >= 8) in_inc_ *= -1;
        parameters_.set_int("sensitivity_in", in_slide_);
    }

    // Stage 2: BPM tracking and refinement (only in Advanced mode)
    int mode = parameters_.get_int("mode", 1);
    int refined_beat;
    if (mode == 1) {
        // Advanced mode: use BPM prediction
        refined_beat = refineBeat(raw_beat);
    } else {
        // Standard mode: just pass through raw detection
        refined_beat = raw_beat;
    }

    // Check only_sticky option
    bool only_sticky = parameters_.get_bool("only_sticky", false);
    if (only_sticky && !sticked_) {
        return false;
    }

    // Animate output slider on output beat
    if (refined_beat) {
        out_slide_ += out_inc_;
        if (out_slide_ <= 0 || out_slide_ >= 8) out_inc_ *= -1;
        parameters_.set_int("sensitivity_out", out_slide_);
    }

    return refined_beat != 0;
}

int BeatDetector::detectRawBeat(const AudioData& visdata) {
    // Calculate energy per channel from waveform data
    // Note: ofxAVS puts waveform in visdata[0], spectrum in visdata[1]
    // (This differs from original AVS where [0]=spectrum, [1]=waveform)
    int lt[2] = {0, 0};

    for (int ch = 0; ch < 2; ch++) {
        for (int x = 0; x < 576; x++) {
            // Convert signed char to absolute value
            int r = visdata[0][ch][x];
            if (r < 0) r = -r;
            lt[ch] += r;
        }
    }

    // Use max of both channels
    lt[0] = std::max(lt[0], lt[1]);

    // Decay peak tracking (original: beat_peak1=(beat_peak1*125+beat_peak2*3)/128)
    beat_peak1_ = (beat_peak1_ * 125 + beat_peak2_ * 3) / 128;

    beat_cnt_++;

    int avs_beat = 0;

    // Beat threshold: 106.25% of peak AND above minimum (576*16 = 9216)
    int threshold = (beat_peak1_ * 34) / 32;
    int min_energy = 576 * 16;

    if (lt[0] >= threshold && lt[0] > min_energy) {
        if (beat_cnt_ > 0) {
            beat_cnt_ = 0;
            avs_beat = 1;
        }
        beat_peak1_ = (lt[0] + beat_peak1_peak_) / 2;
        beat_peak1_peak_ = lt[0];
    }
    else if (lt[0] > beat_peak2_) {
        beat_peak2_ = lt[0];
    }
    else {
        beat_peak2_ = (beat_peak2_ * 14) / 16;
    }

    return avs_beat;
}

int BeatDetector::refineBeat(int raw_beat) {
    bool accepted = false;
    bool predicted = false;

    uint32_t tc_now = frame_count_;

    // Record raw beat in history if present
    if (raw_beat) {
        accepted = historyStep(tc_now, true);
    }

    // Calculate BPM from history
    calcBpm();

    // Try to predict beats based on tempo
    if (bpm_ > 0 && tc_now > prediction_last_tc_ + (60000 / bpm_)) {
        predicted = true;
    }

    // Update prediction BPM
    bool sticky_enabled = parameters_.get_bool("sticky_beats", false);

    if ((accepted || predicted) && !sticked_) {
        if (confidence_ >= best_confidence_) {
            best_confidence_ = confidence_;
            prediction_bpm_ = bpm_;
        }
        if (confidence_ >= 50) {
            prediction_bpm_ = bpm_;
        }
    }

    if (!sticked_) {
        prediction_bpm_ = bpm_;
    }
    bpm_ = prediction_bpm_;

    // Handle sticky mode
    if (sticky_enabled && prediction_bpm_ > 0 && confidence_ >= 75) {
        sticky_confidence_count_++;
        if (sticky_confidence_count_ >= 4) {
            sticked_ = true;
        }
    } else {
        sticky_confidence_count_ = 0;
    }

    // Output logic
    if (predicted) {
        prediction_last_tc_ = tc_now;
        if (confidence_ > 25) {
            historyStep(tc_now, false);  // Record predicted beat
        }
        return 1;
    }

    if (accepted && prediction_bpm_ > 0) {
        // Resync: accept real beat if it's close to expected time
        uint32_t expected_interval = 60000 / prediction_bpm_;
        if (tc_now > prediction_last_tc_ + expected_interval * 7 / 10) {
            prediction_last_tc_ = tc_now;
            return 1;
        }
    }

    // If we have prediction, suppress raw beats that don't align
    if (prediction_bpm_ > 0) {
        return 0;
    }

    return raw_beat;
}

bool BeatDetector::historyStep(uint32_t tc, bool is_real) {
    if (!readyToLearn()) {
        // Still filling history
        insertHistory(tc, is_real, 0);
        last_tc_ = tc;
        return true;
    }

    uint32_t this_len = tc - last_tc_;

    // Reject beats that come too soon (less than half average - 20%)
    if (avg_ > 0 && this_len < avg_ / 2 - avg_ / 5) {
        return false;
    }

    // Check if this might be a subdivided beat (1/2, 1/3, 1/4 of expected interval)
    for (int off_i = 2; off_i < 8; off_i++) {
        if (avg_ > 0 && std::abs(static_cast<int>(avg_ / off_i - this_len)) < static_cast<int>(avg_ / off_i * 0.2)) {
            half_discriminated_[hd_pos_++] = 1;
            hd_pos_ %= HIST_SIZE;
            return false;
        }
    }

    // Accept this beat
    half_discriminated_[hd_pos_++] = 0;
    hd_pos_ %= HIST_SIZE;

    last_tc_ = tc;
    insertHistory(tc, is_real, 0);
    return true;
}

void BeatDetector::insertHistory(uint32_t tc, bool is_real, int pos) {
    if (pos >= HIST_SIZE) return;

    if (insertion_count_ < HIST_SIZE * 2) {
        insertion_count_++;
    }

    // Shift history and insert at position
    for (int i = HIST_SIZE - 1; i > pos; i--) {
        history_[i] = history_[i - 1];
    }
    history_[pos].tick_count = tc;
    history_[pos].is_real = is_real;
}

bool BeatDetector::readyToLearn() const {
    for (int i = 0; i < HIST_SIZE; i++) {
        if (history_[i].tick_count == 0) return false;
    }
    return true;
}

bool BeatDetector::readyToGuess() const {
    return insertion_count_ >= HIST_SIZE * 2;
}

void BeatDetector::calcBpm() {
    if (!readyToLearn()) return;

    // Calculate average interval between beats
    uint32_t total_tc = 0;
    for (int i = 0; i < HIST_SIZE - 1; i++) {
        total_tc += history_[i].tick_count - history_[i + 1].tick_count;
    }
    avg_ = total_tc / (HIST_SIZE - 1);

    if (avg_ == 0) return;

    // Count real vs predicted beats
    int real_count = 0;
    for (int i = 0; i < HIST_SIZE; i++) {
        if (history_[i].is_real) real_count++;
    }

    // Calculate confidence part 1: ratio of real beats
    float r_conf = std::min(static_cast<float>(real_count) / HIST_SIZE * 2.0f, 1.0f);

    // Calculate confidence part 2: consistency of intervals
    double sum_sq = 0;
    uint32_t max_interval = 0;
    for (int i = 0; i < HIST_SIZE - 1; i++) {
        uint32_t interval = history_[i].tick_count - history_[i + 1].tick_count;
        max_interval = std::max(max_interval, interval);
        sum_sq += static_cast<double>(interval) * interval;
    }

    float std_dev = std::sqrt(sum_sq / (HIST_SIZE - 1) - static_cast<double>(avg_) * avg_);
    float et_conf = max_interval > 0 ? 1.0f - (std_dev / max_interval) : 0.0f;

    // Combined confidence
    confidence_ = std::max(0, static_cast<int>(((r_conf * et_conf) * 100.0f) - 50) * 2);

    if (readyToGuess()) {
        bpm_ = 60000 / avg_;

        if (bpm_ != last_bpm_) {
            newBpm(bpm_);
            last_bpm_ = bpm_;
        }

        bpm_ = getSmoothedBpm();

        // Check if we should double/half the beat
        int hd_count = 0;
        for (int i = 0; i < HIST_SIZE; i++) {
            if (half_discriminated_[i]) hd_count++;
        }

        if (hd_count >= HIST_SIZE / 2 && bpm_ * 2 < MAX_BPM) {
            doubleBeat();
        }

        if (bpm_ < MIN_BPM) {
            if (++double_count_ > 4) doubleBeat();
        } else {
            double_count_ = 0;
        }

        if (bpm_ > MAX_BPM) {
            if (++half_count_ > 4) halfBeat();
        } else {
            half_count_ = 0;
        }
    }
}

void BeatDetector::newBpm(int bpm) {
    smoother_[sm_ptr_++] = bpm;
    sm_ptr_ %= HIST_SIZE;
}

int BeatDetector::getSmoothedBpm() const {
    int sum = 0;
    int count = 0;
    for (int i = 0; i < HIST_SIZE; i++) {
        if (smoother_[i] > 0) {
            sum += smoother_[i];
            count++;
        }
    }
    return count > 0 ? sum / count : 0;
}

void BeatDetector::doubleBeat() {
    if (sticked_ && bpm_ > MIN_BPM) return;

    // Store intervals first, then apply (must not modify in place)
    uint32_t intervals[HIST_SIZE];
    for (int i = 0; i < HIST_SIZE - 1; i++) {
        intervals[i] = history_[i].tick_count - history_[i + 1].tick_count;
    }
    for (int i = 1; i < HIST_SIZE; i++) {
        history_[i].tick_count = history_[i - 1].tick_count - intervals[i - 1] / 2;
    }

    avg_ /= 2;
    bpm_ *= 2;
    double_count_ = 0;
    std::memset(smoother_, 0, sizeof(smoother_));
    std::memset(half_discriminated_, 0, sizeof(half_discriminated_));
}

void BeatDetector::halfBeat() {
    if (sticked_ && bpm_ < MIN_BPM) return;

    // Store intervals first, then apply (must not modify in place)
    uint32_t intervals[HIST_SIZE];
    for (int i = 0; i < HIST_SIZE - 1; i++) {
        intervals[i] = history_[i].tick_count - history_[i + 1].tick_count;
    }
    for (int i = 1; i < HIST_SIZE; i++) {
        history_[i].tick_count = history_[i - 1].tick_count - intervals[i - 1] * 2;
    }

    avg_ *= 2;
    bpm_ /= 2;
    half_count_ = 0;
    std::memset(smoother_, 0, sizeof(smoother_));
    std::memset(half_discriminated_, 0, sizeof(half_discriminated_));
}

} // namespace avs
