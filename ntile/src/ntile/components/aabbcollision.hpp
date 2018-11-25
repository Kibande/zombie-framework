#ifndef ntile_components_aabbcollision_hpp
#define ntile_components_aabbcollision_hpp

#include <framework/datamodel.hpp>

namespace ntile
{
    struct AabbCollision
    {
        zfw::Float3 min, max;
    };
}

#endif
