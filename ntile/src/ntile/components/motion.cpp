#include "motion.hpp"

#include <framework/componenttype.hpp>

namespace zfw
{
    static BasicComponentType<Motion> type;

    IComponentType& Motion::GetType() {
        return type;
    }
}
