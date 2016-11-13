
#include "n3d_ctr_internal.hpp"

#include <framework/event.hpp>
#include <framework/timer.hpp>
#include <framework/videohandler.hpp>

namespace n3d
{
    static const char* controllerName = "Nintendo 3DS";

    static void PushMouseButtonEvent(MessageQueue* msgQueue, int x, int y, int flags, int button)
    {
        MessageConstruction<EventVkey> msg(msgQueue);
        msg->input.vkey.type = VKEY_MOUSEBTN;
        msg->input.vkey.device = -1;
        msg->input.vkey.key = button;
        msg->input.vkey.subkey = -1;
        msg->input.flags = flags;
        msg->input.value = 0;
    }

    static void PushVkeyEvent(MessageQueue* msgQueue, int16_t inclass, int16_t dev, int16_t key, int16_t subkey, int flags, int value)
    {
        MessageConstruction<EventVkey> msg(msgQueue);
        msg->input.vkey.type = inclass;
        msg->input.vkey.device = dev;
        msg->input.vkey.key = key;
        msg->input.vkey.subkey = subkey;
        msg->input.flags = flags;
        msg->input.value = value;

        //Sys::printk("Vkey [%s] &%08X =%i", Event::FormatVkey(msg->vkey.vk), msg->vkey.flags, msg->vkey.value);
    }

    static void KeyEvents(MessageQueue* msgQueue, uint32_t keys, int flags)
    {
        static const uint32_t validKeys = (KEY_A | KEY_B | KEY_SELECT | KEY_START
                | KEY_DRIGHT | KEY_DLEFT | KEY_DUP | KEY_DDOWN
                | KEY_R | KEY_L | KEY_X | KEY_Y | KEY_ZL | KEY_ZR
                | KEY_CSTICK_RIGHT | KEY_CSTICK_LEFT | KEY_CSTICK_UP | KEY_CSTICK_DOWN
                | KEY_CPAD_RIGHT | KEY_CPAD_LEFT | KEY_CPAD_UP | KEY_CPAD_DOWN
                );

        for (int index = 0; index < 32; index++)
        {
            if (!((1 << index) & validKeys))
                continue;

            if (keys & (1 << index))
                PushVkeyEvent(msgQueue, VKEY_JOYBTN, -1, index, 0, flags, 0);
        }
    }

    class CTRVideoHandler : public IVideoHandler
    {
        CTRPlatform* plat;

        unique_ptr<Timer> fpsTimer;
        int fpsNumFrames;

        public:
            CTRVideoHandler(CTRPlatform* plat) : plat(plat)
            {
                fpsTimer.reset(g_sys->CreateTimer());
                fpsNumFrames = 0;
            }

            virtual void BeginFrame() override
            {
                if (!fpsTimer->IsStarted())
                {
                    fpsTimer->Start();
                    fpsNumFrames = 0;
                }
                else
                {
                    fpsNumFrames++;
                }
            }

            virtual void BeginDrawFrame() override
            {
                ctrr->BeginFrame();
            }

            virtual bool CaptureFrame(Pixmap_t* pm_out) override
            {
                return ctrr->CaptureFrame(pm_out);
            }

            virtual void EndFrame(int ticksElapsed) override
            {
                ctrr->EndFrame(ticksElapsed);

                plat->Swap();
            }

            virtual Int2 GetDefaultViewportSize() override
            {
                return Int2(400, 240);
            }

            virtual bool MoveWindow(Int2 vec) override
            {
                return false;
            }

            virtual void ReceiveEvents() override
            {
                MessageQueue* msgQueue = plat->GetEventMessageQueue();

                aptMainLoop();      // TODO: check retval

                //gspWaitForVBlank();
                hidScanInput();

                KeyEvents(msgQueue, hidKeysDown(), VKEY_PRESSED);
                KeyEvents(msgQueue, hidKeysUp(), VKEY_RELEASED);

                if (hidKeysDown() & KEY_X)
                    msgQueue->Post(EVENT_WINDOW_CLOSE);
            }

            virtual const char* JoystickDevToName(int dev)
            {
                if (dev == 0)
                    return controllerName;
                else
                    return nullptr;
            }

            virtual int JoystickNameToDev(const char* name)
            {
                if (strcmp(name, controllerName) == 0)
                    return 0;
                else
                    return -1;
            }
    };

    CTRPlatform::CTRPlatform(ISystem* sys, MessageQueue* eventMsgQueue) : eventMsgQueue(eventMsgQueue)
    {
        g_sys = sys;
    }

    void CTRPlatform::Init()
    {
        g_sys->SetVideoHandler(unique_ptr<CTRVideoHandler>(new CTRVideoHandler(this)));
    }

    void CTRPlatform::Shutdown()
    {
        if (ctrr != nullptr)
        {
            //ctrr->Shutdown();
            ctrr.reset();
        }
    }

    IRenderer* CTRPlatform::InitRendering()
    {
        ctrr.reset();

        ctrr.reset(new CTRRenderer());
        zombie_assert(ctrr->Init());
        return ctrr.get();
    }

    void CTRPlatform::Swap()
    {
        //gfxFlushBuffers();
        //gfxSwapBuffers();
    }
}
