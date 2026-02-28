// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Live MIDI Input Handler
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "ofxMidi.h"
#include "core/event_bus.h"
#include <string>
#include <vector>
#include <deque>
#include <mutex>

namespace avs {

// Debug log entry for MIDI messages
struct MidiLogEntry {
    double timestamp;
    std::string message;
};

// Live MIDI input handler
// Receives MIDI from hardware devices and routes to EventBus
class MidiInput {
public:
    static constexpr int CHANNEL_OMNI = 0;  // Receive from all channels

    MidiInput();
    ~MidiInput();

    // Device management
    std::vector<std::string> getDeviceList();
    bool openDevice(int index);
    bool openDevice(const std::string& name);
    void closeDevice();
    bool isOpen();
    std::string getDeviceName();

    // Channel filter (0 = Omni, 1-16 = specific channel)
    void setChannel(int channel);
    int getChannel() const { return channel_; }

    // Call from update() to process incoming messages
    void update();

    // Debug log
    void setDebugEnabled(bool enabled) { debug_enabled_ = enabled; }
    bool isDebugEnabled() const { return debug_enabled_; }
    const std::deque<MidiLogEntry>& getDebugLog() const { return debug_log_; }
    void clearDebugLog();
    void setAutoScroll(bool enabled) { auto_scroll_ = enabled; }
    bool isAutoScroll() const { return auto_scroll_; }

    // Max entries in debug log
    static constexpr size_t MAX_LOG_ENTRIES = 200;

private:
    void processMessage(const ofxMidiMessage& msg);
    void addLogEntry(const std::string& text);
    std::string formatMessage(const ofxMidiMessage& msg) const;

    ofxMidiIn midi_in_;
    int channel_ = CHANNEL_OMNI;
    bool debug_enabled_ = false;
    bool auto_scroll_ = true;

    // Debug log (thread-safe access)
    std::deque<MidiLogEntry> debug_log_;
    mutable std::mutex log_mutex_;
};

} // namespace avs
