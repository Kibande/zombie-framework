#pragma once

// Pulls in littl too (which is good)
#include <framework/base.hpp>

namespace n3d
{
    // Any nicer way to do this?
}

namespace ntile
{
    using namespace li;
    using namespace zfw;
    using namespace n3d;

    enum
    {
        BLOCK_WORLD = 0x00,
        BLOCK_SEA = 0x01,
        BLOCK_SHIROI_OUTSIDE = 0x10
    };

    enum
    {
        TILES_IN_BLOCK_H = 16,
        TILES_IN_BLOCK_V = 16,
        TILE_SIZE_H = 16,
        TILE_SIZE_V = 16,
    };

    enum
    {
        MINUTE_TICKS =  60,
        HOUR_TICKS =    60 * MINUTE_TICKS,
        DAY_TICKS =     10 * HOUR_TICKS
    };

    class IGameScreen;
}
