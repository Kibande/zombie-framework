
#if defined(_DEBUG) && defined(_MSC_VER)
#include <crtdbg.h>
#endif

#include <framework/framework.hpp>
#include <framework/app.hpp>
#include <framework/startupscreen.hpp>

#include "client.hpp"

namespace client
{
    /*
    class ScanAssetsStartupTask : public IStartupTask
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
        ZFW_ASSERT(scanPaths.getLength() < INT_MAX)
        
        return (int) scanPaths.getLength() * 100;
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
    }
    */

    static void ParseArgs(int argc, char **argv)
    {
        String line;

        for (int i = 1; i < argc; i++)
        {
            if (argv[i][0] == '+' || argv[i][0] == '-')
            {
                if (!line.isEmpty())
                    Sys::ExecLine(line);

                line = argv[i] + 1;
            }
            else
            {
                line += " ";
                line += argv[i];
            }
        }

        if (!line.isEmpty())
            Sys::ExecLine(line);
    }

    ClientEntryPoint
    {
#if defined(_DEBUG) && defined(_MSC_VER)
        _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF );
        //_CrtSetBreakAlloc();
#endif

        try
        {
            Sys::Init();

            Var::SetStr( "lang", "en" );
            Var::SetStr( "r_windowcaption", "Broken Pixel Technology" );

            Var::SetStr( "net_hostname", "127.0.0.1" );
            Var::SetInt( "net_port", 61012 );

            Sys::ExecList( "boot.txt", true );
            Sys::ExecList( "cfg_client.txt", true );
            Sys::ExecList( "cfg_client_" ZOMBIE_PLATFORM ".txt", false );

            ParseArgs(argc, argv);

            Video::Init();
            Video::Reset();

            bool nostartup = (Var::GetInt("nostartup", false, 0) > 0);

            try
            {
                ZFW_ASSERT(!nostartup)

                //TexturePreviewCache* texPreviewCache = TexturePreviewCache::Create("texpreview.cache", Short2(64, 64), Short2(16, 16));
                //texPreviewCache->Init();

                auto startup = new StartupScreen(new WorldStage());

                startup->PreInit();

                auto task = startup->GetUpdateFontCacheStartupTask(true);
                task->AddFaceList(Sys::OpenInput("media/startup/fontcachelist.txt"));
                task->AddFaceList(Sys::OpenInput("client/fontcachelist.txt"));

                /*auto assetScanTask = new ScanAssetsStartupTask();
                assetScanTask->QueueDirectory("media/models");
                assetScanTask->QueueDirectory("media/props");
                assetScanTask->QueueDirectory("media/texture");
                startup->AddTask(assetScanTask, false, true);*/

                /*auto updateTexPreviewCacheTask = contentkit::UpdateTexturePreviewCacheStartupTask::Create(texPreviewCache);
                updateTexPreviewCacheTask->AddTextureDirectory("media/texture");
                updateTexPreviewCacheTask->AddTextureDirectory("contentkit/brushtex");
                startup->AddTask(updateTexPreviewCacheTask, false, true);*/

                Sys::ChangeScene(startup);
                Sys::MainLoop();
                ReleaseSharedMedia();
            } catch (SysException se) {
                Sys::DisplayError(se, ERR_DISPLAY_FATAL);
            }

            Video::Exit();

            Var::Dump();
            Sys::Exit();
        } catch (SysException se) {
            Sys::DisplayError(se, ERR_DISPLAY_UNCAUGHT);
        }

        return 0;
    }
}
