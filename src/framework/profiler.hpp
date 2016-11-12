#pragma once

#include <framework/base.hpp>

namespace zfw
{
    typedef unsigned int TimeUnit_t;

    struct ProfilingSection_t
    {
        const char* name;
        TimeUnit_t begin, end;

        ProfilingSection_t* parent;
        ProfilingSection_t* nextSibling;
        ProfilingSection_t* firstChild;
        ProfilingSection_t* lastChild;
    };

    class Profiler
    {
        public:
            static Profiler* Create();
            virtual ~Profiler() {}

            virtual void BeginProfiling() = 0;
            virtual void EnterSection(ProfilingSection_t& section) = 0;
            virtual void LeaveSection() = 0;
            virtual void EndProfiling() = 0;

            virtual void PrintProfile() = 0;
    };
}
