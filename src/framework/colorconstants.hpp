#pragma once

#include <framework/datamodel.hpp>

namespace zfw
{
    // Float4
    static const Float4 COLOUR_WHITE(1.0f, 1.0f, 1.0f, 1.0f);
    static const Float4 COLOUR_BLACK(0.0f, 0.0f, 0.0f, 1.0f);

    static inline Float4 COLOUR_BLACKA(float a) { return glm::vec4(0.0f, 0.0f, 0.0f, a); }
    static inline Float4 COLOUR_WHITEA(float a) { return glm::vec4(1.0f, 1.0f, 1.0f, a); }
    static inline Float4 COLOUR_GREY(float l, float a = 1.0f) { return glm::vec4(l, l, l, a); }

    // Float4 colours from bytes

    // Byte4 colours
    static const Byte4 RGBA_BLACK(0, 0, 0, 255);
    static const Byte4 RGBA_WHITE(255, 255, 255, 255);

    inline Byte4 RGBA_COLOUR(int r, int g, int b, int a = 255) { return Byte4(r, g, b, a); }
    inline Byte4 RGBA_COLOUR(float r, float g, float b, float a = 1.0f) { return Byte4(r * 255.0f, g * 255.0f, b * 255.0f, a * 255.0f); }

    inline Byte4 RGBA_BLACK_A(int a) { return Byte4(0, 0, 0, a); }
    inline Byte4 RGBA_GREY(int l, int a = 255) { return Byte4(l, l, l, a); }
    inline Byte4 RGBA_WHITE_A(int a) { return Byte4(255, 255, 255, a); }

    // Byte4 colours from floats
    inline Byte4 RGBA_BLACK_A(float a) { return Byte4(0, 0, 0, a * 255.0f); }
    inline Byte4 RGBA_GREY(float l, float a = 1.0f) { return Byte4(l * 255.0f, l * 255.0f, l * 255.0f, a * 255.0f); }
    inline Byte4 RGBA_WHITE_A(float a) { return Byte4(255, 255, 255, a * 255.0f); }

    inline Byte4 rgbCSS(uint32_t hex) { return Byte4(hex >> 16, hex >> 8, hex, 0xff); }
}
