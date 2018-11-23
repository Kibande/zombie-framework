#include "aabbcollision.hpp"

#include <framework/componenttype.hpp>

namespace zfw
{
    static BasicComponentType<AabbCollision> type;

    IComponentType& AabbCollision::GetType() {
        return type;
    }
}
