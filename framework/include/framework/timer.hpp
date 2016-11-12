#pragma once

#include <framework/base.hpp>

namespace zfw
{
    class Timer
    {
        public:
            virtual ~Timer() {}

            virtual uint64_t GetGlobalMicros() = 0;

            virtual uint64_t GetMicros() = 0;
            virtual bool IsStarted() = 0;
            virtual bool IsPaused() = 0;

            virtual void Pause() = 0;
            virtual void Reset() = 0;
            virtual void Start() = 0;
            virtual void Stop() = 0;
    };
}
