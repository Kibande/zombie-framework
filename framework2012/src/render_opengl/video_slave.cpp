
#include "render.hpp"
#include "rendering.hpp"

// FIXME: fix Init/Exit for zr::Batch

namespace zfw
{
    namespace render
    {
        METHOD_NOT_IMPLEMENTED(void Video::BeginFrame())
        METHOD_NOT_IMPLEMENTED(void Video::EndFrame())
        METHOD_NOT_IMPLEMENTED(void Video::ReceiveEvents())

        METHOD_NOT_IMPLEMENTED(int Video::GetJoystickDev(const char* name))
        METHOD_NOT_IMPLEMENTED(const char* Video::GetJoystickName(int dev))
        METHOD_NOT_IMPLEMENTED(bool Video::MoveWindow(int x, int y))
        METHOD_NOT_IMPLEMENTED(bool Video::SetWindowTransparency(bool enabled))

        int numDrawCalls, numShaderSwitches;

        Var_t* gl_allowshaders = nullptr;

        static Var_t* in_mousex = nullptr;
        static Var_t* in_mousey = nullptr;

        Var_t* r_viewportw = nullptr;
        Var_t* r_viewporth = nullptr;

        void Video::Exit()
        {
            GLFont::Exit();
        }

        void Video::Init()
        {
            GLFont::Init();
        }

#if 0
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
#endif
        void Video::Reset()
        {
            /*if ( R_DisplayRes != nullptr )
            {
                R_DisplayRes->Unlock();
                R_DisplayRes = nullptr;
            }*/

            // Get Variables
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
            //R_DisplayRes = Var::Lock( "r_displayres" );
            //R_FullScreen = Var::Lock( "r_fullscreen" );

            gl_allowshaders = Var::LockInt( "gl_allowshaders", true, 1 );
            r_viewportw = Var::LockInt( "r_viewportw" );
            r_viewporth = Var::LockInt( "r_viewporth" );
            in_mousex = Var::LockInt( "in_mousex" );
            in_mousey = Var::LockInt( "in_mousey" );

            //R_DisplayRes->SetStr( displayRes );
            //R_FullScreen->SetInt( 0 );

            r_viewportw->basictypes.i = 0;
            r_viewporth->basictypes.i = 0;

            in_mousex->basictypes.i = 0;
            in_mousey->basictypes.i = 0;

            // We deserve some basic set-up here
            glEnable( GL_BLEND );
            R::SetBlendMode( BLEND_NORMAL );

            zr::Renderer::Init();
        }

        void Video::SetViewport(unsigned int w, unsigned int h)
        {
            glViewport( 0, 0, w, h );

            r_viewportw->basictypes.i = w;
            r_viewporth->basictypes.i = h;
        }
    }
}
