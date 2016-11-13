
#include "stats.hpp"

#include <cmath>

namespace ntile
{
    void Stats::CalculateStats(const BaseStats& base, CharacterStats& stats)
    {
        stats.hp = 8 + (int)round(stats.level * base.bhp / 10.0f);
        stats.str = 2 + (int)round(pow(stats.level, 1.2f) * base.bstr / 40.0f);
        stats.def = 2 + (int)round(pow(stats.level, 1.2f) * base.bdef / 40.0f);
    }
}
