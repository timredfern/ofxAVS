// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

#pragma once

#include "core/effect_base.h"
#include "core/script/script_engine.h"
#include <vector>
#include <cmath>

namespace avs {

class DDMEffect : public EffectBase {
public:
    DDMEffect();
    virtual ~DDMEffect() = default;

    int render(AudioData visdata, int isBeat,
               uint32_t* framebuffer, uint32_t* fbout,
               int w, int h) override;

    const PluginInfo& get_plugin_info() const override { return effect_info; }

    void on_parameter_changed(const std::string& param_name) override;

    static const PluginInfo effect_info;

private:
    void generate_distance_table(int imax_d);

    // Integer square root using lookup table (from original AVS)
    static unsigned long isqrt(unsigned long n);

    // Script engine for expression evaluation
    ScriptEngine script_engine_;
    bool inited_ = false;

    // Cached dimensions
    int last_w_ = 0;
    int last_h_ = 0;
    double max_d_ = 0.0;

    // Row multiplication table for fast pixel indexing
    std::vector<int> wmul_;

    // Distance modification table
    std::vector<int> tab_;

    // Square root lookup table (static, shared by all instances)
    static const unsigned char sq_table_[256];
};

} // namespace avs
