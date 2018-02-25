#pragma once

#include <framework/utility/essentials.hpp>
#include <framework/utility/util.hpp>

#pragma warning( push )

// warning C4127: conditional expression is constant
#pragma warning( disable : 4127 )

namespace zfw
{
    enum ShouldCheckOverflow
    {
        CHECK_OVERFLOW,
        NO_CHECK_OVERFLOW
    };
    
    template <typename TimeType, typename ValueType>
    struct InterpolatorKeyframe_t
    {
        TimeType time;
        ValueType value;
    };

    template <typename TimeType, typename ValueType>
    class InterpolationModeBase
    {
        public:
            typedef InterpolatorKeyframe_t<TimeType, ValueType> Keyframe;

        protected:
            Keyframe prev, next;
            ValueType value;

        public:
            TimeType GetNextTime() const
            {
                return next.time;
            }

            const ValueType& GetValue() const
            {
                return value;
            }

            void SetSteadyValue(const ValueType& value)
            {
                this->value = value;
            }
    };

    template <typename TimeType, typename ValueType>
    class LinearInterpolationMode : public InterpolationModeBase<TimeType, ValueType>
    {
        public:
            typedef typename InterpolationModeBase<TimeType, ValueType>::Keyframe Keyframe;
        
        protected:
            ValueType derivative;

        public:
            void SetInterpolationBetween(const Keyframe& prev, const Keyframe& next)
            {
                this->prev = prev;
                this->next = next;

                derivative = (this->next.value - this->prev.value) * (1.0f / (this->next.time - this->prev.time));
            }

            void SetTime(TimeType time)
            {
                this->value = this->prev.value + derivative * (time - this->prev.time);
            }
    };

    template <typename TimeType, typename ValueType, class InterpolationMode = LinearInterpolationMode<TimeType, ValueType>>
    class Interpolator
    {
        public:
            typedef InterpolatorKeyframe_t<TimeType, ValueType> Keyframe;

        protected:
            Keyframe *releaseKeyframes;
            const Keyframe *keyframes;

            size_t numKeyframes, index;
            bool haveMore;
        
            TimeType time, maxTime;
            InterpolationMode mode;
        
            void Load();
        
        private:
            Interpolator(const Interpolator&);
        
        public:
            Interpolator();
            ~Interpolator();
        
            void Init(const Interpolator& other);
            void Init(Keyframe* keyframes, size_t count);
            void Init(const Keyframe* keyframes, size_t count, bool makeCopy);
            void Shutdown();
        
            template <ShouldCheckOverflow shouldCheckOverflow>
            const ValueType& AdvanceTime(TimeType t);
        
            template <ShouldCheckOverflow shouldCheckOverflow>
            const ValueType& SetTime(TimeType t);

            Keyframe* GetMutableKeyframes() { return releaseKeyframes; }
            size_t GetNumKeyframes() const { return numKeyframes; }
            const ValueType& GetValue() const { return mode.GetValue(); }
            void ResetTime();
            void SetMaxValue(TimeType t) { maxTime = t; }
    };
    
    template <typename TimeType, typename ValueType, class InterpolationMode>
    Interpolator<TimeType, ValueType, InterpolationMode>::Interpolator()
    {
        releaseKeyframes = nullptr;
        keyframes = nullptr;
        
        numKeyframes = 0;
        index = 0;
        haveMore = false;
    }
    
    template <typename TimeType, typename ValueType, class InterpolationMode>
    Interpolator<TimeType, ValueType, InterpolationMode>::~Interpolator()
    {
        Shutdown();
    }
    
    template <typename TimeType, typename ValueType, class InterpolationMode>
    void Interpolator<TimeType, ValueType, InterpolationMode>::Init(const Interpolator& other)
    {
        if (other.numKeyframes > 0)
            Init(other.keyframes, other.numKeyframes, true);
    }

    template <typename TimeType, typename ValueType, class InterpolationMode>
    void Interpolator<TimeType, ValueType, InterpolationMode>::Init(Keyframe* keyframes, size_t count)
    {
        ZFW_ASSERT(count > 0)
        
        releaseKeyframes = keyframes;
        this->keyframes = releaseKeyframes;
        
        numKeyframes = count;
        maxTime = this->keyframes[numKeyframes - 1].time;
        
        ResetTime();
    }
    
    template <typename TimeType, typename ValueType, class InterpolationMode>
    void Interpolator<TimeType, ValueType, InterpolationMode>::Init(const Keyframe* keyframes, size_t count, bool makeCopy)
    {
        ZFW_ASSERT(count > 0)
        
        if (makeCopy)
        {
            releaseKeyframes = Util::StructDup(keyframes, count);
            this->keyframes = releaseKeyframes;
        }
        else
        {
            releaseKeyframes = nullptr;
            this->keyframes = keyframes;
        }
        
        numKeyframes = count;
        maxTime = this->keyframes[numKeyframes - 1].time;
        
        ResetTime();
    }
    
    template <typename TimeType, typename ValueType, class InterpolationMode>
    void Interpolator<TimeType, ValueType, InterpolationMode>::Shutdown()
    {
        if (releaseKeyframes != nullptr)
        {
            free(releaseKeyframes);
            releaseKeyframes = nullptr;
        }
    }

    template <typename TimeType, typename ValueType, class InterpolationMode>
    template <ShouldCheckOverflow shouldCheckOverflow>
    const ValueType& Interpolator<TimeType, ValueType, InterpolationMode>::AdvanceTime(TimeType t)
    {
        time += t;
        
        if (!haveMore)
            return GetValue();
        
        if (shouldCheckOverflow == CHECK_OVERFLOW)
        {
            if (maxTime > 0 && time >= maxTime)
            {
                do
                    time -= maxTime;
                while (time >= maxTime);
                
                index = 0;
                Load();
            }
        }
        
        while (time >= mode.GetNextTime())
        {
            index++;
            Load();
            
            if (!haveMore)
                return GetValue();
        }
        
        mode.SetTime(time);
        return mode.GetValue();
    }
    
    template <typename TimeType, typename ValueType, class InterpolationMode>
    void Interpolator<TimeType, ValueType, InterpolationMode>::Load()
    {
        if (index + 1 >= numKeyframes)
        {
            mode.SetSteadyValue(keyframes[index].value);
            haveMore = false;
        }
        else
            mode.SetInterpolationBetween(keyframes[index], keyframes[index + 1]);
    }
    
    template <typename TimeType, typename ValueType, class InterpolationMode>
    void Interpolator<TimeType, ValueType, InterpolationMode>::ResetTime()
    {
        if (numKeyframes > 0)
        {
            time = 0;
            index = 0;
            
            mode.SetSteadyValue(keyframes[0].value);
            haveMore = true;
        
            Load();
        }
    }

    template <typename TimeType, typename ValueType, class InterpolationMode>
    template <ShouldCheckOverflow shouldCheckOverflow>
    const ValueType& Interpolator<TimeType, ValueType, InterpolationMode>::SetTime(TimeType t)
    {
        if (t > time)
            AdvanceTime<shouldCheckOverflow>(t - time);
        else
        {
            ResetTime();
            AdvanceTime<shouldCheckOverflow>(t);
        }
        
        return mode.GetValue();
    }
}

#pragma warning( pop )
