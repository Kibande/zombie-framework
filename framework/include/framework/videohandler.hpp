#pragma once

#include <framework/datamodel.hpp>

namespace zfw
{
    class IVideoHandler
    {
        public:
            virtual ~IVideoHandler() {}
            
            virtual Int2 GetDefaultViewportSize() = 0;
            virtual bool MoveWindow(Int2 vec) = 0;
            virtual void ReceiveEvents() = 0;

            virtual void BeginFrame() = 0;
            virtual void BeginDrawFrame() = 0;
            virtual void EndFrame(int ticksElapsed) = 0;

            virtual bool CaptureFrame(Pixmap_t* pm_out) = 0;

            // This will go (hoepfully soon)
            virtual const char* JoystickDevToName(int dev) { return nullptr; }
            virtual int JoystickNameToDev(const char* name) { return -1; }
    };
}
