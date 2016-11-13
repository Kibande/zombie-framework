
#include "zombie/zombie.hpp"

#include <littl/Directory.hpp>

#if defined(_DEBUG) && defined(_MSC_VER)
#include <crtdbg.h>
#endif

namespace zombie
{
    extern "C" int main( int argc, char** argv )
    {
#if defined(_DEBUG) && defined(_MSC_VER)
        _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF );
        //_CrtSetBreakAlloc(6601);
#endif

        // set-up
        li::Directory::create("maps");

        try
        {
            printf( "WinZombie: SysInit\n" );
            Sys::Init();

            Var::SetStr( "lang", "en" );
            Var::SetStr( "r_windowcaption", "ZOMBIE FRAMEWORK (C) MINEXEW GAMES 2012" );
            Sys::ExecList( "boot.txt", true );

            printf( "WinZombie: VideoInit\n" );
            Video::Init();
            Video::Reset();

            for (int i = 0; i < MAX_GAMEPADS; i++)
            {
                Sys::ExecList( Sys::tmpsprintf(64, "gamepad_%i.txt", i), false );
                GetGamepadCfg(i, gamepads[i]);
            }

            try
            {
                Sys::ChangeScene( new TitleScene() );
				Sys::MainLoop();
                SharedReleaseMedia();
            } catch ( SysException se ) {
                Sys::DisplayError( se, ERR_DISPLAY_FATAL );
            }

            printf( "WinZombie: Exit\n" );
            Video::Exit();

            Var::Dump();
            Sys::Exit();
        } catch ( SysException se ) {
            Sys::DisplayError( se, ERR_DISPLAY_UNCAUGHT );
        }

        return 0;
    }
}
