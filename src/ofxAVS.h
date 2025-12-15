#pragma once

#include "ofMain.h"
#include "avs_lib/core/renderer.h"
#include "avs_lib/core/builtin_effects.h"
#include "avs_lib/core/coordinate_lookup_table.h"

namespace avs {

class ofxAVS {
public:
    ofxAVS();
    ~ofxAVS();
    
    void setup(int width, int height);
    void update();
    void draw(float x, float y, float w, float h);
    
    // Audio input methods
    void audioReceived(const float* input, int buffer_size, int num_channels);
    void setAudioData(const std::vector<float>& left, const std::vector<float>& right);
    
    // Effect management
    void addEffect(const std::string& effect_name);
    void addTransformEffect(const std::string& x_expr, const std::string& y_expr, 
                          int grid_width = 16, int grid_height = 16, 
                          InterpolationMode interp_mode = InterpolationMode::NONE);
    void addTransformEffect(const std::string& x_expr, const std::string& y_expr, 
                          int grid_width, int grid_height, int interpolation);
    void addClearEffect(bool only_first = false, uint32_t color = 0xFF000000);
    void clearEffects();
    
    // Beat detection
    void setBeatThreshold(float threshold) { beat_threshold = threshold; }
    bool isBeat() const { return is_beat; }
    
    // Rendering options
    void setSize(int width, int height);
    ofTexture& getTexture() { return texture; }
    
private:
    void processAudioData();
    void detectBeat();
    void updateTexture();
    
    // Core AVS components
    std::unique_ptr<DefaultRenderer> renderer;
    
    // OpenFrameworks integration
    ofTexture texture;
    ofPixels pixels;
    
    // Audio processing
    AudioData vis_data;
    std::vector<float> audio_buffer_left;
    std::vector<float> audio_buffer_right;
    float beat_threshold;
    bool is_beat;
    float last_audio_peak;
    
    // Rendering state
    int width, height;
    std::vector<uint32_t> framebuffer;
    std::vector<uint32_t> output_buffer;
    bool initialized;
};

} // namespace avs