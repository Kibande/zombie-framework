
#include "party/party.hpp"

#if defined(_DEBUG) && defined(_MSC_VER)
#include <crtdbg.h>
#endif

namespace party
{
    extern "C" int main( int argc, char** argv )
    {
#if defined(_DEBUG) && defined(_MSC_VER)
        _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF );
        //_CrtSetBreakAlloc(4775);
#endif

        try
        {
            Sys::Init();

            Var::SetStr( "lang", "en" );
            Var::SetStr( "r_windowcaption", "Party - zfw Particle Studio" );
			Var::SetInt( "r_batchallocsize", 512 );

            Sys::ExecList( "partyconf.txt", true );

            Video::Init();
            Video::Reset();

            try
            {
                Sys::ChangeScene( new ParticleEdScene() );
                Sys::MainLoop();
                SharedReleaseMedia();
            } catch ( SysException se ) {
                Sys::DisplayError( se, ERR_DISPLAY_FATAL );
            }

            Video::Exit();

            Var::Dump();
            Sys::Exit();
        } catch ( SysException se ) {
            Sys::DisplayError( se, ERR_DISPLAY_UNCAUGHT );
        }

        return 0;
    }
}
