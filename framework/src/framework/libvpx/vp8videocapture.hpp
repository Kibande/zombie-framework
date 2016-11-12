#pragma once

#ifdef ZOMBIE_USE_LIBVPX
#include <framework/framework.hpp>
#include <framework/videocapture.hpp>

namespace zfw
{
    class VP8VideoCapture
    {
        public:
            static IVideoCapture* Create(SeekableOutputStream* output, Int2 resolution, Int2 frameRateNumDen);
    };
}
#endif