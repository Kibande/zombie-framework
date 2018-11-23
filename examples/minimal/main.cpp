#if defined(_DEBUG) && defined(_MSC_VER)
#include <crtdbg.h>
#endif

#include <framework/app.hpp>
#include <framework/engine.hpp>
#include <framework/utility/errorbuffer.hpp>

namespace example
{
    zfw::ErrorBuffer_t* g_eb;
    zfw::IEngine* g_sys;

    static bool SysInit(int argc, char** argv)
    {
        zfw::ErrorBuffer::Create(g_eb);

        g_sys = zfw::CreateEngine();

        if (!g_sys->Init(g_eb, 0))
            return false;

        if (!g_sys->Startup())
            return false;

        return true;
    }

    static void SysShutdown()
    {
        g_sys->Shutdown();

        zfw::ErrorBuffer::Release(g_eb);
    }

    static bool GameInit()
    {
        return true;
    }

    static void GameShutdown()
    {
    }

    static void GameMain(int argc, char** argv)
    {
        if (!SysInit(argc, argv) || !GameInit())
            g_sys->DisplayError(g_eb, true);
        else {
            g_sys->Printf(zfw::kLogAlways, "Hello, world!");
        }

        GameShutdown();
        SysShutdown();
    }

    ClientEntryPoint
    {
#if defined(_DEBUG) && defined(_MSC_VER)
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF);
        //_CrtSetBreakAlloc();
#endif

        GameMain(argc, argv);
        return 0;
    }
}
