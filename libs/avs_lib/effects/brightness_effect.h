#pragma once

#include "../core/effect_base.h"

namespace avs {

class BrightnessEffect : public EffectBase {
public:
    BrightnessEffect();
    virtual ~BrightnessEffect() = default;
    
    // Core render function
    int render(AudioData visdata, int isBeat,
              uint32_t* framebuffer, uint32_t* fbout,
              int w, int h) override;
    
    std::string get_name() const override { return "Brightness"; }
    std::string get_description() const override { return "Trans / Brightness"; }

private:
    void setup_parameters();
};

} // namespace avs