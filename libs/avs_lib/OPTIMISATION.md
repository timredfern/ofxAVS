# avs_lib Optimisation Notes

## Bit-Shift Division for Blur Effects

The original AVS blur effect (`r_blur.cpp`) uses a clever bit-shift technique for fast division of packed RGB pixels without separating color channels. This avoids expensive per-channel extraction and recombination.

### The Problem

When dividing a packed 32-bit ARGB pixel (0xAARRGGBB) by a power of 2, a simple shift causes bits from one channel to overflow into the adjacent channel. For example, shifting `0x00FF0000` (pure red) right by 1 would produce `0x007F8000`, where bits have leaked into the green channel.

### The Solution: Channel Isolation Masks

The original AVS code defines masks that clear the bits that would overflow before shifting:

```cpp
// Bit masks for fast division without overflow between color channels
#define MASK_SH1 (~(((1<<7)|(1<<15)|(1<<23))<<1))   // For /2
#define MASK_SH2 (~(((3<<6)|(3<<14)|(3<<22))<<2))   // For /4
#define MASK_SH3 (~(((7<<5)|(7<<13)|(7<<21))<<3))   // For /8
#define MASK_SH4 (~(((15<<4)|(15<<12)|(15<<20))<<4)) // For /16

// Fast division macros using bit shifts (no actual division)
#define DIV_2(x)  (((x) & MASK_SH1) >> 1)
#define DIV_4(x)  (((x) & MASK_SH2) >> 2)
#define DIV_8(x)  (((x) & MASK_SH3) >> 3)
#define DIV_16(x) (((x) & MASK_SH4) >> 4)
```

For MASK_SH2 (divide by 4), the mask clears the top 2 bits of each 8-bit channel before shifting right by 2. This ensures that when shifted, no bits cross channel boundaries.

### Algorithm Complexity

The blur operates in a single O(n) pass over the image. Each pixel reads its 4 neighbors (up, down, left, right) and combines them with weighted bit-shift operations. The three blur levels use different weight distributions:

- **Light**: 3/4 center + 1/16 each neighbor (preserves detail)
- **Medium**: 1/2 center + 1/8 each neighbor (balanced)
- **Heavy**: 0 center + 1/4 each neighbor (maximum smoothing)

## Auto-Vectorization

### Compiler Output

With `-O3` optimisation, clang auto-vectorizes the blur inner loop to ARM NEON SIMD instructions. The following assembly excerpt from `blur_effect.s` shows the vectorized code:

```asm
; Load 4 pixels at once using NEON interleaved load
ld4.4s  { v2, v3, v4, v5 }, [x19]

; Unsigned shift right by 2 (DIV_4) on 4 pixels simultaneously
ushr.4s v6, v4, #2

; AND with mask to prevent channel overflow
and.16b v6, v6, v1

; Load more pixels for parallel processing
ld4.4s  { v16, v17, v18, v19 }, [x25]
ld4.4s  { v20, v21, v22, v23 }, [x19]

; More shift-and-mask operations
ushr.4s v7, v2, #2
and.16b v7, v7, v1
ushr.4s v24, v16, #2
and.16b v24, v24, v1
ushr.4s v25, v20, #2
and.16b v25, v25, v1

; Accumulate results
add.4s  v6, v6, v0
add.4s  v7, v7, v24
add.4s  v6, v6, v7
add.4s  v24, v6, v25
```

Key NEON instructions:
- `ld4.4s` - Load 4x4 32-bit values (structure-of-arrays load)
- `ushr.4s` - Unsigned shift right on 4 32-bit values simultaneously
- `and.16b` - Bitwise AND on 16 bytes (4 pixels)
- `add.4s` - Add 4 32-bit values in parallel

### Compiler Flags

OpenFrameworks Release builds use the following optimisation flags:

```
-O3 -mtune=native
```

These enable:
- Full optimisation including loop unrolling
- Auto-vectorization to SIMD
- Target-specific tuning (NEON on ARM, SSE/AVX on x86)

### Generating Assembly Output

To check if code is being vectorized:

```bash
clang++ -std=c++17 -O3 -S blur_effect.cpp -I../core -o blur_effect.s
```

Then search for NEON patterns:
```bash
grep -E "\.4s|\.16b|ushr|add\.4s" blur_effect.s
```

## NEON Intrinsics

If auto-vectorization proves insufficient for a particular algorithm, NEON intrinsics provide a middle ground before resorting to raw assembly. Intrinsics are C/C++ functions that map directly to SIMD instructions while remaining portable and maintainable.

### Example: Vectorized DIV_4

```cpp
#include <arm_neon.h>

// Process 4 pixels at once
void blur_4pixels_neon(uint32_t* dst, const uint32_t* src, uint32x4_t mask) {
    // Load 4 pixels
    uint32x4_t pixels = vld1q_u32(src);

    // Shift right by 2 (divide by 4)
    uint32x4_t shifted = vshrq_n_u32(pixels, 2);

    // Apply mask to prevent channel overflow
    uint32x4_t result = vandq_u32(shifted, mask);

    // Store 4 pixels
    vst1q_u32(dst, result);
}
```

### Key NEON Intrinsic Functions

| Intrinsic | Assembly | Description |
|-----------|----------|-------------|
| `vld1q_u32()` | `ld1.4s` | Load 4 x 32-bit values |
| `vst1q_u32()` | `st1.4s` | Store 4 x 32-bit values |
| `vshrq_n_u32()` | `ushr.4s` | Unsigned shift right |
| `vandq_u32()` | `and.16b` | Bitwise AND |
| `vaddq_u32()` | `add.4s` | Add |
| `vdupq_n_u32()` | `dup.4s` | Broadcast scalar to all lanes |

### When to Use Intrinsics

- When the compiler fails to auto-vectorize a critical loop
- When you need precise control over memory access patterns
- When profiling shows a specific hotspot that needs manual optimization
- When porting x86 SSE/AVX code that used intrinsics

### Cross-Platform Considerations

For portable SIMD, consider:
- **Runtime detection**: Check CPU features before using NEON paths
- **Fallback paths**: Always provide scalar implementations
- **Abstraction libraries**: Libraries like `simde` or `xsimd` can abstract across ARM/x86

## Conclusions

1. **The original AVS bit-shift technique is optimal** - It avoids per-channel operations and maps directly to SIMD shift/mask instructions.

2. **Hand-written SIMD is not necessary** - Clang's auto-vectorizer effectively converts the scalar C++ to NEON code.

3. **The 4-pixel-at-a-time loop unrolling helps** - The compiler can better schedule memory operations and keep SIMD lanes utilized.

4. **Original MMX assembly was for its era** - The x86 MMX assembly in the original AVS (circa 2005) was necessary because compilers of that time couldn't auto-vectorize effectively. Modern compilers make this unnecessary.
