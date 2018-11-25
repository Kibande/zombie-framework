#include "motion.hpp"

#include <framework/componenttype.hpp>

namespace zfw
{
    namespace
    {
        BasicComponentType<ntile::Motion> type;
    }

    template <>
    IComponentType& GetComponentType<ntile::Motion>() {
        return type;
    }
}
