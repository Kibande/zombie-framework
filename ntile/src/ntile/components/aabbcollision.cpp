#include "aabbcollision.hpp"

#include <framework/componenttype.hpp>

namespace zfw
{
    namespace
    {
        BasicComponentType<ntile::AabbCollision> type;
    }

    template <>
    IComponentType& GetComponentType<ntile::AabbCollision>() {
        return type;
    }
}
