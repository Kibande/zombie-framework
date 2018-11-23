#include <framework/components/drawable.hpp>
#include <framework/componenttype.hpp>

#include <littl/Allocator.hpp>

namespace zfw
{
    static BasicComponentType<Drawable> type;

    IComponentType& Drawable::GetType() {
        return type;
    }
}
