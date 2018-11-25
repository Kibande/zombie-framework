#ifndef ntile_components_motion_hpp
#define ntile_components_motion_hpp

#include <framework/datamodel.hpp>

namespace ntile
{
    struct Motion
    {
        int ticksRemaining = 0;     // 0 if stationary, otherwise number of remaining ticks in motion
        int lastAnim = 0;

        zfw::Int2 nudgeDirection;   // this is the force that actually initiates movement
        zfw::Float3 speed;          // current motion velocity
    };
}

#endif
