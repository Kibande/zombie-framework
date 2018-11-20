#ifndef ntile_ambient_hpp
#define ntile_ambient_hpp

#include "../ntile.hpp"

namespace ntile
{
    static const Float3 SUN_NIGHT(0.2f, 0.2f, 0.5f);
    static const Float3 SUN_HALFRISE(1.0f, 0.7f, 0.7f);
    static const Float3 SUN_DAY(1.0f, 1.0f, 1.0f);
    static const Float3 SUN_EVENING(1.0f, 0.9f, 0.7f);
    static const Float3 SUN_HALFSET(1.0f, 0.6f, 0.7f);

    struct SunLight
    {
        Float3 colour;
        float ambient;

        SunLight operator + (const SunLight& other) const
        {
            const SunLight sl = {colour + other.colour, ambient + other.ambient};
            return sl;
        }

        SunLight operator - (const SunLight& other) const
        {
            const SunLight sl = {colour - other.colour, ambient - other.ambient};
            return sl;
        }

        SunLight operator * (float f) const
        {
            SunLight sl = {colour * f, ambient * f};
            return sl;
        }
    };


    static const Interpolator<float, SunLight>::Keyframe sunColours[] =
    {
        // Ratios such as 1 diff : 0.2 amb (during the day) make the shapes much more pronounced,
        // which gives a nice artistic effect

        // 7:30 - 2:30 night
        // 2:30 - 3:00 sunrise
        // 3:00 - 6:00 day
        // 6:00 - 7:00 evening
        // 7:00 - 7:30 sunset

        HOUR_TICKS * 0 + MINUTE_TICKS * 0,  {SUN_NIGHT, 0.3f},
        HOUR_TICKS * 2 + MINUTE_TICKS * 30, {SUN_NIGHT, 0.3f},
        HOUR_TICKS * 2 + MINUTE_TICKS * 45, {SUN_HALFRISE, 0.4f},
        HOUR_TICKS * 3 + MINUTE_TICKS * 0,  {SUN_DAY, 0.4f},
        HOUR_TICKS * 6 + MINUTE_TICKS * 0,  {SUN_DAY, 0.4f},
        HOUR_TICKS * 7 + MINUTE_TICKS * 0,  {SUN_EVENING, 0.3f},
        HOUR_TICKS * 7 + MINUTE_TICKS * 15, {SUN_HALFSET, 0.2f},
        HOUR_TICKS * 7 + MINUTE_TICKS * 30, {SUN_NIGHT, 0.3f}
    };

    class Ambient {
    public:
        void Init(int daytime)
        {
            sunInterpolator.Init(sunColours, li_lengthof(sunColours), false);
            sunInterpolator.SetTime<CHECK_OVERFLOW>((float)daytime);
            sunInterpolator.SetMaxValue(DAY_TICKS);
        }

        void CalculateWorldLighting(int daytime,
                                    Float3& backgroundColour_out,
                                    Float3& sun_ambient_out,
                                    Float3& sun_diffuse_out,
                                    Float3& sun_direction_out)
        {
            // 2:30 - 0
            // 5:00 - 90
            // 7:30 - 180

            // calculate sun angle based on daytime
            const float sunAngle = 2.0f * f_pi * (daytime - MINUTE_TICKS * 150) / DAY_TICKS;

            // interpolate sun colour and ambient factor
            const auto& sunLight = sunInterpolator.GetValue();
            Float3 sunColour = sunLight.colour;

            backgroundColour_out = sunColour * sunLight.ambient;

            sun_ambient_out = sunColour * sunLight.ambient;

            const float angle_cutoff = f_halfpi / 6.0f;

            if (sunAngle > 0.0f && sunAngle < f_pi)
            {
                if (sunAngle < angle_cutoff)
                    sunColour *= sunAngle / angle_cutoff;
                else if (sunAngle > f_pi - angle_cutoff)
                    sunColour *= 1.0f - (sunAngle - (f_pi - angle_cutoff)) / angle_cutoff;

                const Float3 sunDir(-cos(sunAngle), 0.0f, -sin(sunAngle));

                //worldShader->SetUniformVec3(sun_dir, -Float3(modelView * sunDir));
                sun_direction_out = -sunDir;
                sun_diffuse_out = sunColour * (1.0f - sunLight.ambient);

                backgroundColour_out += sunColour * (1.0f - sunLight.ambient);
            }
            else
                // deep night, no sun diffuse needed
                sun_diffuse_out = Float3(0.0f, 0.0f, 0.0f);
        }

        void SetTime(int daytime)
        {
            sunInterpolator.SetTime<CHECK_OVERFLOW>((float)daytime);
        }

    private:
        Interpolator<float, SunLight> sunInterpolator;
    };
}

#endif
