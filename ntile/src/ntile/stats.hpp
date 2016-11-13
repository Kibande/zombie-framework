#pragma once

#include "nbase.hpp"

namespace ntile
{
    struct BaseStats
    {
        int bhp, bstr, bdef;
    };

    struct CharacterStats
    {
        int level;
        int hp, str, def;
    };

    class Stats
    {
        public:
            static void CalculateStats(const BaseStats& base, CharacterStats& stats);
    };
}
