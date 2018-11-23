#ifndef framework_position_hpp
#define framework_position_hpp

#include <framework/datamodel.hpp>

namespace zfw
{
    struct Position
    {
        Float3 pos;

        static IAspectType& GetType();
    };
}

#endif
