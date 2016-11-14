
#include "worldcraftpro.hpp"
#include "sandbox/sandbox.hpp"

#include <littl/Directory.hpp>

#include <framework/app.hpp>
#include <framework/filesystem.hpp>
#include <framework/framework.hpp>
#include <framework/messagequeue.hpp>
#include <framework/resourcemanager.hpp>

#include <framework/utility/pixmap.hpp>

#include <RenderingKit/utility/RKVideoHandler.hpp>

#include <StudioKit/startupscreen.hpp>

#if defined(_DEBUG) && defined(_MSC_VER)
#include <crtdbg.h>
#endif

namespace worldcraftpro
{
    Globals g;

    ISystem*       g_sys;

    static unique_ptr<ITexturePreviewCache> s_texPreviewCache;
    static shared_ptr<StudioKit::IStartupScreen> s_startup;

    /*class ScanAssetsStartupTask : public IStartupTask
    {
        IStartupTaskProgressListener* progressListener;

        List<String> scanPaths;

        public:
            virtual int Analyze() override;
            virtual void Init(IStartupTaskProgressListener* progressListener) override;
            virtual void Start() override;

            void QueueDirectory(const char* path) { scanPaths.add(path); }
    };

    void ScanAssetsStartupTask::Init(IStartupTaskProgressListener* progressListener)
    {
        this->progressListener = progressListener;
    }

    int ScanAssetsStartupTask::Analyze()
    {
        return scanPaths.getLength() * 100;
    }

    void ScanAssetsStartupTask::Start()
    {
        progressListener->SetStatusMessage("scanning assets...");

        int num_scanned = 0;

        iterate2 (i, scanPaths)
        {
            //pauseThread(200);

            num_scanned++;
            progressListener->SetProgress(num_scanned * 100);
        }
    }*/

    static bool SysInit(int argc, char** argv)
    {
        g_sys = CreateSystem();

        if (!g_sys->Init(g.eb, 0))
            abort();

        IFSUnion* fsUnion = g_sys->GetFSUnion();
        fsUnion->AddFileSystem(g_sys->CreateStdFileSystem(".", kFSAccessStat | kFSAccessRead), 50);

		li::Directory::create("writecache");
		fsUnion->AddFileSystem(g_sys->CreateStdFileSystem("writecache", kFSAccessAll), 100);

        auto var = g_sys->GetVarSystem();
        g_sys->ParseArgs1(argc, argv);
        var->SetVariable("appName", "Worldcraft", 0);

        if (!g_sys->Startup())
            return false;

        // TODO: Replace with proper configuration
        //Sys::ExecList( "boot.txt", true );
        //Sys::ExecList( "cfg_worldcraft.txt", true );
        //Sys::ExecList( "cfg_worldcraft_" ZOMBIE_PLATFORM ".txt", false );

        g.msgQueue.reset(MessageQueue::Create());

        g.res.reset(g_sys->CreateResourceManager("g.res"));

        //IModuleHandler* imh = g.sys->GetModuleHandler(true);
        //g.scr = imh->CreateInterface<ISquirrelScriptHandler>("SquirrelScripting");

        //if (g.scr == nullptr || !g.scr->Init(Sys::GetSys(), g.eb))
        //    return false;

        return true;
    }

    static void SysShutdown()
    {
        //zfw::Release(g.scr);

        g.msgQueue.reset();
        g_sys->Shutdown();
    }

    static bool PlatformInit()
    {
        auto imh = g_sys->GetModuleHandler(true);

        auto rk = TryCreateRenderingKit(imh);
        ErrorCheck(rk);
        g.rk.reset(rk);

        if (!g.rk->Init(g_sys, g.eb, nullptr))
            return false;
            
        return true;
    }

    static void PlatformShutdown()
    {
        // Needs to be released before RenderingManager!
        g.res.reset();

        g.rk.reset();
    }

    static bool VideoInit()
    {
        g.wm = g.rk->GetWindowManager();

        if (!g.wm->LoadDefaultSettings(nullptr)
                || !g.wm->ResetVideoOutput())
            return false;

        g.rm = g.rk->GetRenderingManager();
        g.rm->RegisterResourceProviders(g.res.get());

        return true;
    }

    static bool WorldcraftInit()
    {
        auto imh = g_sys->GetModuleHandler(true);

        auto texPreviewCache = StudioKit::TryCreateTexturePreviewCache(imh);
        ErrorCheck(texPreviewCache);
        s_texPreviewCache.reset(texPreviewCache);
        ErrorCheck(s_texPreviewCache->Init(g_sys, g.rm, "texpreview.cache", Short2(64, 64), Short2(16, 16)));

        auto ss = StudioKit::TryCreateStartupScreen(imh);
        ErrorCheck(ss);
        s_startup.reset(ss);
        ErrorCheck(s_startup->Init(g_sys, g.rm, std::make_shared<worldcraftpro::WorldcraftScene>(s_texPreviewCache.get())));

        /*
        auto task = startup->GetUpdateFontCacheStartupTask(true);
        task->AddFaceList(Sys::OpenInput("media/startup/fontcachelist.txt"));
        task->AddFaceList(Sys::OpenInput("contentkit/worldcraft/fontcachelist.txt"));
        */

        /*auto assetScanTask = new ScanAssetsStartupTask();
        assetScanTask->QueueDirectory("media/models");
        assetScanTask->QueueDirectory("media/props");
        assetScanTask->QueueDirectory("media/texture");
        startup->AddTask(assetScanTask, false, true);*/

        auto updateTexPreviewCacheTask = StudioKit::TryCreateUpdateTexturePreviewCacheStartupTask(imh);
        ErrorCheck(updateTexPreviewCacheTask != nullptr);
        ErrorCheck(updateTexPreviewCacheTask->Init(g_sys, s_texPreviewCache.get()));
        updateTexPreviewCacheTask->AddTextureDirectory("media/texture");
        s_startup->AddTask(updateTexPreviewCacheTask, false, true);

        return true;
    }

    static void WorldcraftShutdown()
    {
        s_texPreviewCache.reset();
    }

    static void WorldcraftMain(int argc, char** argv)
    {
        if (!SysInit(argc, argv) || !PlatformInit() || !VideoInit() || !WorldcraftInit())
            g_sys->DisplayError(g.eb, true);
        else
        {
            /*
#if defined(ZOMBIE_USE_LIBVPX)
            const char* Cap_OutFile =   Var::GetStr( "cap_outfile", false );
            int Cap_FrameRate =         Var::GetInt( "cap_framerate", false, 30 );

            if (Cap_OutFile != nullptr)
                voMgr->SetVideoCapture(VP8VideoCapture::Create(File::open(Cap_OutFile, true), r_pixelRes, Int2(Cap_FrameRate, 1)));
#endif
            */

            g_sys->SetVideoHandler(unique_ptr<RKVideoHandler>(new RKVideoHandler(g.rm, g.wm, g.msgQueue)));

            g_sys->ChangeScene(move(s_startup));
            g_sys->RunMainLoop();
            g_sys->ReleaseScene();
        }

        WorldcraftShutdown();
        PlatformShutdown();
        SysShutdown();
    }

    ClientEntryPoint
    {
#if defined(_DEBUG) && defined(_MSC_VER)
        _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF );
        //_CrtSetBreakAlloc();
#endif

        ErrorBuffer::Create(g.eb);

        g_sys = nullptr;
        g.msgQueue = nullptr;
        g.res = nullptr;
        g.world = nullptr;

        if (argc >= 2 && strcmp(argv[1], "-compile") == 0)
            compileMain(argc - 1, argv + 1);
        else if (argc >= 2 && strcmp(argv[1], "-sandbox") == 0)
            sandbox::sandboxMain(argc - 1, argv + 1);
        else
            WorldcraftMain(argc, argv);

        ErrorBuffer::Release(g.eb);

        //Sys::ClearLog();

#ifdef ZOMBIE_WINNT
        //getchar();
#endif

        return 0;
    }
}
