#ifndef ntile_components_motion_hpp
#define ntile_components_motion_hpp

#include <framework/datamodel.hpp>

namespace zfw
{
    struct Motion
    {
        int ticksRemaining = 0;     // 0 if stationary, otherwise number of remaining ticks in motion
        int lastAnim = 0;

        Int2 nudgeDirection;        // this is the force that actually initiates movement
        Float3 speed;               // current motion velocity

        static IComponentType& GetType();
    };
}

#endif
