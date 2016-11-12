
#include "sandbox.hpp"
#include "mapselectionscene.hpp"

#include <framework/entityworld.hpp>
#include <framework/filesystem.hpp>
#include <framework/framework.hpp>
#include <framework/messagequeue.hpp>
#include <framework/resourcemanager.hpp>
#include <framework/utility/pixmap.hpp>

#include <RenderingKit/utility/RKVideoHandler.hpp>

#include <littl/File.hpp>

namespace sandbox
{
    Globals g;

    ISystem*       g_sys;

    class RKHost : public IRenderingKitHost
    {
        public:
            virtual bool LoadBitmap(const char* path, zfw::Pixmap_t* pm) override
            {
                return Pixmap::LoadFromFile(g_sys, pm, path);
            }
    };

    static RKHost* rkHost;

    static bool SysInit(int argc, char** argv)
    {
        ErrorBuffer::Create(g.eb);

        g_sys = CreateSystem();

        if (!g_sys->Init(g.eb, 0))
            return false;

        IFSUnion* fsUnion = g_sys->GetFSUnion();
        fsUnion->AddFileSystem(g_sys->CreateStdFileSystem(".", kFSAccessStat | kFSAccessRead), 50);

        auto var = g_sys->GetVarSystem();
        g_sys->ParseArgs1(argc, argv);
        var->SetVariable("appName", "Sandbox", 0);

        if (!g_sys->Startup())
            return false;

        // TODO: Replace with proper configuration
        //Sys::ExecList( "boot.txt", true );
        //Sys::ExecList( "cfg_worldcraft.txt", true );
        //Sys::ExecList( "cfg_worldcraft_" ZOMBIE_PLATFORM ".txt", false );

        g.msgQueue.reset(MessageQueue::Create());

        g.res.reset(g_sys->CreateResourceManager("g.res"));

        IModuleHandler* imh = g_sys->GetModuleHandler(true);
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

        ErrorBuffer::Release(g.eb);
    }

    static bool PlatformInit()
    {
        auto imh = g_sys->GetModuleHandler(true);

        auto rk = TryCreateRenderingKit(imh);
        ErrorCheck(rk);
        g.rk.reset(rk);

        rkHost = new RKHost();

        if (!g.rk->Init(g_sys, g.eb, rkHost))
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

    static bool sandboxInit()
    {
        auto ivs = g_sys->GetVarSystem();

        if (!ivs->GetVariableOrDefault("map", (const char*) nullptr))
            g_sys->ChangeScene(IMapSelectionScene::Create());
        else
            g_sys->ChangeScene(CreateSandboxScene());

        return true;
    }

    static void sandboxShutdown()
    {
    }

#ifdef ZOMBIE_WINNT
    BOOL WINAPI HandlerRoutine(DWORD dwCtrlType)
    {
        if (dwCtrlType == CTRL_CLOSE_EVENT)
        {
            g_sys->StopMainLoop();
            return TRUE;
        }
        else
            return FALSE;
    }
#endif

    void sandboxMain(int argc, char** argv)
    {
#ifdef ZOMBIE_WINNT
        // shutdown properly on console window closure
        SetConsoleCtrlHandler(HandlerRoutine, TRUE);
#endif

        g_sys = nullptr;
        g.eb = nullptr;
        g.msgQueue = nullptr;
        g.res = nullptr;
        g.world = nullptr;

        if (!SysInit(argc, argv) || !PlatformInit() || !VideoInit() || !sandboxInit())
            g_sys->DisplayError(g.eb, true);
        else
        {
            g_sys->SetVideoHandler(unique_ptr<IVideoHandler>(new RKVideoHandler(g.rm, g.wm, g.msgQueue)));

            g_sys->RunMainLoop();
            g_sys->ReleaseScene();
        }

        sandboxShutdown();
        PlatformShutdown();
        SysShutdown();
    }
}
