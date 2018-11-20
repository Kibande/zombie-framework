#pragma once

#include <framework/videohandler.hpp>

#include "../RenderingKit.hpp"

namespace RenderingKit
{
    class RKVideoHandler : public zfw::IVideoHandler
    {
        public:
            /**
             * Preferred. shared_ptr only brings pain.
             * @param irm
             * @param iwm
             * @param eventQueue
             */
            RKVideoHandler(IRenderingManager* irm, IWindowManager* iwm, zfw::MessageQueue* eventQueue)
                    : irm(irm), iwm(iwm), eventQueue(eventQueue)
            {
            }

            RKVideoHandler(IRenderingManager* irm, IWindowManager* iwm, shared_ptr<zfw::MessageQueue> eventQueue)
                    : irm(irm), iwm(iwm), eventQueue(eventQueue.get()), eventQueueOwn(move(eventQueue))
            {
            }

            void BeginFrame() override
            {
                irm->BeginFrame();
            }

            void BeginDrawFrame() override
            {
                //irm->BeginDrawFrame();
            }

            bool CaptureFrame(zfw::Pixmap_t* pm_out) override
            {
                return false;
            }

            void EndFrame(int ticksElapsed) override
            {
                irm->EndFrame(ticksElapsed);
            }

            Int2 GetDefaultViewportSize() override
            {
                return iwm->GetWindowSize();
            }

            bool MoveWindow(Int2 vec) override
            {
                return iwm->MoveWindow(vec);
            }

            void ReceiveEvents() override
            {
                iwm->ReceiveEvents(eventQueue);
            }

        protected:
            IRenderingManager*  irm;
            IWindowManager*     iwm;
            zfw::MessageQueue*  eventQueue;

            shared_ptr<zfw::MessageQueue> eventQueueOwn;
    };
}
