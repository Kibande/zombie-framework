#include "components.hpp"

#include <framework/componenttype.hpp>

namespace zfw
{
    namespace
    {
        BasicComponentType<example::PointLight> pointLightType;
        BasicComponentType<example::SkyBox> skyBoxType;
        BasicComponentType<example::WorldGeometry> worldGeometryType;
    }

    template <>
    IComponentType& GetComponentType<example::PointLight>() {
        return pointLightType;
    }

    template <>
    IComponentType& GetComponentType<example::SkyBox>() {
        return skyBoxType;
    }

    template <>
    IComponentType& GetComponentType<example::WorldGeometry>() {
        return worldGeometryType;
    }
}
