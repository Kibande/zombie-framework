#pragma once

#include <framework/videohandler.hpp>

#include "../RenderingKit.hpp"

namespace RenderingKit
{
    class RKVideoHandler : public zfw::IVideoHandler
    {
        public:
            RKVideoHandler(IRenderingManager* irm, IWindowManager* iwm, shared_ptr<zfw::MessageQueue> eventQueue)
                    : irm(irm), iwm(iwm), eventQueue(move(eventQueue))
            {
            }

            virtual void BeginFrame()
            {
                irm->BeginFrame();
            }

            virtual void BeginDrawFrame()
            {
                //irm->BeginDrawFrame();
            }

            virtual bool CaptureFrame(zfw::Pixmap_t* pm_out)
            {
                return false;
            }

            virtual void EndFrame(int ticksElapsed)
            {
                irm->EndFrame(ticksElapsed);
            }

            virtual Int2 GetDefaultViewportSize()
            {
                return iwm->GetWindowSize();
            }

            virtual bool MoveWindow(Int2 vec)
            {
                return iwm->MoveWindow(vec);
            }

            virtual void ReceiveEvents()
            {
                iwm->ReceiveEvents(eventQueue.get());
            }

        protected:
            IRenderingManager*  irm;
            IWindowManager*     iwm;

            shared_ptr<zfw::MessageQueue> eventQueue;
    };
}
