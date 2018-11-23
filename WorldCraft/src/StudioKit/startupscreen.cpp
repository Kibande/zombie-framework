
#include <StudioKit/startupscreen.hpp>

#include <RenderingKit/utility/BasicPainter.hpp>

#include <framework/colorconstants.hpp>
#include <framework/engine.hpp>
#include <framework/errorcheck.hpp>
#include <framework/resourcemanager.hpp>

#include <littl/Directory.hpp>
#include <littl/Thread.hpp>

#define STARTUP_PREFIX "StudioKit/startup/"

#undef DrawText

namespace StudioKit
{
    using namespace li;
    using namespace RenderingKit;

    struct StartupState
    {
        bool done;
        const char *text;
        int progress, progress_max;

        IStartupTask* currentTask;
    };

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    class StartupWorkerThread : public Thread, public IStartupTaskProgressListener
    {
        public:
            StartupWorkerThread(volatile StartupState *status) : status(status) {}
        
            virtual void SetProgress(int points) override;
            virtual void SetStatusMessage(const char* message) override;

        protected:
            virtual void run() override;

        private:
            List<unique_ptr<IStartupTask>> tasks;
        
            volatile StartupState* status;
        
            int progress_initial;

            friend class StartupScreen;
    };
    
    class StartupScreen : public IStartupScreen
    {
        IEngine* sys;
        ErrorBuffer_t* eb;
        IRenderingManager* rm;

        unique_ptr<IResourceManager> res;

        StartupState state;
        unique_ptr<StartupWorkerThread> worker;
        shared_ptr<IScene> nextScene;

        BasicPainter2D<> bp, bp2;
        shared_ptr<IShader> shader;
        shared_ptr<IFontFace> logofont, bootfont;
        Paragraph* logoP;

        int frame, numFrames;

        public:
            StartupScreen();
        
            virtual bool Init(IEngine* sys, RenderingKit::IRenderingManager* rm, shared_ptr<IScene> nextScene) override;

            virtual bool Init() override;
            virtual void Shutdown() override;

            virtual bool AcquireResources() override;
            virtual void DropResources() override;

            virtual void DrawScene() override;
            virtual void OnFrame( double delta ) override;

            void PreInit();
            void AddTask(IStartupTask* task, bool inOrder, bool threaded);
    };

    // ====================================================================== //
    //  class StartupWorkerThread
    // ====================================================================== //

    void StartupWorkerThread::run()
    {
        SetStatusMessage("analyzing...");
        
        status->progress_max = 0;
        
        iterate2 (task, tasks)
        {
            task->Init(this);
            status->progress_max += task->Analyze();
        }
        
        iterate2 (task, tasks)
        {
            status->currentTask = (*task).get();

            progress_initial = status->progress;
            
            task->Start();
        }
        
        SetStatusMessage("loading user interface...");
        status->done = true;
    }
    
    void StartupWorkerThread::SetProgress(int points)
    {
        status->progress = progress_initial + points;
    }
    
    void StartupWorkerThread::SetStatusMessage(const char* message)
    {
        status->text = message;
    }
    
    // ====================================================================== //
    //  class StartupScreen
    // ====================================================================== //

    IStartupScreen* CreateStartupScreen()
    {
        return new StartupScreen();
    }

    StartupScreen::StartupScreen()
    {
        nextScene = nullptr;
        res = nullptr;

        shader = nullptr;

        logofont = nullptr;
        bootfont = nullptr;
        logoP = nullptr;
        
        worker = nullptr;
    }

    bool StartupScreen::Init(IEngine* sys, RenderingKit::IRenderingManager* rm, shared_ptr<IScene> nextScene)
    {
        SetEssentials(sys->GetEssentials());

        this->sys = sys;
        this->eb = GetErrorBuffer();
        this->rm = rm;
        this->nextScene = nextScene;

        res.reset(sys->CreateResourceManager("StartupScreen::res"));

        worker.reset(new StartupWorkerThread(&state));

        return true;
    }

    bool StartupScreen::Init()
    {
        state.done = false;
        state.text = nullptr;
        state.progress = 0;
        state.progress_max = 0;

        state.currentTask = nullptr;

        frame = 0;
        numFrames = 3;

        if (!AcquireResources())
            return false;

        if (!bp.Init(rm) || !bp2.InitWithShader(rm, shader))
            return false;

        worker->start();
        return true;
    }

    void StartupScreen::Shutdown()
    {
        DropResources();

        worker.reset();
        nextScene.reset();
        res.reset();
    }
    
    bool StartupScreen::AcquireResources()
    {
        rm->RegisterResourceProviders(res.get());

        ErrorCheck(shader = res->GetResourceByPath<IShader>(STARTUP_PREFIX "startup", RESOURCE_REQUIRED, 0));
        ErrorCheck(logofont = res->GetResource<IFontFace>("path=" STARTUP_PREFIX "DejaVuSans.ttf,size=52", RESOURCE_REQUIRED, 0));
        ErrorCheck(bootfont = res->GetResource<IFontFace>("path=" STARTUP_PREFIX "DejaVuSans.ttf,size=24", RESOURCE_REQUIRED, 0));

        logoP = logofont->CreateParagraph();
        logofont->LayoutParagraph(logoP, "zombie framework", RGBA_GREY(0.2f), ALIGN_HCENTER | ALIGN_BOTTOM);

        frame = 0;
        numFrames = 3;

        return true;
    }

    void StartupScreen::DropResources()
    {
        if (logofont)
            logofont->ReleaseParagraph(logoP);

        logofont.reset();
        bootfont.reset();

        shader.reset();
    }

    void StartupScreen::AddTask(IStartupTask* task, bool inOrder, bool threaded)
    {
        ZFW_ASSERT(inOrder == false)
        ZFW_ASSERT(threaded == true)
        
        worker->tasks.add(unique_ptr<IStartupTask>(task));
    }

    void StartupScreen::DrawScene()
    {
        const Int2 viewport = rm->GetViewportSize();

        rm->SetProjectionOrthoScreenSpace(-1.0f, 1.0f);
        bp2.DrawFilledRectangle(Short2(), Short2(viewport), RGBA_WHITE);

        logofont->DrawParagraph(logoP, Float3(viewport.x / 2, viewport.y / 2, 0.0f), Float2());

        StartupState copy = state;

        if (copy.text != nullptr)
            bootfont->DrawText(copy.text, RGBA_GREY(0.2f), ALIGN_HCENTER | ALIGN_VCENTER, Float3(viewport.x / 2, viewport.y * 2 / 3, 0.0f), Float2());

        if (copy.progress_max > 0)
        {
            const int progress_width = 200;
            const int progress_height = 6;

            const int x0 = viewport.x / 2 - progress_width / 2;
            const int y0 = viewport.y * 2 / 3 + 30;

            bp.DrawFilledRectangle(Short2(x0, y0), Short2(progress_width, progress_height), Byte4(COLOUR_GREY(0.2f, 0.4f) * 255.0f));
            bp.DrawFilledRectangle(Short2(x0, y0), Short2(progress_width * copy.progress / copy.progress_max, progress_height), Byte4(COLOUR_GREY(0.2f) * 255.0f));
        }

        if (copy.currentTask != nullptr)
            copy.currentTask->OnMainThreadDraw();
    }

    void StartupScreen::OnFrame( double delta )
    {
        if (state.done && ++frame == numFrames)
        {
            sys->ChangeScene(move(nextScene));
            return;
        }
    }
}
