#include <catch2/catch_test_macros.hpp>
#include "core/script/script_engine.h"

using namespace avs;

TEST_CASE("Audio Variables Integration", "[audio][variables]") {
    ScriptEngine engine;
    
    SECTION("Beat variable") {
        AudioData dummy_audio = {};
        
        // Test beat = false
        engine.set_audio_context(dummy_audio, false);
        REQUIRE(engine.evaluate("beat") == 0.0);
        
        // Test beat = true  
        engine.set_audio_context(dummy_audio, true);
        REQUIRE(engine.evaluate("beat") == 1.0);
    }
    
    SECTION("Waveform variables v1-v8") {
        AudioData test_audio = {};
        
        // Set up test waveform data (left channel)
        for (int i = 0; i < 576; i++) {
            test_audio[0][0][i] = (i % 127) - 64; // Sawtooth pattern
        }
        
        engine.set_audio_context(test_audio, false);
        
        // Test v1 (sample at index 0)
        double expected_v1 = static_cast<double>(test_audio[0][0][0]) / 127.0;
        REQUIRE(engine.evaluate("v1") == expected_v1);
        
        // Test v2 (sample at index 72)
        double expected_v2 = static_cast<double>(test_audio[0][0][72]) / 127.0;
        REQUIRE(engine.evaluate("v2") == expected_v2);
        
        // Test v8 (sample at index 7*72 = 504)
        double expected_v8 = static_cast<double>(test_audio[0][0][504]) / 127.0;
        REQUIRE(engine.evaluate("v8") == expected_v8);
    }
    
    SECTION("Right channel variables vr1-vr8") {
        AudioData test_audio = {};
        
        // Set up test waveform data (right channel)
        for (int i = 0; i < 576; i++) {
            test_audio[0][1][i] = 127 - (i % 127); // Reverse sawtooth
        }
        
        engine.set_audio_context(test_audio, false);
        
        // Test vr1 (right channel sample at index 0)
        double expected_vr1 = static_cast<double>(test_audio[0][1][0]) / 127.0;
        REQUIRE(engine.evaluate("vr1") == expected_vr1);
        
        // Test vr4 (right channel sample at index 3*72 = 216)
        double expected_vr4 = static_cast<double>(test_audio[0][1][216]) / 127.0;
        REQUIRE(engine.evaluate("vr4") == expected_vr4);
    }
    
    SECTION("Spectrum variables s1-s8") {
        AudioData test_audio = {};
        
        // Set up test spectrum data
        for (int i = 0; i < 576; i++) {
            test_audio[1][0][i] = (i / 8) % 127; // Step pattern
        }
        
        engine.set_audio_context(test_audio, false);
        
        // Test s1 (spectrum sample at index 0)
        double expected_s1 = static_cast<double>(test_audio[1][0][0]) / 127.0;
        REQUIRE(engine.evaluate("s1") == expected_s1);
        
        // Test s3 (spectrum sample at index 2*72 = 144)
        double expected_s3 = static_cast<double>(test_audio[1][0][144]) / 127.0;
        REQUIRE(engine.evaluate("s3") == expected_s3);
    }
    
    SECTION("Audio variables in expressions") {
        AudioData test_audio = {};
        
        // Set up known test values
        test_audio[0][0][0] = 64;   // v1 = 64/127 ≈ 0.504
        test_audio[0][0][72] = 32;  // v2 = 32/127 ≈ 0.252
        
        engine.set_audio_context(test_audio, true); // beat = true
        engine.set_pixel_context(0, 0, 100, 100);   // x = 0, y = 0
        
        // Test complex expression combining audio and position
        double result = engine.evaluate("x + v1 + beat");
        double expected = 0.0 + (64.0/127.0) + 1.0; // x + v1 + beat
        REQUIRE(std::abs(result - expected) < 0.001);
        
        // Test audio modulated transform
        double result2 = engine.evaluate("x + v2 * 0.1");
        double expected2 = 0.0 + (32.0/127.0) * 0.1;
        REQUIRE(std::abs(result2 - expected2) < 0.001);
    }
    
    SECTION("No audio context") {
        // Test that variables default to 0 when no audio context is set
        REQUIRE(engine.evaluate("beat") == 0.0);
        REQUIRE(engine.evaluate("v1") == 0.0);
        REQUIRE(engine.evaluate("vr1") == 0.0);
        REQUIRE(engine.evaluate("s1") == 0.0);
    }
}