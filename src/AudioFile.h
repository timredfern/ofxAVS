// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

// AudioFile - miniaudio-based audio file decoder
// Cross-platform audio decoding (macOS, Linux, Windows)

#pragma once

#include "ofMain.h"

class AudioFile {
public:
    // Load audio file into ofSoundBuffer
    static bool load(ofSoundBuffer& buffer, const std::string& filePath, size_t framesToRead = 0);
};
