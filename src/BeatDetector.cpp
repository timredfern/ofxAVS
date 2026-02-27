// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "BeatDetector.h"
#include <cmath>
#include <cstdlib>

// UI Layout for beat detector settings - matches original AVS IDD_GCFG_BPM (137x137)
const avs::EffectUILayout BeatDetector::ui_layout_ = {
    {
        // Outer groupbox
        {
            .id = "settings_group",
            .text = "Beat detection settings",
            .type = avs::ControlType::GROUPBOX,
            .x = 0, .y = 0, .w = 137, .h = 130
        },
        // Mode selection (Standard/Advanced radio)
        {
            .id = "mode",
            .text = "",
            .type = avs::ControlType::RADIO_GROUP,
            .x = 4, .y = 11, .w = 49, .h = 20,
            .default_val = 1,  // Advanced mode default
            .radio_options = {
                {"Standard", 0, 0, 45, 10},
                {"Advanced", 0, 10, 49, 10}
            }
        },
        // Beat indicator sliders (visual feedback, disabled)
        {
            .id = "sensitivity_in",
            .text = "",
            .type = avs::ControlType::SLIDER,
            .x = 55, .y = 11, .w = 79, .h = 11,
            .range = {0, 8},
            .default_val = 0,
            .enabled = false
        },
        {
            .id = "sensitivity_out",
            .text = "",
            .type = avs::ControlType::SLIDER,
            .x = 55, .y = 21, .w = 79, .h = 11,
            .range = {0, 8},
            .default_val = 0,
            .enabled = false
        },
        // Advanced status groupbox
        {
            .id = "status_group",
            .text = "Advanced status",
            .type = avs::ControlType::GROUPBOX,
            .x = 4, .y = 33, .w = 128, .h = 92
        },
        // BPM display
        {
            .id = "bpm_label",
            .text = "Current BPM :",
            .type = avs::ControlType::LABEL,
            .x = 8, .y = 43, .w = 45, .h = 8
        },
        {
            .id = "bpm_value",
            .text = "",
            .type = avs::ControlType::LABEL,
            .x = 55, .y = 43, .w = 70, .h = 8
        },
        // Confidence display
        {
            .id = "confidence_label",
            .text = "Confidence :",
            .type = avs::ControlType::LABEL,
            .x = 8, .y = 52, .w = 41, .h = 8
        },
        {
            .id = "confidence_value",
            .text = "",
            .type = avs::ControlType::LABEL,
            .x = 55, .y = 52, .w = 71, .h = 8
        },
        // Auto-keep checkbox (default OFF to avoid locking to wrong BPM early)
        {
            .id = "sticky_beats",
            .text = "Auto-keep",
            .type = avs::ControlType::CHECKBOX,
            .x = 8, .y = 61, .w = 49, .h = 10,
            .default_val = 0
        },
        // Predict only if BPM found
        {
            .id = "only_sticky",
            .text = "Predict only if bpm has been found",
            .type = avs::ControlType::CHECKBOX,
            .x = 8, .y = 70, .w = 121, .h = 10,
            .default_val = 0
        },
        // Reset/Keep/Readapt buttons
        {
            .id = "reset",
            .text = "Reset",
            .type = avs::ControlType::BUTTON,
            .x = 7, .y = 80, .w = 23, .h = 10,
            .default_val = 0
        },
        {
            .id = "stick",
            .text = "Keep",
            .type = avs::ControlType::BUTTON,
            .x = 34, .y = 80, .w = 29, .h = 10,
            .default_val = 0
        },
        {
            .id = "unstick",
            .text = "Readapt",
            .type = avs::ControlType::BUTTON,
            .x = 66, .y = 80, .w = 29, .h = 10,
            .default_val = 0
        },
        // New Song groupbox
        {
            .id = "newsong_group",
            .text = "New Song",
            .type = avs::ControlType::GROUPBOX,
            .x = 8, .y = 90, .w = 121, .h = 32
        },
        // New song behavior radio
        {
            .id = "on_new_song",
            .text = "",
            .type = avs::ControlType::RADIO_GROUP,
            .x = 13, .y = 99, .w = 106, .h = 20,
            .default_val = 0,
            .radio_options = {
                {"Adapt from known BPM", 0, 0, 91, 10},
                {"Restart learning from scratch", 0, 10, 106, 10}
            }
        },
        // Beat adjustment buttons (not in original dialog, but useful)
        {
            .id = "double_beat",
            .text = "2x",
            .type = avs::ControlType::BUTTON,
            .x = 100, .y = 80, .w = 15, .h = 10,
            .default_val = 0
        },
        {
            .id = "half_beat",
            .text = "/2",
            .type = avs::ControlType::BUTTON,
            .x = 118, .y = 80, .w = 15, .h = 10,
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
        // Helper to get int from variant (default 0)
        auto get_int = [&]() -> int {
            if (auto* v = std::get_if<int>(&control.default_val)) return *v;
            if (auto* v = std::get_if<bool>(&control.default_val)) return *v ? 1 : 0;
            return 0;
        };
        // Helper to get bool from variant (default false)
        auto get_bool = [&]() -> bool {
            if (auto* v = std::get_if<bool>(&control.default_val)) return *v;
            if (auto* v = std::get_if<int>(&control.default_val)) return *v != 0;
            return false;
        };

        switch (control.type) {
            case avs::ControlType::CHECKBOX:
                parameters_.add_parameter(std::make_shared<avs::Parameter>(
                    control.id, avs::ParameterType::BOOL, get_bool()));
                break;
            case avs::ControlType::SLIDER:
                parameters_.add_parameter(std::make_shared<avs::Parameter>(
                    control.id, avs::ParameterType::INT, get_int(),
                    control.range.min, control.range.max));
                break;
            case avs::ControlType::RADIO_GROUP: {
                std::vector<std::string> labels;
                for (const auto& opt : control.radio_options) {
                    labels.push_back(opt.label);
                }
                parameters_.add_parameter(std::make_shared<avs::Parameter>(
                    control.id, avs::ParameterType::ENUM, get_int(), labels));
                break;
            }
            case avs::ControlType::BUTTON:
                parameters_.add_parameter(std::make_shared<avs::Parameter>(
                    control.id, avs::ParameterType::BOOL, false));
                break;
            default:
                break;
        }
    }
    // Hidden parameter for UI visibility control
    parameters_.add_parameter(std::make_shared<avs::Parameter>(
        "is_sticked", avs::ParameterType::BOOL, false));
    // Dynamic display values (updated by process())
    parameters_.add_parameter(std::make_shared<avs::Parameter>(
        "bpm_value", avs::ParameterType::STRING, std::string("")));
    parameters_.add_parameter(std::make_shared<avs::Parameter>(
        "confidence_value", avs::ParameterType::STRING, std::string("")));
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

    // Modern mode state
    std::memset(prev_spectrum_left_, 0, sizeof(prev_spectrum_left_));
    std::memset(prev_spectrum_right_, 0, sizeof(prev_spectrum_right_));
    std::memset(flux_history_, 0, sizeof(flux_history_));
    flux_history_pos_ = 0;
    flux_history_count_ = 0;
}

void BeatDetector::handleButtonPresses() {
    if (parameters_.get_bool("double_beat")) {
        parameters_.set_bool("double_beat", false);
        doubleBeat();
    }
    if (parameters_.get_bool("half_beat")) {
        parameters_.set_bool("half_beat", false);
        halfBeat();
    }
    if (parameters_.get_bool("reset")) {
        parameters_.set_bool("reset", false);
        reset();
    }
    if (parameters_.get_bool("stick")) {
        parameters_.set_bool("stick", false);
        sticked_ = true;
        sticky_confidence_count_ = 0;
    }
    if (parameters_.get_bool("unstick")) {
        parameters_.set_bool("unstick", false);
        sticked_ = false;
        sticky_confidence_count_ = 0;
    }
}

void BeatDetector::updateDisplayValues() {
    parameters_.set_bool("is_sticked", sticked_);

    if (bpm_ > 0) {
        parameters_.set_string("bpm_value", std::to_string(bpm_));
    } else {
        parameters_.set_string("bpm_value", "");
    }
    parameters_.set_string("confidence_value", std::to_string(confidence_) + "%");
}

bool BeatDetector::process(const avs::AudioData& visdata) {
    handleButtonPresses();
    updateDisplayValues();

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
    int mode = parameters_.get_int("mode");
    int refined_beat;
    if (mode == 1) {
        // Advanced mode: use BPM prediction
        refined_beat = refineBeat(raw_beat);
    } else {
        // Standard mode: just pass through raw detection
        refined_beat = raw_beat;
    }

    // Check only_sticky option
    bool only_sticky = parameters_.get_bool("only_sticky");
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

// ============================================================================
// MODERN MODE: Spectral flux onset detection
// ============================================================================

float BeatDetector::calcSpectralFlux(const float* amplitude, float* prevSpectrum, int binSize) {
    float flux = 0.0f;

    // Focus on bass frequencies (bins 1-8 roughly 86-690Hz at 44100/512)
    // This is where kick drums live
    int bassEnd = std::min(binSize, 16);

    for (int i = 1; i < bassEnd; i++) {
        float diff = amplitude[i] - prevSpectrum[i];
        // Half-wave rectification: only count increases (onsets, not releases)
        if (diff > 0) {
            flux += diff;
        }
        prevSpectrum[i] = amplitude[i];
    }

    return flux;
}

float BeatDetector::calcAdaptiveThreshold() {
    if (flux_history_count_ < 4) {
        return 0.0f;  // Not enough history yet
    }

    // Calculate mean
    float sum = 0.0f;
    for (int i = 0; i < flux_history_count_; i++) {
        sum += flux_history_[i];
    }
    float mean = sum / flux_history_count_;

    // Calculate standard deviation
    float sum_sq = 0.0f;
    for (int i = 0; i < flux_history_count_; i++) {
        float diff = flux_history_[i] - mean;
        sum_sq += diff * diff;
    }
    float stddev = std::sqrt(sum_sq / flux_history_count_);

    // Threshold = mean + k * stddev (k=1.5 is typical)
    return mean + 1.5f * stddev;
}

bool BeatDetector::processModern(const float* amplitudeLeft, const float* amplitudeRight, int binSize) {
    handleButtonPresses();
    updateDisplayValues();

    frame_count_ += 17;

    // Calculate spectral flux for both channels
    float fluxLeft = calcSpectralFlux(amplitudeLeft, prev_spectrum_left_, binSize);
    float fluxRight = calcSpectralFlux(amplitudeRight, prev_spectrum_right_, binSize);

    // Combine channels (use max for better transient detection)
    float flux = std::max(fluxLeft, fluxRight);

    // Store in history for adaptive thresholding
    flux_history_[flux_history_pos_] = flux;
    flux_history_pos_ = (flux_history_pos_ + 1) % FLUX_HISTORY_SIZE;
    if (flux_history_count_ < FLUX_HISTORY_SIZE) {
        flux_history_count_++;
    }

    // Check if flux exceeds adaptive threshold
    float threshold = calcAdaptiveThreshold();
    int raw_beat = (flux > threshold && threshold > 0) ? 1 : 0;

    // Animate input slider
    if (raw_beat) {
        in_slide_ += in_inc_;
        if (in_slide_ <= 0 || in_slide_ >= 8) in_inc_ *= -1;
        parameters_.set_int("sensitivity_in", in_slide_);
    }

    // Use existing BPM tracking infrastructure
    int mode = parameters_.get_int("mode");
    int refined_beat;
    if (mode == 1) {
        refined_beat = refineBeat(raw_beat);
    } else {
        refined_beat = raw_beat;
    }

    bool only_sticky = parameters_.get_bool("only_sticky");
    if (only_sticky && !sticked_) {
        return false;
    }

    if (refined_beat) {
        out_slide_ += out_inc_;
        if (out_slide_ <= 0 || out_slide_ >= 8) out_inc_ *= -1;
        parameters_.set_int("sensitivity_out", out_slide_);
    }

    return refined_beat != 0;
}

// ============================================================================
// CLASSIC MODE: Energy-based detection (original AVS algorithm)
// ============================================================================

int BeatDetector::detectRawBeat(const avs::AudioData& visdata) {
    // Calculate energy per channel from waveform data
    int lt[2] = {0, 0};

    for (int ch = 0; ch < 2; ch++) {
        for (int x = 0; x < avs::MIN_AUDIO_SAMPLES; x++) {
            // Convert signed char to absolute value
            int r = visdata[avs::AUDIO_WAVEFORM][ch][x];
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
    int min_energy = avs::MIN_AUDIO_SAMPLES * 16;

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

// ============================================================================
// BPM TRACKING (shared by classic and modern modes)
// ============================================================================

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
    bool sticky_enabled = parameters_.get_bool("sticky_beats");

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
