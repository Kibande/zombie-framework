
#include "n3d_gl_internal.hpp"

#include <framework/event.hpp>
#include <framework/timer.hpp>
#include <framework/videohandler.hpp>
#include <framework/utility/pixmap.hpp>

namespace n3d
{
    struct Joy_t
    {
        SDL_Joystick* joystick;

        Array<int> hatStates;
    };

    // FIXME: this should be in a class
    static List<Joy_t> joysticks;

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

    static void JoyHatCheck(MessageQueue* msgQueue, const SDL_Event& event, int old_value, int sdlmask, int joybtn_key)
    {
        if ((old_value & sdlmask) && !(event.jhat.value & sdlmask))
            PushVkeyEvent(msgQueue, VKEY_JOYBTN, event.jhat.which, joybtn_key, -1, VKEY_RELEASED, 0);
        else if ((event.jhat.value & sdlmask) && !(old_value & sdlmask))
            PushVkeyEvent(msgQueue, VKEY_JOYBTN, event.jhat.which, joybtn_key, -1, VKEY_PRESSED, 0);
    }

    class SDLVideoHandler : public IVideoHandler
    {
        SDLPlatform* plat;

        unique_ptr<Timer> fpsTimer;
        int fpsNumFrames;

        public:
            SDLVideoHandler(SDLPlatform* plat) : plat(plat)
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

                    if (fpsTimer->GetMicros() >= 100000)
                    {
                        float frameTimeMs = fpsTimer->GetMicros() / 1000.0f / fpsNumFrames;
                        float fps = 1000.0f / (frameTimeMs);

                        char buffer[200];
                        snprintf(buffer, sizeof(buffer), "%g fps @ %g ms/frame", fps, frameTimeMs);

                        SDL_SetWindowTitle(plat->GetSdlWindow(), buffer);

                        fpsTimer->Start();
                        fpsNumFrames = 0;
                    }
                }
            }

            virtual void BeginDrawFrame() override
            {
            }

            virtual bool CaptureFrame(Pixmap_t* pm_out) override
            {
                Pixmap::Initialize(pm_out, gl.renderViewport, PixmapFormat_t::BGR8);

                for (int y = gl.renderViewport.y - 1; y >= 0; y--)
                {
                    uint8_t* line = Pixmap::GetPixelDataForWriting(pm_out) + y * Pixmap::GetBytesPerLine(pm_out->info);

                    glReadPixels(0, gl.renderViewport.y - y - 1, gl.renderViewport.x, 1, GL_BGR, GL_UNSIGNED_BYTE, line);
                }

                return true;
            }

            virtual void EndFrame(int ticksElapsed) override
            {
                plat->Swap();
            }

            virtual Int2 GetDefaultViewportSize() override
            {
                return gl.renderViewport;
            }

            virtual bool MoveWindow(Int2 vec) override
            {
                return false;
            }

            virtual void ReceiveEvents() override
            {
                SDL_Event event;

                //auto rm = master->GetRenderingManagerImpl();
                //rm->R_OnBeginReceiveEvents();

                MessageQueue* msgQueue = plat->GetEventMessageQueue();

                while ( SDL_PollEvent( &event ) )
                {
                    switch ( event.type )
                    {
                        /*case SDL_JOYAXISMOTION:
                            PushVkeyEvent(msgQueue, IN_ANALOG, event.jaxis.which, event.jaxis.axis,
                                    (event.jaxis.value < 0) ? 0 : 1, VKEY_VALUECHANGE, abs(event.jaxis.value));
                            break;*/

                        case SDL_JOYBUTTONDOWN:
                        case SDL_JOYBUTTONUP:
                            PushVkeyEvent(msgQueue, VKEY_JOYBTN, event.jbutton.which, event.jbutton.button, 0,
                                    (event.jbutton.type == SDL_JOYBUTTONDOWN) ? VKEY_PRESSED : VKEY_RELEASED, 0);
                            break;

                        case SDL_JOYHATMOTION:
                        {
                            Joy_t& joy = joysticks[event.jhat.which];

                            JoyHatCheck(msgQueue, event, joy.hatStates[event.jhat.hat], SDL_HAT_UP, JOYBTN_HAT_UP);
                            JoyHatCheck(msgQueue, event, joy.hatStates[event.jhat.hat], SDL_HAT_DOWN, JOYBTN_HAT_DOWN);
                            JoyHatCheck(msgQueue, event, joy.hatStates[event.jhat.hat], SDL_HAT_LEFT, JOYBTN_HAT_LEFT);
                            JoyHatCheck(msgQueue, event, joy.hatStates[event.jhat.hat], SDL_HAT_RIGHT, JOYBTN_HAT_RIGHT);

                            joy.hatStates[event.jhat.hat] = event.jhat.value;
                            break;
                        }

                        case SDL_KEYDOWN:
                        case SDL_KEYUP:
                        {
                            /*if (event.key.keysym.sym == SDLK_LALT || event.key.keysym.sym == SDLK_RALT)
                                altDown = (event.key.type == SDL_KEYDOWN);
                            else if (event.key.keysym.sym == SDLK_LCTRL || event.key.keysym.sym == SDLK_RCTRL)
                                ctrlDown = (event.key.type == SDL_KEYDOWN);
                            else if (event.key.keysym.sym == SDLK_LSHIFT || event.key.keysym.sym == SDLK_RSHIFT)
                                shiftDown = (event.key.type == SDL_KEYDOWN);*/

                            if (!event.key.repeat)
                                PushVkeyEvent(msgQueue, VKEY_KEY, /*event.key.which*/ 0,
                                        (uint16_t) event.key.keysym.sym, event.key.keysym.scancode,
                                        (event.key.type == SDL_KEYDOWN) ? VKEY_PRESSED : VKEY_RELEASED, 0);

                            //if (event.key.type == SDL_KEYDOWN && event.key.keysym.unicode > 0)
                            //    Event::PushUnicodeInEvent(event.key.keysym.unicode);
                            break;
                        }

                        case SDL_MOUSEBUTTONUP:
                        case SDL_MOUSEBUTTONDOWN:
                        {
                            int flags = (event.type == SDL_MOUSEBUTTONDOWN) ? VKEY_PRESSED : VKEY_RELEASED;

                            if ( event.button.button == SDL_BUTTON_LEFT )
                                PushMouseButtonEvent(msgQueue, event.button.x, event.button.y, flags, MOUSEBTN_LEFT);
                            else if ( event.button.button == SDL_BUTTON_RIGHT )
                                PushMouseButtonEvent(msgQueue, event.button.x, event.button.y, flags, MOUSEBTN_RIGHT);
                            else if ( event.button.button == SDL_BUTTON_MIDDLE )
                                PushMouseButtonEvent(msgQueue, event.button.x, event.button.y, flags, MOUSEBTN_MIDDLE);
                            //else if ( event.button.button == SDL_BUTTON_WHEELUP )
                            //    PushMouseButtonEvent(msgQueue, event.button.x, event.button.y, flags, MOUSEBTN_WHEEL_UP);
                            //else if ( event.button.button == SDL_BUTTON_WHEELDOWN )
                            //    PushMouseButtonEvent(msgQueue, event.button.x, event.button.y, flags, MOUSEBTN_WHEEL_DOWN);
                            break;
                        }

                        case SDL_MOUSEMOTION:
                        {
                            MessageConstruction<EventMouseMove> msg(msgQueue);
                            msg->x = event.motion.x;
                            msg->y = event.motion.y;

                            //mousePos = Int2(event.motion.x, event.motion.y);

                            //in_mousex->basictypes.i = event.motion.x;
                            //in_mousey->basictypes.i = event.motion.y;
                            break;
                        }

                        case SDL_MOUSEWHEEL:
                            if ( event.wheel.y < 0 )
                                PushMouseButtonEvent(msgQueue, 0, 0, 0, MOUSEBTN_WHEEL_UP);
                            if ( event.wheel.y > 0 )
                                PushMouseButtonEvent(msgQueue, 0, 0, 0, MOUSEBTN_WHEEL_DOWN);
                            break;

                        case SDL_WINDOWEVENT:
                        {
                            if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                                int w = event.window.data1;
                                int h = event.window.data2;
                                glViewport(0, 0, w,  h);

                                MessageConstruction<EventWindowResize> msg(msgQueue);
                                msg->width = w;
                                msg->height = h;

                                gl.renderWindow = Int2(w, h);
                            }
                            break;
                        }

                        case SDL_QUIT:
                            msgQueue->Post(EVENT_WINDOW_CLOSE);
                            break;
                    }
                }

                //rm->R_OnEndReceiveEvents();
            }

            virtual const char* JoystickDevToName(int dev)
            {
                return SDL_JoystickNameForIndex(dev);
            }

            virtual int JoystickNameToDev(const char* name)
            {
                // FIXME: this is a race condition in SDL 2.0
                int numJoysticks = SDL_NumJoysticks();

                for (int i = 0; i < numJoysticks; i++)
                    if (strcmp(name, SDL_JoystickNameForIndex(i)) == 0)
                        return i;

                return -1;
            }
    };

    SDLPlatform::SDLPlatform(ISystem* sys, MessageQueue* eventMsgQueue) : eventMsgQueue(eventMsgQueue)
    {
        g_sys = sys;
        displayWindow = nullptr;

        multisampleLevel = 0;
        swapControl = 0;
    }

    void SDLPlatform::Init()
    {
        g_sys->Printf(kLogInfo, "Platform: Starting up SDL");

        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE);

        // FIXME: this is a race condition in SDL 2.0
        int numJoysticks = SDL_NumJoysticks();

        for (int i = 0; i < numJoysticks; i++)
        {
            g_sys->Printf(kLogInfo, "Platform: opening joystick '%s'", SDL_JoystickNameForIndex(i));

            SDL_Joystick* joystick = SDL_JoystickOpen(i);

            if (joystick != nullptr)
            {
                auto& joy = joysticks.addEmpty();
                joy.joystick = joystick;
            }
        }

        g_sys->SetVideoHandler(unique_ptr<SDLVideoHandler>(new SDLVideoHandler(this)));
    }

    void SDLPlatform::Shutdown()
    {
        iterate2 (i, joysticks)
            SDL_JoystickClose((*i).joystick);

        if (glr != nullptr)
        {
            glr->Shutdown();
            glr.reset();
        }

        SDL_Quit();
    }

    IRenderer* SDLPlatform::SetGLVideoMode(int w, int h, int flags)
    {
        g_sys->Printf(kLogInfo, "Platform: Setting video mode: %i x %i pixels (flags = %08X)", w, h, flags);

        static const char* cap = "ntile alpha";

        glr.reset();

        if (multisampleLevel > 0)
        {
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, multisampleLevel);
        }

        if (swapControl > 0)
            SDL_GL_SetSwapInterval(swapControl);

        displayWindow = SDL_CreateWindow(cap, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                w, h, SDL_WINDOW_OPENGL | (flags & VIDEO_FULLSCREEN ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0));

        if (displayWindow == nullptr)
        {
            ErrorBuffer::SetError3(EX_OPERATION_FAILED, 1,
                                   "desc", sprintf_4095("SDL_CreateWindow failed: %s", SDL_GetError()));
            return nullptr;
        }

        // TODO: proper error handling

        if (!SDL_GL_CreateContext(displayWindow))
            //Sys::RaiseException(EX_BACKEND_ERR, "Failed to create OpenGL context", SDL_GetError());
            zombie_assert(false);

        ZFW_ASSERT(glGetError() == GL_NO_ERROR);

        gl.renderViewport = Int2(w, h);

        GLenum err = glewInit();
        if (GLEW_OK != err)
            //Sys::RaiseException( EX_BACKEND_ERR, "VideoOutputManagerImpl::ResetVideoOutput", "GLEW Initialization failed: %s", glewGetErrorString(err) );
            zombie_assert(false);

        if ( !GLEW_VERSION_2_0 )
            //Sys::RaiseException( EX_BACKEND_ERR, "VideoOutputManagerImpl::ResetVideoOutput", "OpenGL 2.0 or greater is required");
            zombie_assert(false);

        glr.reset(new GLRenderer());
        ZFW_ASSERT(glr->Init());
        return glr.get();
    }

    void SDLPlatform::Swap()
    {
        SDL_GL_SwapWindow(displayWindow);
    }
}
