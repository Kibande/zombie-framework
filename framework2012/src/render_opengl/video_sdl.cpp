
#include "render.hpp"
#include "rendering.hpp"

#define GL_SGIX_fragment_lighting

#ifndef __linux__
#include <SDL.h>
#include <SDL_opengl.h>
#else
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#endif

#ifdef ZOMBIE_WINNT
#include <windows.h>
#include "SDL_syswm.h"
#endif

#include <superscanf.h>

namespace zfw
{
    namespace render
    {
        struct Joy_t
        {
            SDL_Joystick* joystick;

            Array<int> hatStates;
        };

        static Var_t* R_DisplayRes = nullptr;
        static Var_t* R_FullScreen = nullptr;

        Var_t* gl_allowshaders = nullptr;

        Var_t* r_viewportw = nullptr;
        Var_t* r_viewporth = nullptr;

        Var_t* r_windoww = nullptr;
        Var_t* r_windowh = nullptr;

        static Var_t* in_mousex = nullptr;
        static Var_t* in_mousey = nullptr;

        static Var_t* r_displayfps = nullptr;

        static SDL_Surface* display = nullptr;
        static List<Joy_t> joysticks;

        bool altDown, ctrlDown, shiftDown;

        Timer frameTime;

        int numDrawCalls, numShaderSwitches;

#ifdef ZOMBIE_WINNT
        static HWND nativeWindow = NULL;
#endif

#ifdef ZOMBIE_WINNT
        // from Dwmapi.h, hope this is not illegal ^_^
        // Blur behind data structures
        #define DWM_BB_ENABLE                 0x00000001  // fEnable has been specified
        #define DWM_BB_BLURREGION             0x00000002  // hRgnBlur has been specified
        #define DWM_BB_TRANSITIONONMAXIMIZED  0x00000004  // fTransitionOnMaximized has been specified

        typedef struct _DWM_BLURBEHIND
        {
            DWORD dwFlags;
            BOOL fEnable;
            HRGN hRgnBlur;
            BOOL fTransitionOnMaximized;
        } DWM_BLURBEHIND, *PDWM_BLURBEHIND;

        typedef HRESULT (STDAPICALLTYPE *DwmEnableBlurBehindWindowFunc)(HWND hWnd, const DWM_BLURBEHIND* pBlurBehind);
#endif

        static int GetHatValue( int sdlhat )
        {
            int value = 0;

            if (sdlhat & SDL_HAT_UP)
                value |= HAT_UP;

            if (sdlhat & SDL_HAT_DOWN)
                value |= HAT_DOWN;

            if (sdlhat & SDL_HAT_LEFT)
                value |= HAT_LEFT;

            if (sdlhat & SDL_HAT_RIGHT)
                value |= HAT_RIGHT;

            return value;
        }

        void Video::BeginFrame()
        {
            numDrawCalls = 0;
            numShaderSwitches = 0;

            zr::Renderer::BeginFrame();
        }

        void Video::EnableKeyRepeat( bool enable )
        {
            SDL_EnableKeyRepeat(enable?SDL_DEFAULT_REPEAT_DELAY:0, SDL_DEFAULT_REPEAT_INTERVAL);
        }

        void Video::EndFrame()
        {
            Renderer1::Flush();
            zr::FlushBatch();

            SDL_GL_SwapBuffers();

            const zr::FrameStats *frameStats = zr::Renderer::EndFrame();

            if (r_displayfps->basictypes.i > 0)
            {
                static int counter = 0;

                if ( ++counter == 100 )
                {
                    if ( frameTime.isStarted() )
                    {
                        unsigned fps = ( unsigned )round( 100000000.0 / frameTime.getMicros() );
                        unsigned timePerFrame = ( unsigned )( frameTime.getMicros() / 100000 );

                        SDL_WM_SetCaption( Sys::tmpsprintf(200, "%u FPS [%u ms; %i tri in %i calls; %i B up; %i shader]",
                                fps, timePerFrame, frameStats->polysDrawn, frameStats->batchesDrawn, frameStats->bytesStreamed,
                                numShaderSwitches), NULL );
                    }

                    frameTime.start();

                    counter = 0;
                }
            }
        }

        void Video::Exit()
        {
            zr::Renderer::Exit();

            GLFont::Exit();

            SDL_Quit();
        }

        int Video::GetJoystickDev(const char* name)
        {
            iterate2 (i, joysticks)
            {
                //const Joy_t& joy = i;

                if (strcmp( SDL_JoystickName(i.getIndex()), name ) == 0)
                    return i.getIndex();
            }

            return -1;
        }

        const char* Video::GetJoystickName( int dev )
        {
            return SDL_JoystickName( dev );
        }

        void Video::Init()
        {
            SDL_Init( SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE );
            SDL_EnableUNICODE(1);

            GLFont::Init();

            printf( "Video::Init: detecting joysticks.\n" );

            int numJoysticks = SDL_NumJoysticks();

            for ( int i = 0; i < numJoysticks; i++ )
            {
                printf( "Video::Init: opening joystick '%s'\n", SDL_JoystickName(i) );

                Joy_t joy;

                joy.joystick = SDL_JoystickOpen(i);

                joysticks.add(joy);
            }

            altDown = false;
            ctrlDown = false;
            shiftDown = false;
        }

        bool Video::IsDown(const VkeyState_t& vkey)
        {
            return (vkey.vk.inclass == IN_KEY && vkey.vk.key == SDLK_DOWN);
        }

        bool Video::IsEnter(const VkeyState_t& vkey)
        {
            return (vkey.vk.inclass == IN_KEY && vkey.vk.key == SDLK_RETURN);
        }

        bool Video::IsEsc(const VkeyState_t& vkey)
        {
            return (vkey.vk.inclass == IN_KEY && vkey.vk.key == SDLK_ESCAPE);
        }

        bool Video::IsLeft(const VkeyState_t& vkey)
        {
            return (vkey.vk.inclass == IN_KEY && vkey.vk.key == SDLK_LEFT);
        }

        bool Video::IsRight(const VkeyState_t& vkey)
        {
            return (vkey.vk.inclass == IN_KEY && vkey.vk.key == SDLK_RIGHT);
        }

        bool Video::IsUp(const VkeyState_t& vkey)
        {
            return (vkey.vk.inclass == IN_KEY && vkey.vk.key == SDLK_UP);
        }

        bool Video::MoveWindow(int x, int y)
        {
#ifdef ZOMBIE_WINNT
            RECT rect;
            GetWindowRect(nativeWindow, &rect);
            SetWindowPos(nativeWindow, 0, rect.left + x, rect.top + y, 0, 0, /*SWP_NOZORDER |*/ SWP_NOSIZE /*| SWP_NOACTIVATE*/);
            return true;
#elif defined(ZOMBIE_OSX)
#warning This should be implemented once stable SDL can return native window handle for Cocoa.
            return false;
#else
#warning zfw::Video::MoveWindow not implemented
            return false;
#endif
        }

        void Video::ReceiveEvents()
        {
            SDL_Event event;

            while ( SDL_PollEvent( &event ) )
            {
                switch ( event.type )
                {
                    case SDL_JOYAXISMOTION:
                        //printf( "[%i] SDL jaxis motion: axis %i = %i\n", event.jaxis.which, event.jaxis.axis, event.jaxis.value );
                        Event::PushVkeyEvent(IN_ANALOG, event.jaxis.which, event.jaxis.axis, (event.jaxis.value < 0) ? 0 : 1, VKEY_VALUECHANGE, abs(event.jaxis.value));
                        break;

                    case SDL_JOYBUTTONDOWN:
                    case SDL_JOYBUTTONUP:
                        //printf( "[%i] SDL jbutton down: %i\n", event.jbutton.which, event.jbutton.button );
                        Event::PushVkeyEvent(IN_JOYBTN, event.jbutton.which, event.jbutton.button, 0,
                                (event.jbutton.type == SDL_JOYBUTTONDOWN) ? (VKEY_PRESSED|VKEY_TRIG) : VKEY_RELEASED, 0);
                        break;

                    case SDL_JOYHATMOTION:
                    {
                        //printf( "[%i] SDL jhat(%i) motion: %i\n", event.jhat.which, event.jhat.hat, event.jhat.value );
                        Joy_t& joy = joysticks[event.jhat.which];

                        if ( joy.hatStates[event.jhat.hat] != 0 )
                            Event::PushVkeyEvent(IN_JOYHAT, event.jhat.which, event.jhat.hat, joy.hatStates[event.jhat.hat], VKEY_RELEASED, 0 );

                        joy.hatStates[event.jhat.hat] = GetHatValue(event.jhat.value);

                        if ( event.jhat.value != 0 )
                            Event::PushVkeyEvent(IN_JOYHAT, event.jhat.which, event.jhat.hat, joy.hatStates[event.jhat.hat], VKEY_PRESSED|VKEY_TRIG, 0 );
                        break;
                    }

                    case SDL_KEYDOWN:
                    case SDL_KEYUP: {
                        //printf( "[%i] SDL key down: %i, %i(%c)\n", event.key.which, event.key.keysym.scancode, event.key.keysym.sym, event.key.keysym.sym );

                        if (event.key.keysym.sym == SDLK_LALT || event.key.keysym.sym == SDLK_RALT)
                            altDown = (event.key.type == SDL_KEYDOWN);
                        else if (event.key.keysym.sym == SDLK_LCTRL || event.key.keysym.sym == SDLK_RCTRL)
                            ctrlDown = (event.key.type == SDL_KEYDOWN);
                        else if (event.key.keysym.sym == SDLK_LSHIFT || event.key.keysym.sym == SDLK_RSHIFT)
                            shiftDown = (event.key.type == SDL_KEYDOWN);

#ifndef ZOMBIE_EMSCRIPTEN
                        int which = event.key.which;
#else
                        int which = 0;
#endif

                        Event::PushVkeyEvent(IN_KEY, which, event.key.keysym.sym, event.key.keysym.scancode,
                                (event.key.type == SDL_KEYDOWN) ? (VKEY_PRESSED|VKEY_TRIG) : VKEY_RELEASED, 0);

                        if (event.key.type == SDL_KEYDOWN && event.key.keysym.unicode > 0)
                            Event::PushUnicodeInEvent(event.key.keysym.unicode);
                        break;
                    }

                    case SDL_MOUSEBUTTONUP:
                    case SDL_MOUSEBUTTONDOWN:
                    {
                        //int button = -1;
                        int flags = (event.type == SDL_MOUSEBUTTONDOWN) ? (VKEY_PRESSED) : (VKEY_RELEASED | VKEY_TRIG);
                        int value = 0;

                        if ( event.button.button == SDL_BUTTON_LEFT )
                        {
                            Event::PushVkeyEvent(IN_MOUSEBTN, event.button.which, MOUSE_BTN_LEFT, 0, flags, value);
                            Event::PushMouseEvent(EV_MOUSE_BTN, event.button.x, event.button.y, flags, MOUSE_BTN_LEFT);
                        }
                        else if ( event.button.button == SDL_BUTTON_RIGHT )
                        {
                            Event::PushVkeyEvent(IN_MOUSEBTN, event.button.which, MOUSE_BTN_RIGHT, 0, flags, value);
                            Event::PushMouseEvent(EV_MOUSE_BTN, event.button.x, event.button.y, flags, MOUSE_BTN_RIGHT);
                        }
                        else if ( event.button.button == SDL_BUTTON_MIDDLE )
                        {
                            Event::PushVkeyEvent(IN_MOUSEBTN, event.button.which, MOUSE_BTN_MIDDLE, 0, flags, value);
                            Event::PushMouseEvent(EV_MOUSE_BTN, event.button.x, event.button.y, flags, MOUSE_BTN_MIDDLE);
                        }
                        else if ( event.button.button == SDL_BUTTON_WHEELUP )
                        {
                            Event::PushVkeyEvent(IN_MOUSEBTN, event.button.which, MOUSE_BTN_WHEELUP, 0, flags, value);
                            Event::PushMouseEvent(EV_MOUSE_BTN, event.button.x, event.button.y, flags, MOUSE_BTN_WHEELUP);
                        }
                        else if ( event.button.button == SDL_BUTTON_WHEELDOWN )
                        {
                            Event::PushVkeyEvent(IN_MOUSEBTN, event.button.which, MOUSE_BTN_WHEELDOWN, 0, flags, value);
                            Event::PushMouseEvent(EV_MOUSE_BTN, event.button.x, event.button.y, flags, MOUSE_BTN_WHEELDOWN);
                        }

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

                    case SDL_MOUSEMOTION:
                        Event::PushMouseEvent(EV_MOUSE_MOVE, event.motion.x, event.motion.y, 0, 0);

                        in_mousex->basictypes.i = event.motion.x;
                        in_mousey->basictypes.i = event.motion.y;
                        break;

                    case SDL_VIDEORESIZE:
                        Video::SetViewport(event.resize.w,  event.resize.h);

                        Event::PushViewportResizeEvent(event.resize.w, event.resize.h);

                        r_windoww->basictypes.i = event.resize.w;
                        r_windowh->basictypes.i = event.resize.h;
                        break;

                    case SDL_QUIT:
                        Event::PushVkeyEvent(IN_OTHER, 0, OTHER_CLOSE, 0, VKEY_TRIG, 0);
                        break;
                }
            }
        }

        void Video::Reset()
        {
            //
            //  Release everything
            //
            zr::Renderer::Exit();

            Var::Unlock(R_DisplayRes);

            //
            //  TODO: Check this
            //

            // FIXME: all variables need to be unlocked at some point

            // Get Variables
            String displayRes =         Var::GetStr( "r_setdisplayres", true );
            int MultiSample =           Var::GetInt( "r_setmultisample", false, 0 );
            int NoBorder =              Var::GetInt( "r_setnoborder", false, 0 );
            int Resizable =             Var::GetInt( "r_setresizable", false, 0 );
            int SwapControl =           Var::GetInt( "r_setswapcontrol", false, 0 );
            const char* WindowCaption = Var::GetStr( "r_windowcaption", false );

            // Lock these two early to use with SetVideoMode
            r_windoww = Var::LockInt( "r_windoww" );
            r_windowh = Var::LockInt( "r_windowh" );

            if (ssscanf(displayRes, 0, "%nx%n", VAR_PTRINT(r_windoww), VAR_PTRINT(r_windowh)) != 2)
            {
                Sys::RaiseException( EX_BACKEND_ERR, "Video::Reset", "Invalid display resolution format: '%s'", displayRes.c_str() );
                return;
            }

            if ( WindowCaption != nullptr )
                SDL_WM_SetCaption( WindowCaption, WindowCaption );

#ifndef ZOMBIE_EMSCRIPTEN
            SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, SwapControl );
#endif

            if ( MultiSample > 0 )
            {
                SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
                SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, MultiSample );
            }

            display = SDL_SetVideoMode(VAR_INT(r_windoww), VAR_INT(r_windowh), 32, SDL_OPENGL|(Resizable?SDL_RESIZABLE:0)|(NoBorder?SDL_NOFRAME:0));

            if (display == nullptr)
            {
                Sys::RaiseException( EX_BACKEND_ERR, "Video::Reset", "Video initialization failed: %s", SDL_GetError() );
                return;
            }

#ifdef ZOMBIE_WINNT
            SDL_SysWMinfo wmi;
            SDL_VERSION(&wmi.version);

            if (SDL_GetWMInfo(&wmi))
                nativeWindow = wmi.window;
            else
                nativeWindow = nullptr;
#endif

            GLenum err = glewInit();
            if (GLEW_OK != err)
            {
                Sys::RaiseException( EX_BACKEND_ERR, "Video::Reset", "GLEW Initialization failed: %s", glewGetErrorString(err) );
                return;
            }

            if ( !GLEW_VERSION_2_0 )
            {
                Sys::RaiseException( EX_BACKEND_ERR, "Video::Reset", "OpenGL 2.0 or greater is required");
                return;
            }

            /*bool shadersAvailable = glewIsSupported( "GLEW_ARB_shader_objects GLEW_ARB_fragment_shader GLEW_ARB_vertex_shader" );

            if ( !shadersAvailable )
                printf( "Warning: one or more of ARB_shader_objects, ARB_fragment_shader, ARB_vertex_shader is unavailable; disabling shaders\n" );*/

            // Lock Variables
            R_DisplayRes = Var::Lock( "r_displayres" );
            R_FullScreen = Var::Lock( "r_fullscreen" );
            Var::SetInt("r_noborder", NoBorder);

            gl_allowshaders = Var::LockInt( "gl_allowshaders", true, 1 );
            r_viewportw = Var::LockInt( "r_viewportw" );
            r_viewporth = Var::LockInt( "r_viewporth" );
            in_mousex = Var::LockInt( "in_mousex" );
            in_mousey = Var::LockInt( "in_mousey" );
            r_displayfps = Var::LockInt( "r_displayfps" );

            R_DisplayRes->SetStr( displayRes );
            R_FullScreen->SetInt( 0 );

            SetViewport(VAR_INT(r_windoww), VAR_INT(r_windowh));

            Renderer1::Init();
            zr::Renderer::Init();

            // We deserve some basic set-up here
            glEnable( GL_BLEND );
            R::SetBlendMode( BLEND_NORMAL );
        }

        void Video::SetViewport(unsigned int w, unsigned int h)
        {
            glViewport( 0, 0, w, h );

            r_viewportw->basictypes.i = w;
            r_viewporth->basictypes.i = h;

            zr::currentViewport = Int4(0, 0, w, h);
        }

        bool Video::SetWindowTransparency(bool enabled)
        {
#ifdef ZOMBIE_WINNT
            if (nativeWindow == nullptr)
                return false;

            HMODULE dwmapiDll = LoadLibraryW(L"dwmapi.dll");

            if (dwmapiDll != nullptr)
            {
                DwmEnableBlurBehindWindowFunc DwmEnableBlurBehindWindow;
                DwmEnableBlurBehindWindow = (DwmEnableBlurBehindWindowFunc) GetProcAddress(dwmapiDll, "DwmEnableBlurBehindWindow");

                if (DwmEnableBlurBehindWindow)
                {
                    DWM_BLURBEHIND bb = {0};
                    bb.dwFlags = DWM_BB_ENABLE;
                    bb.fEnable = enabled;
                    bb.hRgnBlur = NULL;
                    DwmEnableBlurBehindWindow(nativeWindow, &bb);
                }

                FreeLibrary(dwmapiDll);
                return true;
            }

            return false;
#elif defined(ZOMBIE_OSX)
            // Not available on OSX/Cocoa.
            return false;
#else
#warning zfw::Video::SetWindowTransparency not implemented
            return false;
#endif
        }
    }
}
