#pragma once

#include <framework/datamodel.hpp>

#include <vector>

namespace zfw
{
    enum class PixmapFormat_t
    {
        BGR8,
        RGB8,
        RGBA8,
    };

    struct PixmapInfo_t
    {
        Int2                    size;
        PixmapFormat_t          format;
    };

    struct Pixmap_t
    {
        PixmapInfo_t            info;
        std::vector<uint8_t>    pixelData;          // ALWAYS aligned to 4 bytes per line
    };
}
