// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace avs {

/**
 * BinaryReader - Helper for reading binary data from legacy AVS presets
 *
 * Handles little-endian integer reading and both old (fixed-size) and
 * new (length-prefixed) string formats used in AVS binary configs.
 */
class BinaryReader {
public:
    BinaryReader(const std::vector<uint8_t>& data) : data_(data), pos_(0) {}
    BinaryReader(const uint8_t* data, size_t len) : data_(data, data + len), pos_(0) {}

    bool eof() const { return pos_ >= data_.size(); }
    size_t pos() const { return pos_; }
    size_t remaining() const { return data_.size() > pos_ ? data_.size() - pos_ : 0; }
    const uint8_t* ptr() const { return pos_ < data_.size() ? &data_[pos_] : nullptr; }

    uint8_t read_u8() {
        if (pos_ >= data_.size()) return 0;
        return data_[pos_++];
    }

    uint32_t read_u32() {
        if (pos_ + 4 > data_.size()) return 0;
        uint32_t val = data_[pos_] |
                       (data_[pos_ + 1] << 8) |
                       (data_[pos_ + 2] << 16) |
                       (data_[pos_ + 3] << 24);
        pos_ += 4;
        return val;
    }

    int32_t read_i32() {
        return static_cast<int32_t>(read_u32());
    }

    // Read a fixed-size string (null-terminated within buffer)
    std::string read_string_fixed(size_t len) {
        if (pos_ + len > data_.size()) return "";
        std::string s(reinterpret_cast<const char*>(&data_[pos_]), len);
        pos_ += len;
        // Trim at null terminator
        size_t null_pos = s.find('\0');
        if (null_pos != std::string::npos) {
            s.resize(null_pos);
        }
        return s;
    }

    // Read a length-prefixed string (4-byte length + data)
    std::string read_length_prefixed_string() {
        uint32_t len = read_u32();
        if (len == 0) return "";
        if (pos_ + len > data_.size()) return "";
        std::string s(reinterpret_cast<const char*>(&data_[pos_]), len);
        pos_ += len;
        // Trim at null terminator (length may include null)
        size_t null_pos = s.find('\0');
        if (null_pos != std::string::npos) {
            s.resize(null_pos);
        }
        return s;
    }

    void skip(size_t bytes) {
        pos_ = std::min(pos_ + bytes, data_.size());
    }

    // Check if next bytes match a pattern
    bool peek_match(const char* pattern, size_t len) const {
        if (pos_ + len > data_.size()) return false;
        return std::memcmp(&data_[pos_], pattern, len) == 0;
    }

    // Convert BGR color (0x00BBGGRR) to ARGB (0xAARRGGBB)
    static uint32_t bgr_to_argb(uint32_t bgr) {
        uint32_t r = bgr & 0xFF;
        uint32_t g = (bgr >> 8) & 0xFF;
        uint32_t b = (bgr >> 16) & 0xFF;
        return 0xFF000000 | (r << 16) | (g << 8) | b;
    }

private:
    std::vector<uint8_t> data_;
    size_t pos_;
};

} // namespace avs
