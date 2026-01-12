// avs_lib - Portable Advanced Visualization Studio library
// Based on Advanced Visualization Studio by Nullsoft, Inc.
// Original AVS Copyright (C) 2005 Nullsoft, Inc.
// Modern C++ port Copyright (C) 2025 Tim Redfern
// Licensed under MIT License

// Matrix operations ported from matrix.cpp

#pragma once

#include <cmath>
#include <cstring>

namespace avs {

// Create a rotation matrix around axis m (1=X, 2=Y, 3=Z) by Deg degrees
inline void matrix_rotate(float matrix[], int axis, float deg) {
    int m1, m2;
    float c, s;
    deg *= 3.141592653589f / 180.0f;
    std::memset(matrix, 0, sizeof(float) * 16);
    matrix[((axis - 1) << 2) + axis - 1] = matrix[15] = 1.0f;
    m1 = (axis % 3);
    m2 = ((m1 + 1) % 3);
    c = std::cos(deg);
    s = std::sin(deg);
    matrix[(m1 << 2) + m1] = c;
    matrix[(m1 << 2) + m2] = s;
    matrix[(m2 << 2) + m2] = c;
    matrix[(m2 << 2) + m1] = -s;
}

// Create a translation matrix
inline void matrix_translate(float m[], float x, float y, float z) {
    std::memset(m, 0, sizeof(float) * 16);
    m[0] = m[4 + 1] = m[8 + 2] = m[12 + 3] = 1.0f;
    m[0 + 3] = x;
    m[4 + 3] = y;
    m[8 + 3] = z;
}

// Multiply dest by src, storing result in dest
inline void matrix_multiply(float* dest, float src[]) {
    float temp[16];
    std::memcpy(temp, dest, sizeof(float) * 16);
    for (int i = 0; i < 16; i += 4) {
        *dest++ = src[i + 0] * temp[(0 << 2) + 0] + src[i + 1] * temp[(1 << 2) + 0] +
                  src[i + 2] * temp[(2 << 2) + 0] + src[i + 3] * temp[(3 << 2) + 0];
        *dest++ = src[i + 0] * temp[(0 << 2) + 1] + src[i + 1] * temp[(1 << 2) + 1] +
                  src[i + 2] * temp[(2 << 2) + 1] + src[i + 3] * temp[(3 << 2) + 1];
        *dest++ = src[i + 0] * temp[(0 << 2) + 2] + src[i + 1] * temp[(1 << 2) + 2] +
                  src[i + 2] * temp[(2 << 2) + 2] + src[i + 3] * temp[(3 << 2) + 2];
        *dest++ = src[i + 0] * temp[(0 << 2) + 3] + src[i + 1] * temp[(1 << 2) + 3] +
                  src[i + 2] * temp[(2 << 2) + 3] + src[i + 3] * temp[(3 << 2) + 3];
    }
}

// Apply matrix transformation to point (x,y,z), output to (outx,outy,outz)
inline void matrix_apply(float* m, float x, float y, float z,
                         float* outx, float* outy, float* outz) {
    *outx = x * m[0] + y * m[1] + z * m[2] + m[3];
    *outy = x * m[4] + y * m[5] + z * m[6] + m[7];
    *outz = x * m[8] + y * m[9] + z * m[10] + m[11];
}

} // namespace avs
