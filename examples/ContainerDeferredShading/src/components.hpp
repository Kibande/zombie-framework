#ifndef CONTAINERDEFERREDSHADING_POINTLIGHT_HPP
#define CONTAINERDEFERREDSHADING_POINTLIGHT_HPP

#include <framework/datamodel.hpp>

namespace example
{
    using namespace zfw;

    struct PointLight {
        bool castsShadows = true;
        Float3 pos;
        Float3 ambient;
        Float3 diffuse;
        float range;
    };

    struct SkyBox {
        std::string texturePath;
    };

    struct WorldGeometry {
        std::string recipe;
    };
}

#endif //CONTAINERDEFERREDSHADING_POINTLIGHT_HPP
