
#include <framework/base.hpp>
#include <framework/timer.hpp>

#ifdef li_MSW
#include <windows.h>
#endif

#ifdef __li_POSIX
#include <sys/time.h>
#endif

#ifdef ZOMBIE_CTR
extern "C" u64 osGetMicros();
#endif

namespace zfw
{
    using namespace li;

#ifdef __li_MSW
    static bool frequencyKnown = false;
    static LARGE_INTEGER frequency, temp;
#endif

    class TimerImpl : public Timer
    {
        uint64_t startTicks, pauseTicks;
        bool paused, started;

        public:
            TimerImpl();

            virtual uint64_t GetGlobalMicros() override;

            virtual uint64_t GetMicros() override;
            virtual bool IsStarted() override { return started; }
            virtual bool IsPaused() override { return paused; }

            virtual void Pause() override;
            virtual void Reset() override;
            virtual void Start() override;
            virtual void Stop() override;
    };

    Timer* p_CreateTimer(ErrorBuffer_t* eb)
    {
        return new TimerImpl();
    }

    TimerImpl::TimerImpl() : startTicks( 0 ), pauseTicks( 0 ), paused( false ), started( false )
    {
    }

    uint64_t TimerImpl::GetGlobalMicros()
    {
#ifdef __li_MSW
        if ( !frequencyKnown )
        {
            QueryPerformanceFrequency( &frequency );
            frequencyKnown = true;
        }

        QueryPerformanceCounter( &temp );
        return temp.QuadPart * 1000000 / frequency.QuadPart;
#elif defined (_3DS)
        return osGetMicros();
#elif defined (__li_POSIX)
        timeval tp;
        
        gettimeofday(&tp, NULL);
        
        return (int64_t)tp.tv_sec * 1000000L + tp.tv_usec;
#else
#error Define me!!!
#endif
    }

    uint64_t TimerImpl::GetMicros()
    {
        if ( started )
        {
            if ( paused )
                return pauseTicks;
            else
                return GetGlobalMicros() - startTicks;
        }

        return 0;
    }

    void TimerImpl::Pause()
    {
        if ( started && !paused )
        {
            paused = true;
            pauseTicks = GetGlobalMicros() - startTicks;
        }
    }

    void TimerImpl::Reset()
    {
        if ( paused )
            pauseTicks = 0;
        else
            startTicks = GetGlobalMicros();
    }

    void TimerImpl::Start()
    {
        startTicks = GetGlobalMicros() - pauseTicks;
        pauseTicks = 0;

        started = true;
        paused = false;
    }

    void TimerImpl::Stop()
    {
        started = false;
        paused = false;
    }
}

