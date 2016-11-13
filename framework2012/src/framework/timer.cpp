
#include "framework.hpp"

#ifdef __li_POSIX
#include <sys/time.h>
#endif

namespace zfw
{
#ifdef __li_MSW
    static bool frequencyKnown = false;
    static LARGE_INTEGER frequency, temp;
#endif

    Timer::Timer() : startTicks( 0 ), pauseTicks( 0 ), paused( false ), started( false )
    {
    }

    uint64_t Timer::getMicros() const
    {
        if ( started )
        {
            if ( paused )
                return pauseTicks;
            else
                return getRelativeMicroseconds() - startTicks;
        }

        return 0;
    }

    /*uint64_t Timer::getMilis() const
    {
        return getMicros() / 1000;
    }*/

    uint64_t Timer::getRelativeMicroseconds()
    {
#ifdef __li_MSW
        if ( !frequencyKnown )
        {
            QueryPerformanceFrequency( &frequency );
            frequencyKnown = true;
        }

        QueryPerformanceCounter( &temp );
        return temp.QuadPart * 1000000 / frequency.QuadPart;
#elif defined (__li_POSIX)
        timeval tp;
        
        gettimeofday(&tp, NULL);
        
        return (int64_t)tp.tv_sec * 1000000L + tp.tv_usec;
#else
#error Define me!!!
#endif
    }

    void Timer::pause()
    {
        if ( started && !paused )
        {
            paused = true;
            pauseTicks = getRelativeMicroseconds() - startTicks;
        }
    }

    void Timer::reset()
    {
        if ( paused )
            pauseTicks = 0;
        else
            startTicks = getRelativeMicroseconds();
    }

    void Timer::start()
    {
        startTicks = getRelativeMicroseconds() - pauseTicks;
        pauseTicks = 0;

        started = true;
        paused = false;
    }

    void Timer::stop()
    {
        started = false;
        paused = false;
    }
}

