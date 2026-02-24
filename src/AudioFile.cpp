// ofxAVS - OpenFrameworks addon for Advanced Visualization Studio
// Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

// AudioFile - miniaudio-based audio file decoder

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "AudioFile.h"

bool AudioFile::load(ofSoundBuffer& buffer, const std::string& filePath, size_t framesToRead) {
    ma_decoder decoder;
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, 0, 0);  // f32, native channels/rate

    ma_result result = ma_decoder_init_file(filePath.c_str(), &config, &decoder);
    if (result != MA_SUCCESS) {
        ofLogError("AudioFile") << "Failed to open file: " << filePath;
        return false;
    }

    // Get file info
    ma_uint64 totalFrames;
    ma_decoder_get_length_in_pcm_frames(&decoder, &totalFrames);

    ma_uint32 channels = decoder.outputChannels;
    ma_uint32 sampleRate = decoder.outputSampleRate;

    // Determine how many frames to read
    ma_uint64 framesToDecode = totalFrames;
    if (framesToRead > 0 && framesToRead < totalFrames) {
        framesToDecode = framesToRead;
    }

    // Allocate buffer and read
    std::vector<float> samples(framesToDecode * channels);
    ma_uint64 framesRead;
    result = ma_decoder_read_pcm_frames(&decoder, samples.data(), framesToDecode, &framesRead);

    ma_decoder_uninit(&decoder);

    if (result != MA_SUCCESS && result != MA_AT_END) {
        ofLogError("AudioFile") << "Failed to decode file: " << filePath;
        return false;
    }

    // Resize to actual frames read
    samples.resize(framesRead * channels);

    // Copy to ofSoundBuffer
    buffer.copyFrom(samples.data(), framesRead, channels, sampleRate);

    ofLogNotice("AudioFile") << "Loaded: " << filePath
                             << " (" << framesRead << " frames, "
                             << channels << " ch, "
                             << sampleRate << " Hz)";

    return true;
}
