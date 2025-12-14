#include "ofxAVS.h"
#include "avs_lib/core/plugin_manager.h"
#include <algorithm>
#include <cstring>

namespace avs {

ofxAVS::ofxAVS() 
    : renderer(nullptr)
    , beat_threshold(0.3f)
    , is_beat(false)
    , last_audio_peak(0.0f)
    , width(512)
    , height(512)
    , initialized(false)
{
    // Initialize audio data structure
    memset(&vis_data, 0, sizeof(AudioData));
}

ofxAVS::~ofxAVS() = default;

void ofxAVS::setup(int w, int h) {
    width = w;
    height = h;
    
    // Initialize renderer
    renderer = std::make_unique<DefaultRenderer>(width, height);
    
    // Register built-in effects
    register_builtin_effects();
    
    // Initialize framebuffers
    framebuffer.resize(width * height, 0);
    output_buffer.resize(width * height, 0);
    
    // Initialize texture with RGBA format (matches new templated pixel format)
    pixels.allocate(width, height, OF_PIXELS_RGBA);
    texture.allocate(pixels);
    
    // Initialize audio buffers
    audio_buffer_left.resize(576, 0.0f);
    audio_buffer_right.resize(576, 0.0f);
    
    initialized = true;
}

void ofxAVS::update() {
    if (!initialized) return;
    
    processAudioData();
    detectBeat();
    
    // Render AVS effects
    renderer->render(vis_data, is_beat, output_buffer.data());
    
    updateTexture();
}

void ofxAVS::draw(float x, float y, float w, float h) {
    if (!initialized) return;
    
    texture.draw(x, y, w, h);
}

void ofxAVS::audioReceived(const float* input, int buffer_size, int num_channels) {
    if (!initialized) return;
    
    // Convert OF audio format to AVS format
    int samples_to_copy = std::min(buffer_size, 576);
    
    if (num_channels >= 1) {
        for (int i = 0; i < samples_to_copy; i++) {
            audio_buffer_left[i] = input[i * num_channels];
        }
    }
    
    if (num_channels >= 2) {
        for (int i = 0; i < samples_to_copy; i++) {
            audio_buffer_right[i] = input[i * num_channels + 1];
        }
    } else {
        // Mono - copy left to right
        audio_buffer_right = audio_buffer_left;
    }
}

void ofxAVS::setAudioData(const std::vector<float>& left, const std::vector<float>& right) {
    if (!initialized) return;
    
    int samples_to_copy = std::min({(int)left.size(), (int)right.size(), 576});
    
    for (int i = 0; i < samples_to_copy; i++) {
        audio_buffer_left[i] = left[i];
        audio_buffer_right[i] = right[i];
    }
}

void ofxAVS::addEffect(const std::string& effect_name) {
    if (!initialized) return;
    
    auto effect = PluginManager::instance().create_effect(effect_name);
    if (effect) {
        renderer->add_effect(std::move(effect));
    }
}

void ofxAVS::addTransformEffect(const std::string& x_expr, const std::string& y_expr) {
    if (!initialized) return;
    
    auto effect = PluginManager::instance().create_effect("transform");
    if (effect) {
        // Configure the transform expressions
        effect->parameters().set_string("x_expr", x_expr);
        effect->parameters().set_string("y_expr", y_expr);
        renderer->add_effect(std::move(effect));
    }
}

void ofxAVS::addClearEffect(bool only_first, uint32_t color) {
    if (!initialized) return;
    
    auto effect = PluginManager::instance().create_effect("clear");
    if (effect) {
        // Configure the clear effect parameters
        effect->parameters().set_bool("only_first", only_first);
        effect->parameters().set_color("color", color);
        renderer->add_effect(std::move(effect));
    }
}

void ofxAVS::clearEffects() {
    if (!initialized) return;
    
    renderer->clear_effects();
}

void ofxAVS::setSize(int w, int h) {
    if (width == w && height == h) return;
    
    width = w;
    height = h;
    
    if (initialized) {
        renderer->resize(width, height);
        
        framebuffer.resize(width * height, 0);
        output_buffer.resize(width * height, 0);
        
        pixels.allocate(width, height, OF_PIXELS_RGBA);
        texture.allocate(pixels);
    }
}

void ofxAVS::processAudioData() {
    // Convert float audio to AVS char format
    for (int i = 0; i < 576; i++) {
        // Waveform data (left and right channels)
        vis_data[0][0][i] = static_cast<char>(audio_buffer_left[i] * 127.0f);
        vis_data[0][1][i] = static_cast<char>(audio_buffer_right[i] * 127.0f);
    }
    
    // Simple FFT placeholder - in real implementation you'd use proper FFT
    // For now, copy waveform to spectrum slots
    for (int i = 0; i < 576; i++) {
        vis_data[1][0][i] = vis_data[0][0][i];
        vis_data[1][1][i] = vis_data[0][1][i];
    }
}

void ofxAVS::detectBeat() {
    // Simple beat detection based on audio energy
    float current_peak = 0.0f;
    for (int i = 0; i < 576; i++) {
        float sample = std::abs(audio_buffer_left[i]) + std::abs(audio_buffer_right[i]);
        current_peak = std::max(current_peak, sample);
    }
    
    // Beat detected if current peak significantly exceeds recent average
    is_beat = (current_peak > last_audio_peak + beat_threshold);
    last_audio_peak = last_audio_peak * 0.9f + current_peak * 0.1f; // Simple smoothing
}

void ofxAVS::updateTexture() {
    // Direct copy - RGBA format matches templated pixel format
    std::memcpy(pixels.getData(), output_buffer.data(), width * height * sizeof(uint32_t));
    texture.loadData(pixels);
}

} // namespace avs