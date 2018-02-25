
#if defined(_DEBUG) && defined(_MSC_VER)
#include <crtdbg.h>
#endif

#include "gamescreen.hpp"
#include "resourceprovider.hpp"

#ifdef ZOMBIE_CTR
#include "n3d_ctr/n3d_ctr.hpp"
#else
#include "n3d_gl/n3d_gl.hpp"
#endif

#include <framework/app.hpp>
#include <framework/filesystem.hpp>
#include <framework/resourcemanager2.hpp>
#include <framework/system.hpp>
#include <framework/varsystem.hpp>

#ifdef _3DS
#include <framework/ctr/ctr.hpp>
#endif

#include <littl/Directory.hpp>

namespace ntile
{
    // TODO: might want to move these? and use this unit just as the entry point

    ErrorBuffer_t* g_eb;
    ISystem* g_sys;

    unique_ptr<IPlatform> iplat;
    IRenderer* ir = nullptr;
    unique_ptr<MessageQueue> g_msgQueue;
    unique_ptr<IResourceManager2> g_res;

    NanoUI nui;

    Int2 r_pixelRes, r_mousePos;

    UUID_t DRAW_EDITOR_MODE, DRAW_ENT_PICKING;

    ResourceProvider resourceProvider;

#ifndef _3DS
    static SDLPlatform* s_sdlplat;

    static bool SysInit(int argc, char** argv)
    {
        ErrorBuffer::Create(g_eb);

        g_sys = CreateSystem();

        if (!g_sys->Init(g_eb, 0))
            return false;

        auto var = g_sys->GetVarSystem();
        var->SetVariable("appName", "Nanotile", 0);
        var->SetVariable("startmap", "dev_zero", 0);

        //Directory::create("AppData_nanotile");

        IFSUnion* fsUnion = g_sys->GetFSUnion();
        //fsUnion->AddFileSystem(Sys::CreateStdFileSystem("AppData_nanotile",  FS_READ | FS_WRITE | FS_CREATE),   1000);
        fsUnion->AddFileSystem(g_sys->CreateStdFileSystem(".", kFSAccessStat | kFSAccessRead), 200);
        fsUnion->AddFileSystem(g_sys->CreateBlebFileSystem("ntile1.bleb", kFSAccessStat | kFSAccessRead), 200);
		fsUnion->AddFileSystem(g_sys->CreateStdFileSystem("NtileWritable", kFSAccessAll), 500);

        g_sys->ParseArgs1(argc, argv);

        g_sys->CreateDirectoryRecursive("ntile/maps");

        if (!g_sys->Startup())
            return false;

        g_sys->Printf(kLogAlways, "Game version: " APP_VERSION);

        //Var::SetInt( "cl_tickrate", 60 );

        //Var::SetStr("appName", "Nanotile");

        /*Sys::ExecList( "boot.txt", true );
        Sys::ExecList( "cfg_ntile.txt", true );
        Sys::ExecList( "cfg_ntile_" ZOMBIE_PLATFORM ".txt", false );*/

        //Sys::SetTickRate(Var::GetInt("cl_tickrate", true));

        // Initialize all handlers here
        g_sys->GetEntityHandler(true);

        g_msgQueue.reset(MessageQueue::Create());
        g_res.reset(g_sys->CreateResourceManager2());

        return true;
    }

    static void SysShutdown()
    {
        g_msgQueue.reset();
        g_sys->Shutdown();

        ErrorBuffer::Release(g_eb);
    }

    static bool PlatformInit()
    {
        s_sdlplat = new SDLPlatform(g_sys, g_msgQueue.get());
        iplat.reset(s_sdlplat);
        iplat->Init();

        return true;
    }

    static void PlatformShutdown()
    {
        // These need to be released before Renderer as they might hold OpenGL resources
        g_res.reset();

        if (iplat != nullptr)
        {
            iplat->Shutdown();
            iplat.reset();
        }
    }

    static bool VideoInit()
    {
        // FIXME: find out why this is commented out and uncomment it
        /*String displayRes =         Var::GetStr( "r_setdisplayres", true );
        int FullScreen =            Var::GetInt( "r_setfullscreen", false, 0 );
        int MultiSample =           Var::GetInt( "r_setmultisample", false, 0 );
        int SwapControl =           Var::GetInt( "r_setswapcontrol", false, 0 );*/

        String displayRes = "1280x720";
        int MultiSample = 0;
        int SwapControl = 0;

        int FullScreen = 0;

#if defined(ZOMBIE_USE_LIBVPX)
        const char* Cap_OutFile =   Var::GetStr( "cap_outfile", false );
        int Cap_FrameRate =         Var::GetInt( "cap_framerate", false, 30 );
#endif

        if (sscanf(displayRes, "%dx%d", &r_pixelRes.x, &r_pixelRes.y) != 2)
        {
            return ErrorBuffer::SetError3(EX_INVALID_ARGUMENT, 2,
                    "desc", sprintf_255("Invalid display resolution format: '%s'", displayRes.c_str()),
                    "function", li_functionName
                    ), false;
        }

        s_sdlplat->SetMultisampleLevel(MultiSample);
        s_sdlplat->SetSwapControl(SwapControl);

        ir = s_sdlplat->SetGLVideoMode(r_pixelRes.x, r_pixelRes.y, FullScreen ? VIDEO_FULLSCREEN : 0);

        if (!ir)
            return false;

        ir->RegisterResourceProviders(g_res.get());
        resourceProvider.RegisterResourceProviders(g_res.get());

        return true;
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
        if (!SysInit(argc, argv) || !PlatformInit() || !VideoInit() || !GameInit())
            g_sys->DisplayError(g_eb, true);
        else
        {
            g_sys->ChangeScene(std::make_shared<GameScreen>());

            g_sys->RunMainLoop();
            g_sys->ReleaseScene();
        }

        GameShutdown();
        PlatformShutdown();
        SysShutdown();
    }

    ClientEntryPoint
    {
#if defined(_DEBUG) && defined(_MSC_VER)
        _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF);
        //_CrtSetBreakAlloc();
#endif

        // check for tool invocation here
        if (argc >= 2 && strcmp(argv[1], "mkfont") == 0)
            mkfont(argc - 1, argv + 1);
        else
            GameMain(argc, argv);

        return 0;
    }
#else
    static CTRPlatform* s_ctrplat;

    static bool SysInit(int argc, char** argv)
    {
        ErrorBuffer::Create(g_eb);

        g_sys = CreateSystem();

        if (!g_sys->Init(g_eb, 0))
            return false;

        //Directory::create("AppData_nanotile");

        IFSUnion* fsUnion = g_sys->GetFSUnion();
        //fsUnion->AddFileSystem(Sys::CreateStdFileSystem("AppData_nanotile",  FS_READ | FS_WRITE | FS_CREATE),   1000);
        fsUnion->AddFileSystem(g_sys->CreateStdFileSystem("sdmc:/3ds/ntile_data/",
                kFSAccessStat | kFSAccessRead | kFSAccessWrite | kFSAccessCreateFile | kFSAccessCreateDir), 200);
        fsUnion->AddFileSystem(g_sys->CreateStdFileSystem("sdmc:/3ds/ntile/",
                kFSAccessStat | kFSAccessRead | kFSAccessWrite ), 200);     // MediaFile currently requires IOStream
        fsUnion->AddFileSystem(g_sys->CreateBlebFileSystem("sdmc:/3ds/ntile/ntile1.bleb", kFSAccessStat | kFSAccessRead), 200);

        auto var = g_sys->GetVarSystem();
        g_sys->ParseArgs1(argc, argv);
        var->SetVariable("appName", "Nanotile", 0);
        var->SetVariable("startmap", "dev_zero", 0);
        var->SetVariable("map", "dev_zero", 0);

        //g_sys->CreateDirectoryRecursive("ntile/maps");

        if (!g_sys->Startup())
            return false;

        g_sys->Printf(kLogAlways, "Game version: " APP_VERSION);

        //Var::SetInt( "cl_tickrate", 60 );

        //Var::SetStr("appName", "Nanotile");

        /*Sys::ExecList( "boot.txt", true );
        Sys::ExecList( "cfg_ntile.txt", true );
        Sys::ExecList( "cfg_ntile_" ZOMBIE_PLATFORM ".txt", false );*/

        //Sys::SetTickRate(Var::GetInt("cl_tickrate", true));

        // Initialize all handlers here
        g_sys->GetEntityHandler(true);

        g_msgQueue.reset(MessageQueue::Create());
        g_res.reset(g_sys->CreateResourceManager2());

        return true;
    }

    static void SysShutdown()
    {
#if 0
        g_msgQueue.reset();
#endif
        g_sys->Shutdown();

        ErrorBuffer::Release(g_eb);
    }

    static bool PlatformInit()
    {
        s_ctrplat = new CTRPlatform(g_sys, g_msgQueue.get());
        iplat.reset(s_ctrplat);
        iplat->Init();

        return true;
    }

    static void PlatformShutdown()
    {
        // These need to be released before Renderer as they might hog resources
        g_res.reset();

        if (iplat != nullptr)
        {
            iplat->Shutdown();
            iplat.reset();
        }
    }

    static bool VideoInit()
    {
        ir = s_ctrplat->InitRendering();
        zombie_assert(ir != nullptr);

        r_pixelRes = Int2(400, 240);

        ir->RegisterResourceProviders(g_res.get());
        resourceProvider.RegisterResourceProviders(g_res.get());

        return true;
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
        if (!SysInit(argc, argv) || !PlatformInit() || !VideoInit() || !GameInit())
            g_sys->DisplayError(g_eb, true);
        else
        {
            g_sys->ChangeScene(std::make_shared<GameScreen>());

            g_sys->RunMainLoop();
            g_sys->ReleaseScene();
        }

        GameShutdown();
        PlatformShutdown();
        SysShutdown();
    }

    extern "C" int main()
    {
        // Initialize services
        srvInit();
        aptInit();
        hidInit(NULL);
        gfxInit();
        fsInit();
        sdmcInit();

        Directory::create("sdmc:/3ds/ntile_data");

        GameMain(0, nullptr);
        /*
        // Main loop
        while (aptMainLoop())
        {
            gspWaitForVBlank();
            hidScanInput();

            // Your code goes here

            u32 kDown = hidKeysDown();
            if (kDown & KEY_START)
                break; // break in order to return to hbmenu

            // Flush and swap framebuffers
            

            //pauseThread(100);
        }*/

        // Exit services
        sdmcExit();
        fsExit();
        gfxExit();
        hidExit();
        aptExit();
        srvExit();
        return 0;
    }
#endif
}