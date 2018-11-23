#ifndef framework_components_position_hpp
#define framework_components_position_hpp

#include <framework/datamodel.hpp>

#include <glm/gtc/quaternion.hpp>

namespace zfw
{
    struct Position
    {
        Float3 pos;
        glm::fquat rotation;
        Float3 scale = {1, 1, 1};

        static IComponentType& GetType();
    };
}

#endif
