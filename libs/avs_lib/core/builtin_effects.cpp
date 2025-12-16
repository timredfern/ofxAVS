#include "builtin_effects.h"
#include "plugin_manager.h"
#include "../effects/clear_effect.h"
#include "../effects/oscilloscope_effect.h"
#include "../effects/blur_effect.h"
#include "../effects/movement_effect.h"
#include "../effects/dynamic_movement_effect.h"

namespace avs {

void register_builtin_effects() {
    auto& pm = PluginManager::instance();
    
    // Register Clear effect
    {
        PluginInfo info;
        info.name = "Clear";
        info.description = "Clear screen effect";
        info.author = "AVS Port";
        info.version = 1;
        info.factory = []() -> std::unique_ptr<EffectBase> {
            return std::make_unique<ClearEffect>();
        };
        pm.register_effect("clear", info);
    }
    
    // Register Oscilloscope effect
    {
        PluginInfo info;
        info.name = "Oscilloscope";
        info.description = "Audio waveform display";
        info.author = "AVS Port";
        info.version = 1;
        info.factory = []() -> std::unique_ptr<EffectBase> {
            return std::make_unique<OscilloscopeEffect>();
        };
        pm.register_effect("oscilloscope", info);
    }
    
    // Register Blur effect
    {
        PluginInfo info;
        info.name = "Blur";
        info.description = "Blur effect";
        info.author = "AVS Port";
        info.version = 1;
        info.factory = []() -> std::unique_ptr<EffectBase> {
            return std::make_unique<BlurEffect>();
        };
        pm.register_effect("blur", info);
    }
    
    // Register Movement effect
    {
        PluginInfo info;
        info.name = "Movement";
        info.description = "Trans / Movement - coordinate transformations with presets";
        info.author = "AVS Port";
        info.version = 1;
        info.factory = []() -> std::unique_ptr<EffectBase> {
            return std::make_unique<MovementEffect>();
        };
        pm.register_effect("movement", info);
    }
    
    // Register Dynamic Movement effect
    {
        PluginInfo info;
        info.name = "Dynamic Movement";
        info.description = "Trans / Dynamic Movement - grid-based transformations with scripting";
        info.author = "AVS Port";
        info.version = 1;
        info.factory = []() -> std::unique_ptr<EffectBase> {
            return std::make_unique<DynamicMovementEffect>();
        };
        pm.register_effect("dynamic_movement", info);
    }
}

} // namespace avs