
#include "../RenderingKitImpl.hpp"

#include <framework/errorbuffer.hpp>
#include <framework/event.hpp>
#include <framework/messagequeue.hpp>
#include <framework/system.hpp>
#include <framework/varsystem.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#ifdef ZOMBIE_WINNT
#include <windows.h>
#include <SDL2/SDL_syswm.h>
#endif

namespace RenderingKit
{
    using namespace zfw;

    static void PushUnicodeInputEvent(MessageQueue* msgQueue, int16_t type, int16_t device, int16_t key, int16_t subkey, int flags, int value, UnicodeChar c)
    {
        MessageConstruction<EventUnicodeInput> msg(msgQueue);
        msg->input.vkey.type = type;
        msg->input.vkey.device = device;
        msg->input.vkey.key = key;
        msg->input.vkey.subkey = subkey;
        msg->input.flags = flags;
        msg->input.value = value;
        msg->c = c;
    }

    static void PushVkeyEvent(MessageQueue* msgQueue, int16_t type, int16_t device, int16_t key, int16_t subkey, int flags, int value)
    {
        MessageConstruction<EventVkey> msg(msgQueue);
        msg->input.vkey.type = type;
        msg->input.vkey.device = device;
        msg->input.vkey.key = key;
        msg->input.vkey.subkey = subkey;
        msg->input.flags = flags;
        msg->input.value = value;
    }

    class SDLWindowManager : public IWindowManagerBackend
    {
        zfw::ErrorBuffer_t* eb;
        RenderingKit* rk;

        Int2 r_displayres, r_mousepos;
        bool r_fullscreen;
        int r_multisample;
        int r_swapcontrol;

        SDL_Window* displayWindow;

        //IVideoCapture* cap;

#ifdef ZOMBIE_WINNT
        HWND nativeWindow;
#endif

        public:
            SDLWindowManager(zfw::ErrorBuffer_t* eb, RenderingKit* rk);
            ~SDLWindowManager();
            virtual void Release() override { delete this; }

            virtual bool Init() override;

            virtual bool LoadDefaultSettings(IConfig* conf) override;
            virtual bool ResetVideoOutput() override;

            virtual Int2 GetWindowSize() override { return r_displayres; }
            virtual bool MoveWindow(Int2 vec) override;
            virtual void ReceiveEvents(zfw::MessageQueue* msgQueue) override;

            virtual void Present() override;
            virtual void SetWindowCaption(const char* caption) override;
    };

    IWindowManagerBackend* CreateSDLWindowManager(zfw::ErrorBuffer_t* eb, RenderingKit* rk)
    {
        return new SDLWindowManager(eb, rk);
    }

    SDLWindowManager::SDLWindowManager(ErrorBuffer_t* eb, RenderingKit* rk)
    {
        this->eb = eb;
        this->rk = rk;

#ifdef ZOMBIE_WINNT
        nativeWindow = nullptr;
#endif
    }

    SDLWindowManager::~SDLWindowManager()
    {
        SDL_Quit();
    }

    bool SDLWindowManager::Init()
    {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE) != 0)
            return ErrorBuffer::SetError2(eb, EX_BACKEND_ERR, 2,
                    "desc", "Failed to initialize SDL.",
                    "function", li_functionName),
                    false;

        return true;
    }

    bool SDLWindowManager::LoadDefaultSettings(IConfig* conf)
    {
        SDL_DisplayMode current;

        if (SDL_GetCurrentDisplayMode(0, &current) == 0)
            r_displayres = Int2(current.w - 200, current.h - 200);
        else
            r_displayres = Int2(1280, 720);

        r_fullscreen = false;
        r_multisample = 0;
        r_swapcontrol = 0;

        return true;
    }

    bool SDLWindowManager::MoveWindow(Int2 vec)
    {
#ifdef ZOMBIE_WINNT
        RECT rect;
        GetWindowRect(nativeWindow, &rect);
        SetWindowPos(nativeWindow, 0, rect.left + vec.x, rect.top + vec.y, 0, 0, SWP_NOSIZE);
        return true;
#else
#warning zfw::Video::MoveWindow not implemented
        return false;
#endif
    }

    void SDLWindowManager::ReceiveEvents(zfw::MessageQueue* msgQueue)
    {
        static const int event_key_which = 0;

        SDL_Event event;

        //auto rm = master->GetRenderingManagerImpl();
        //rm->R_OnBeginReceiveEvents();

        while ( SDL_PollEvent( &event ) )
        {
            switch ( event.type )
            {
                /*case SDL_JOYAXISMOTION:
                    //printf( "[%i] SDL jaxis motion: axis %i = %i\n", event.jaxis.which, event.jaxis.axis, event.jaxis.value );
                    Event::PushVkeyEvent(IN_ANALOG, event.jaxis.which, event.jaxis.axis, (event.jaxis.value < 0) ? 0 : 1, VKEY_VALUECHANGE, abs(event.jaxis.value));
                    break;

                case SDL_JOYBUTTONDOWN:
                case SDL_JOYBUTTONUP:
                    //printf( "[%i] SDL jbutton down: %i\n", event.jbutton.which, event.jbutton.button );
                    Event::PushVkeyEvent(IN_JOYBTN, event.jbutton.which, event.jbutton.button, 0,
                            (event.jbutton.type == SDL_JOYBUTTONDOWN) ? (VKEY_PRESSED|VKEY_TRIG) : VKEY_RELEASED, 0);
                    break;*/

                /*case SDL_JOYHATMOTION:
                {
                    //printf( "[%i] SDL jhat(%i) motion: %i\n", event.jhat.which, event.jhat.hat, event.jhat.value );
                    Joy_t& joy = joysticks[event.jhat.which];

                    if ( joy.hatStates[event.jhat.hat] != 0 )
                        Event::PushVkeyEvent(IN_JOYHAT, event.jhat.which, event.jhat.hat, joy.hatStates[event.jhat.hat], VKEY_RELEASED, 0 );

                    joy.hatStates[event.jhat.hat] = GetHatValue(event.jhat.value);

                    if ( event.jhat.value != 0 )
                        Event::PushVkeyEvent(IN_JOYHAT, event.jhat.which, event.jhat.hat, joy.hatStates[event.jhat.hat], VKEY_PRESSED|VKEY_TRIG, 0 );
                    break;
                }*/

                case SDL_KEYDOWN:
                case SDL_KEYUP:
                {
                    /*if (event.key.keysym.sym == SDLK_LALT || event.key.keysym.sym == SDLK_RALT)
                        altDown = (event.key.type == SDL_KEYDOWN);
                    else if (event.key.keysym.sym == SDLK_LCTRL || event.key.keysym.sym == SDLK_RCTRL)
                        ctrlDown = (event.key.type == SDL_KEYDOWN);
                    else if (event.key.keysym.sym == SDLK_LSHIFT || event.key.keysym.sym == SDLK_RSHIFT)
                        shiftDown = (event.key.type == SDL_KEYDOWN);*/

                    PushVkeyEvent(msgQueue, VKEY_KEY, event_key_which, (uint16_t) event.key.keysym.sym, event.key.keysym.scancode,
                            (event.key.type == SDL_KEYDOWN) ? VKEY_PRESSED : VKEY_RELEASED, 0);

                    /*if (event.key.keysym.unicode != 0 && event.type == SDL_KEYDOWN)
                    {
                        PushUnicodeInputEvent(msgQueue, VKEY_KEY, event_key_which, (uint16_t) event.key.keysym.sym, event.key.keysym.scancode,
                            (event.key.type == SDL_KEYDOWN) ? VKEY_PRESSED : VKEY_RELEASED, 0, event.key.keysym.unicode);
                    }*/

                    //if (event.key.type == SDL_KEYDOWN && event.key.keysym.unicode > 0)
                    //    Event::PushUnicodeInEvent(event.key.keysym.unicode);
                    break;
                }

                case SDL_MOUSEBUTTONUP:
                case SDL_MOUSEBUTTONDOWN:
                {
                    //int button = -1;
                    int flags = (event.type == SDL_MOUSEBUTTONDOWN) ? VKEY_PRESSED : VKEY_RELEASED;
                    int value = 0;

                    if ( event.button.button == SDL_BUTTON_LEFT )
                        PushVkeyEvent(msgQueue, VKEY_MOUSEBTN, event.button.which, MOUSEBTN_LEFT, 0, flags, value);
                    else if ( event.button.button == SDL_BUTTON_RIGHT )
                        PushVkeyEvent(msgQueue, VKEY_MOUSEBTN, event.button.which, MOUSEBTN_RIGHT, 0, flags, value);
                    else if ( event.button.button == SDL_BUTTON_MIDDLE )
                        PushVkeyEvent(msgQueue, VKEY_MOUSEBTN, event.button.which, MOUSEBTN_MIDDLE, 0, flags, value);

                    /*StormGraph::Key::State state = ( event.type == SDL_MOUSEBUTTONDOWN ) ? StormGraph::Key::pressed : StormGraph::Key::released;

                    if ( event.button.button == SDL_BUTTON_LEFT )
                    {
                        eventListener->onKeyState( Key::anyMouse, state, 0 );
                        eventListener->onKeyState( Key::leftMouse, state, 0 );
                    }
                    else if ( event.button.button == SDL_BUTTON_RIGHT )
                    {
                        eventListener->onKeyState( Key::anyMouse, state, 0 );
                        eventListener->onKeyState( Key::rightMouse, state, 0 );
                    }*/
                    break;
                }

                case SDL_MOUSEWHEEL:
                {
                    int flags = VKEY_PRESSED;
                    int value = abs(event.wheel.y);

                    if ( event.wheel.y < 0 )
                        PushVkeyEvent(msgQueue, VKEY_MOUSEBTN, event.button.which, MOUSEBTN_WHEEL_DOWN, 0, flags, value);
                    else if ( event.wheel.y > 0 )
                        PushVkeyEvent(msgQueue, VKEY_MOUSEBTN, event.button.which, MOUSEBTN_WHEEL_UP, 0, flags, value);

                    break;
                }

                case SDL_MOUSEMOTION:
                {
                    r_mousepos = Int2(event.motion.x, event.motion.y);

                    MessageConstruction<EventMouseMove> msg(msgQueue);
                    msg->x = r_mousepos.x;
                    msg->y = r_mousepos.y;
                    break;
                }

                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                    {
                        r_displayres = Int2(event.window.data1, event.window.data2);
                        rk->GetRenderingManagerBackend()->OnWindowResized(r_displayres);

                        MessageConstruction<EventWindowResize> msg(msgQueue);
                        msg->width = r_displayres.x;
                        msg->height = r_displayres.y;
                    }
                    break;

                case SDL_QUIT:
                    msgQueue->Post(EVENT_WINDOW_CLOSE);
                    break;
            }
        }

        //rm->R_OnEndReceiveEvents();
    }

    void SDLWindowManager::Present()
    {
        SDL_GL_SwapWindow(displayWindow);
    }

    bool SDLWindowManager::ResetVideoOutput()
    {
        //const int bit_depth = 32;

        auto ivs = rk->GetSys()->GetVarSystem();

        // TODO: Shut down all existing rendering etc etc

		// Note: this isn't drivern by RENDERING_KIT_USING_OPENGL_ES, because that doesn't actually
		// use OpenGL ES -- just a functional subset of standard GL.
#ifdef ZOMBIE_EMSCRIPTEN
		//SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif

        SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

        /*SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

        if (r_multisample > 0)
        {
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
            SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, r_multisample);
        }

        if (r_swapcontrol > 0)
            SDL_GL_SetSwapInterval(1); */

        //rk->GetSys()->Printf(kLogInfo, "Rendering Kit: resetting video output to %ix%i @ %i bpp", r_displayres.x, r_displayres.y, bit_depth);

        int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

        if (r_fullscreen == 1)
            flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

        if (ivs->GetVariableOrDefault<bool>("r_noframe", false))
            flags |= SDL_WINDOW_BORDERLESS;

        if (ivs->GetVariableOrDefault<bool>("r_grabinput", false))
            flags |= SDL_WINDOW_INPUT_GRABBED;

        displayWindow = SDL_CreateWindow("zombie framework", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                r_displayres.x, r_displayres.y, flags);

        if (displayWindow == nullptr)
        {
            return ErrorBuffer::SetError(eb, EX_BACKEND_ERR, "desc", "Failed to set video mode.",
                    "backend_error", SDL_GetError(),
                    nullptr),
                    false;
        }
        
#ifdef ZOMBIE_WINNT
        SDL_SysWMinfo wmi;
        SDL_VERSION(&wmi.version);

        if (SDL_GetWindowWMInfo(displayWindow, &wmi) && wmi.subsystem == SDL_SYSWM_WINDOWS)
            nativeWindow = wmi.info.win.window;
        else
            nativeWindow = nullptr;
#endif

        SDL_GL_CreateContext(displayWindow);
        ZFW_ASSERT(glGetError() == GL_NO_ERROR);

        glewExperimental = GL_TRUE;
        GLenum rc = glewInit();

        if (rc != GLEW_OK)
        {
            return ErrorBuffer::SetError(eb, EX_BACKEND_ERR, "desc", "Failed to initialize OpenGL.",
                    "backend_error", glewGetErrorString(rc),
                    nullptr),
                    false;
        }

        if (!GLEW_VERSION_2_0)
        {
            return ErrorBuffer::SetError(eb, EX_BACKEND_ERR, "desc", "OpenGL 2.0 is required.",
                    nullptr),
                    false;
        }

        // May get mis-set by glewInit
        glGetError();

        //if (cap != nullptr)
        //    cap->StartCapture(pixel_resolution, 60);

        auto rm = rk->GetRenderingManagerBackend();
        //rMgr->SetVideoCapture(cap);

        if (!rm->Startup())
            return false;

        return true;
    }

    void SDLWindowManager::SetWindowCaption(const char* caption)
    {
        SDL_SetWindowTitle(displayWindow, caption);
    }
}
