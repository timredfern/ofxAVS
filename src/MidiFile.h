// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// MIDI File (SMF) Parser
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace avs {

// MIDI event with absolute time in seconds
struct MidiFileEvent {
    double time;        // Time in seconds from start
    uint8_t status;     // MIDI status byte (includes channel for channel messages)
    uint8_t data1;      // First data byte (note number, CC number, etc.)
    uint8_t data2;      // Second data byte (velocity, CC value, etc.)

    // Extract channel (0-15) from status byte
    int channel() const { return status & 0x0F; }

    // Extract message type from status byte
    int type() const { return status & 0xF0; }

    // Common MIDI status types
    static constexpr uint8_t NOTE_OFF = 0x80;
    static constexpr uint8_t NOTE_ON = 0x90;
    static constexpr uint8_t POLY_AFTERTOUCH = 0xA0;
    static constexpr uint8_t CONTROL_CHANGE = 0xB0;
    static constexpr uint8_t PROGRAM_CHANGE = 0xC0;
    static constexpr uint8_t CHANNEL_AFTERTOUCH = 0xD0;
    static constexpr uint8_t PITCH_BEND = 0xE0;
};

// Simple MIDI file parser
class MidiFile {
public:
    MidiFile() = default;

    // Load a Standard MIDI File
    bool load(const std::string& path);

    // Get all events sorted by time
    const std::vector<MidiFileEvent>& getEvents() const { return events_; }

    // Get duration in seconds
    double getDuration() const { return duration_; }

    // Get tempo (BPM) - returns first tempo found, or 120 if none
    double getTempo() const { return tempo_; }

    // Check if file is loaded
    bool isLoaded() const { return loaded_; }

    // Get error message if load failed
    const std::string& getError() const { return error_; }

private:
    std::vector<MidiFileEvent> events_;
    double duration_ = 0.0;
    double tempo_ = 120.0;
    bool loaded_ = false;
    std::string error_;

    // Parsing helpers
    uint32_t readVarLen(const uint8_t* data, size_t& offset, size_t max);
    uint32_t readU32BE(const uint8_t* data);
    uint16_t readU16BE(const uint8_t* data);
};

} // namespace avs
