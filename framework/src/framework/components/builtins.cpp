#include <framework/components/model3d.hpp>
#include <framework/components/position.hpp>
#include <framework/componenttype.hpp>

namespace zfw
{
    static BasicComponentType<Model3D> model3DType;
    static BasicComponentType<Position> positionType;

    template <>
    IComponentType& GetComponentType<Model3D>() {
        return model3DType;
    }

    template <>
    IComponentType& GetComponentType<Position>() {
        return positionType;
    }
}
