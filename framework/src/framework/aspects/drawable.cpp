#include <framework/aspects/drawable.hpp>
#include <framework/aspecttype.hpp>

#include <littl/Allocator.hpp>

namespace zfw
{
    static GenericAspectType<Drawable> type;

    IAspectType& Drawable::GetType() {
        return type;
    }
}
