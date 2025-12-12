#pragma once

#include "../core/effect_base.h"

namespace avs {

class ClearEffect : public EffectBase {
public:
    ClearEffect();
    virtual ~ClearEffect() = default;
    
    // Core render function - ported from original r_clear.cpp
    int render(AudioData visdata, int isBeat,
              uint32_t* framebuffer, uint32_t* fbout,
              int w, int h) override;
    
    std::string get_name() const override { return "Clear"; }
    std::string get_description() const override { return "Render / Clear screen"; }

private:
    int frame_counter_ = 0;
    
    void setup_parameters();
};

} // namespace avs