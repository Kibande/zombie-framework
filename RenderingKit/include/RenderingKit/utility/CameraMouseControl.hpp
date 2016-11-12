
#include <framework/scene.hpp>

#include "../RenderingKit.hpp"

namespace RenderingKit
{
    class RKCameraMouseControl : public zfw::CameraMouseControlBase
    {
        ICamera* cam;

        protected:
            virtual void RotateCameraXY(int mouseAmount) override
            {
                if (cam != nullptr)
                    cam->CameraRotateXY(mouseAmount * panSpeedY, false);
            }

            virtual void RotateCameraZ(int mouseAmount) override
            {
                if (cam != nullptr)
                    cam->CameraRotateZ(mouseAmount * -panSpeedX, false);
            }

            virtual void ZoomCamera(float amount) override
            {
                if (cam != nullptr)
                    cam->CameraZoom(amount, false);
            }

        public:
            RKCameraMouseControl() : cam(nullptr) {}

            void SetCamera(ICamera* cam) { this->cam = cam; }
    };
}
