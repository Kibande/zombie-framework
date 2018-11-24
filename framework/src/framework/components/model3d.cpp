#include <framework/components/model3d.hpp>
#include <framework/componenttype.hpp>

#include <littl/Allocator.hpp>

namespace zfw
{
    static BasicComponentType<Model3D> type;

    IComponentType& Model3D::GetType() {
        return type;
    }
}
