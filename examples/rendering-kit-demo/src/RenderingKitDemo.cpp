
#if defined(_DEBUG) && defined(_MSC_VER)
#include <crtdbg.h>
#endif

#include "RenderingKitDemo.hpp"

#include <framework/app.hpp>
#include <framework/errorbuffer.hpp>
#include <framework/filesystem.hpp>
#include <framework/framework.hpp>
#include <framework/modulehandler.hpp>
#include <framework/resourcemanager.hpp>
#include <framework/shader_preprocessor.hpp>
#include <framework/videohandler.hpp>
//#include <framework/vp8videocapture.hpp>

#include <littl/File.hpp>
#include <littl/Thread.hpp>

#include <cstdarg>

namespace client
{
    ISystem* g_sys;
    ErrorBuffer_t* g_eb;
    MessageQueue* g_msgQueue;
    EntityWorld* g_world;

    IRenderingKit* irk;
    IRenderingManager* irm;
    IWindowManager* iwm;

    Globals g;

    //static Int2 r_pixelRes;

    class RKVideoHandler : public IVideoHandler
    {
        public:
            virtual void BeginFrame()
            {
                irm->BeginFrame();
            }

            virtual void BeginDrawFrame()
            {
                //irm->BeginDrawFrame();
            }

            virtual void EndFrame(int ticksElapsed)
            {
                irm->EndFrame(ticksElapsed);
            }

            virtual Int2 GetDefaultViewportSize()
            {
                // FIXME
                return Int2(1280, 720);
                //return r_pixelRes;
            }

            virtual bool MoveWindow(Int2 vec)
            {
                return iwm->MoveWindow(vec);
            }

            virtual void ReceiveEvents()
            {
                iwm->ReceiveEvents(g_msgQueue);
            }

			virtual bool CaptureFrame(Pixmap_t* pm_out)
			{
				zombie_assert(false);
				return false;
			}
    };

    static bool SysInit(int argc, char** argv)
    {
		g_sys = CreateSystem();

        if (!g_sys->Init(g_eb, 0))
            return false;

        IFSUnion* fsUnion = g_sys->GetFSUnion();
        fsUnion->AddFileSystem(g_sys->CreateStdFileSystem(".", kFSAccessStat | kFSAccessRead | kFSAccessWrite | kFSAccessCreateFile), 200);

        if (!g_sys->Startup())
            return false;

		auto var = g_sys->GetVarSystem();

        var->SetVariable( "cl_tickrate", "60", 0 );

        var->SetVariable("appName", "Rendering Kit Demo", 0);

        //g_sys->ExecList1( "boot.txt", true );
        //g_sys->ExecList1( "cfg_client.txt", true );
        //g_sys->ExecList1( "cfg_client_" ZOMBIE_PLATFORM ".txt", false );

        g_sys->ParseArgs1(argc, argv);

        //g_sys->SetTickRate(Var::GetInt("cl_tickrate", true));

        // Initialize all handlers here
        //g_sys->InitModuleHandler();

        g_msgQueue = MessageQueue::Create();

        g.res = g_sys->CreateResourceManager("");

        return true;
    }

    static void SysShutdown()
    {
        delete g_msgQueue;
        g_sys->Shutdown();
    }

    static bool PlatformInit()
    {
        IModuleHandler* imh = g_sys->GetModuleHandler(true);
        //irk = imh->CreateInterface<IRenderingKit>("RenderingKit");
		irk = TryCreateRenderingKit(imh);

        if (irk == nullptr)
            return false;

        if (!irk->Init(g_sys, g_eb, nullptr))
            return false;
            
        return true;
    }

    static void PlatformShutdown()
    {
        // Needs to be released before RenderingManager!
        delete g.res;

        delete irk;
    }

    static bool VideoInit()
    {
        iwm = irk->GetWindowManager();

        iwm->LoadDefaultSettings(nullptr);
        if (!iwm->ResetVideoOutput())
            return false;

        irm = irk->GetRenderingManager();
        irm->RegisterResourceProviders(g.res);

        return true;
    }

    ClientEntryPoint
    {
#if defined(_DEBUG) && defined(_MSC_VER)
        _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF );
        //_CrtSetBreakAlloc();
#endif

        ErrorBuffer::Create(g_eb);

        g_msgQueue = nullptr;
        g_world = nullptr;

        irk = nullptr;
        irm = nullptr;
        iwm = nullptr;

        g.res = nullptr;

        //try
        {
            if (!SysInit(argc, argv) || !PlatformInit() || !VideoInit())
                g_sys->DisplayError(g_eb, true);
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

                g_sys->SetVideoHandler(std::make_unique<RKVideoHandler>());

                g_sys->ChangeScene(std::make_shared<RenderingKitDemoScene>());
                g_sys->RunMainLoop();
                g_sys->ReleaseScene();
            }
        }
        /*catch (const AppError& err)
        {
            g_sys->DisplayError(err);
            g_sys->ReleaseScene();
        }
        catch (const zfw::SysException& ex)
        {
            g_sys->DisplayError(ex, ERR_DISPLAY_FATAL);
            g_sys->ReleaseScene();
        }*/

        PlatformShutdown();
        SysShutdown();

        ErrorBuffer::Release(g_eb);

#ifdef ZOMBIE_WINNT
        //getchar();
#endif

        return 0;
    }
}
