#ifndef framework_components_position_hpp
#define framework_components_position_hpp

#include <framework/datamodel.hpp>

namespace zfw
{
    struct Position
    {
        Float3 pos;

        static IComponentType& GetType();
    };
}

#endif
