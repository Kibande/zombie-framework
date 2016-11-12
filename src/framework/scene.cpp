
#include <framework/event.hpp>
#include <framework/messagequeue.hpp>
#include <framework/scene.hpp>

namespace zfw
{
    CameraMouseControlBase::CameraMouseControlBase()
    {
        panning = false;

        panSpeedX = 0.02f;
        panSpeedY = 0.02f;
        zoomAmt = 1.0f;
    }

    int CameraMouseControlBase::HandleMessage(MessageHeader* msg)
    {
        switch (msg->type)
        {
            case EVENT_MOUSE_MOVE:
            {
                auto ev = msg->Data<EventMouseMove>();

                if (panning)
                {
                    RotateCameraXY(ev->y - mouseOrigin.y);
                    RotateCameraZ(ev->x - mouseOrigin.x);
                }

                mouseOrigin = Int2(ev->x, ev->y);
                return 0;
            }

            case EVENT_VKEY:
            {
                auto ev = msg->Data<EventVkey>();

                int button;
                bool pressed;

                if (!Vkey::IsMouseButtonEvent(ev->input, button, pressed))
                    break;

                if (button == MOUSEBTN_LEFT)
                {
                    if (pressed)
                        panning = true;
                    else
                        panning = false;

                    return 1;
                }
                else if (button == MOUSEBTN_WHEEL_DOWN)
                {
                    ZoomCamera(zoomAmt);
                    return 1;
                }
                else if (button == MOUSEBTN_WHEEL_UP)
                {
                    ZoomCamera(-zoomAmt);
                    return 1;
                }
                break;
            }
        }

        return -1;
    }
}

