#include <framework/aspects/position.hpp>
#include <framework/aspecttype.hpp>

namespace zfw
{
    static GenericAspectType<Position> type;

    IAspectType& Position::GetType() {
        return type;
    }
}
