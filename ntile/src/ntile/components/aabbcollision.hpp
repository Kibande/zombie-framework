#ifndef ntile_components_aabbcollision_hpp
#define ntile_components_aabbcollision_hpp

#include <framework/datamodel.hpp>

namespace zfw
{
    struct AabbCollision
    {
        Float3 min, max;

        static IComponentType& GetType();
    };
}

#endif
