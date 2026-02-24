// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// MIDI File (SMF) Parser Implementation
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#include "MidiFile.h"
#include <fstream>
#include <algorithm>
#include <cstring>

namespace avs {

uint32_t MidiFile::readU32BE(const uint8_t* data) {
    return (static_cast<uint32_t>(data[0]) << 24) |
           (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8) |
           static_cast<uint32_t>(data[3]);
}

uint16_t MidiFile::readU16BE(const uint8_t* data) {
    return (static_cast<uint16_t>(data[0]) << 8) |
           static_cast<uint16_t>(data[1]);
}

uint32_t MidiFile::readVarLen(const uint8_t* data, size_t& offset, size_t max) {
    uint32_t value = 0;
    uint8_t byte;

    do {
        if (offset >= max) return 0;
        byte = data[offset++];
        value = (value << 7) | (byte & 0x7F);
    } while (byte & 0x80);

    return value;
}

bool MidiFile::load(const std::string& path) {
    events_.clear();
    duration_ = 0.0;
    tempo_ = 120.0;
    loaded_ = false;
    error_.clear();

    // Read entire file
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        error_ = "Could not open file: " + path;
        return false;
    }

    size_t fileSize = file.tellg();
    file.seekg(0);

    std::vector<uint8_t> data(fileSize);
    file.read(reinterpret_cast<char*>(data.data()), fileSize);
    file.close();

    if (fileSize < 14) {
        error_ = "File too small to be a valid MIDI file";
        return false;
    }

    // Check MThd header
    if (std::memcmp(data.data(), "MThd", 4) != 0) {
        error_ = "Invalid MIDI file header";
        return false;
    }

    uint32_t headerLen = readU32BE(&data[4]);
    if (headerLen < 6) {
        error_ = "Invalid header length";
        return false;
    }

    uint16_t format = readU16BE(&data[8]);
    uint16_t numTracks = readU16BE(&data[10]);
    uint16_t division = readU16BE(&data[12]);

    // Division: ticks per quarter note (if positive)
    // or SMPTE frames (if negative - not supported here)
    if (division & 0x8000) {
        error_ = "SMPTE time format not supported";
        return false;
    }

    int ticksPerQuarter = division;
    double usPerTick = 500000.0 / ticksPerQuarter;  // Default 120 BPM

    // Parse tracks
    size_t offset = 8 + headerLen;

    for (int track = 0; track < numTracks && offset < fileSize; track++) {
        // Check MTrk header
        if (offset + 8 > fileSize) break;
        if (std::memcmp(&data[offset], "MTrk", 4) != 0) {
            error_ = "Invalid track header";
            return false;
        }

        uint32_t trackLen = readU32BE(&data[offset + 4]);
        offset += 8;

        size_t trackEnd = offset + trackLen;
        if (trackEnd > fileSize) trackEnd = fileSize;

        uint32_t absoluteTick = 0;
        uint8_t runningStatus = 0;

        while (offset < trackEnd) {
            // Read delta time
            uint32_t delta = readVarLen(data.data(), offset, trackEnd);
            absoluteTick += delta;

            if (offset >= trackEnd) break;

            uint8_t status = data[offset];

            // Handle running status
            if (status < 0x80) {
                status = runningStatus;
            } else {
                offset++;
                if (status < 0xF0) {
                    runningStatus = status;
                }
            }

            // Calculate time in seconds
            double timeInSeconds = absoluteTick * usPerTick / 1000000.0;

            // Handle different message types
            if (status >= 0x80 && status <= 0xEF) {
                // Channel messages
                uint8_t type = status & 0xF0;

                if (type == 0xC0 || type == 0xD0) {
                    // Program change, channel aftertouch: 1 data byte
                    if (offset >= trackEnd) break;
                    uint8_t d1 = data[offset++];

                    MidiFileEvent event;
                    event.time = timeInSeconds;
                    event.status = status;
                    event.data1 = d1;
                    event.data2 = 0;
                    events_.push_back(event);
                } else {
                    // Note on/off, CC, pitch bend, etc: 2 data bytes
                    if (offset + 1 >= trackEnd) break;
                    uint8_t d1 = data[offset++];
                    uint8_t d2 = data[offset++];

                    // Handle note on with velocity 0 as note off
                    if (type == MidiFileEvent::NOTE_ON && d2 == 0) {
                        status = MidiFileEvent::NOTE_OFF | (status & 0x0F);
                    }

                    MidiFileEvent event;
                    event.time = timeInSeconds;
                    event.status = status;
                    event.data1 = d1;
                    event.data2 = d2;
                    events_.push_back(event);
                }
            } else if (status == 0xFF) {
                // Meta event
                if (offset + 1 >= trackEnd) break;
                uint8_t metaType = data[offset++];
                uint32_t len = readVarLen(data.data(), offset, trackEnd);

                if (metaType == 0x51 && len == 3 && offset + 3 <= trackEnd) {
                    // Tempo change
                    uint32_t usPerQuarter = (static_cast<uint32_t>(data[offset]) << 16) |
                                           (static_cast<uint32_t>(data[offset + 1]) << 8) |
                                           static_cast<uint32_t>(data[offset + 2]);
                    usPerTick = static_cast<double>(usPerQuarter) / ticksPerQuarter;
                    tempo_ = 60000000.0 / usPerQuarter;
                }

                offset += len;
            } else if (status == 0xF0 || status == 0xF7) {
                // SysEx
                uint32_t len = readVarLen(data.data(), offset, trackEnd);
                offset += len;
            } else if (status >= 0xF1 && status <= 0xFE) {
                // System common/realtime - skip
                // Most have 0-2 data bytes
                if (status == 0xF1 || status == 0xF3) offset++;
                else if (status == 0xF2) offset += 2;
            }
        }

        offset = trackEnd;
    }

    // Sort events by time
    std::sort(events_.begin(), events_.end(),
              [](const MidiFileEvent& a, const MidiFileEvent& b) {
                  return a.time < b.time;
              });

    // Calculate duration
    if (!events_.empty()) {
        duration_ = events_.back().time;
    }

    loaded_ = true;
    return true;
}

} // namespace avs
