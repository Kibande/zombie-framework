#pragma once

#include <framework/base.hpp>

namespace zfw
{
    class IVideoCapture
    {
        protected:
            ~IVideoCapture() {}

        public:
            virtual void Release() = 0;

            virtual int StartCapture(Int2 resolution, int tickRate) = 0;
            virtual int EndCapture() = 0;

            // frameid:     global frame counter
            // frameticks:  ticks elapsed in this frame
            // Return 1 and fill in output pointers if interested in this frame (do your timing yourself)
            virtual int OfferFrameBGRFlipped(int frameid, int frameticks, uint8_t** rgb_buf_out) = 0;

            // Called if OfferFrame* returns 1, supplied buffer is filled in now
            virtual void OnFrameReady(int frameid) = 0;
    };
}
