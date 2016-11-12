#pragma once

#include <framework/framework.hpp>

#ifdef ZOMBIE_USE_RENDERING_KIT
#include <RenderingKit/RenderingKit.hpp>
#endif

namespace zfw
{
    struct MessageHeader;

    class IScene
    {
        public:
            virtual ~IScene() {}

            virtual bool Init() = 0;
            virtual void Shutdown() = 0;

            virtual bool AcquireResources() = 0;
            virtual void DropResources() = 0;

            virtual void DrawScene() {}
            virtual void OnFrame( double delta ) {}
            virtual void OnTicks(int ticks) {}
    };

    class CameraMouseControlBase
    {
        bool panning;
        Int2 mouseOrigin;

        protected:
            float panSpeedX, panSpeedY;
            float zoomAmt;

            virtual void RotateCameraXY(int mouseAmount) = 0;
            virtual void RotateCameraZ(int mouseAmount) = 0;
            virtual void ZoomCamera(float amount) = 0;

        public:
            CameraMouseControlBase();

            int HandleMessage(MessageHeader* msg);
    };
}
