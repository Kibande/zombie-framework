#include <framework/components/position.hpp>
#include <framework/componenttype.hpp>

namespace zfw
{
    static BasicComponentType<Position> type;

    IComponentType& Position::GetType() {
        return type;
    }
}
