#include "startupscreen.hpp"

#include <littl/Directory.hpp>
#include <littl/Thread.hpp>

#define STARTUP_PREFIX "media/startup/"

namespace zfw
{
    class UpdateFontCacheStartupTask : public IUpdateFontCacheStartupTask, public ztype::FontCache::ProgressListener
    {
        IStartupTaskProgressListener* progressListener;
        
        Object<ztype::FontCache> cache;
        
        const char* cache_dir;
        int build_count;
        
        public:
            UpdateFontCacheStartupTask();
            virtual ~UpdateFontCacheStartupTask();
        
            virtual int Analyze() override;
            virtual void Init(IStartupTaskProgressListener* progressListener) override;
            virtual void Start() override;
        
            virtual void AddFaceList(SeekableInputStream* input) override;
        
            virtual void OnFontCacheProgress(int faces_done, int faces_total, int glyphs_done, int glyphs_total) override;
    };
    
    UpdateFontCacheStartupTask::UpdateFontCacheStartupTask()
            : cache_dir("fontcache"),
            build_count(0)
    {
        cache = ztype::FontCache::Create();
        cache->SetProgressListener(this);
    }
    
    UpdateFontCacheStartupTask::~UpdateFontCacheStartupTask()
    {
    }

    void UpdateFontCacheStartupTask::Init(IStartupTaskProgressListener* progressListener)
    {
        this->progressListener = progressListener;
    }
    
    void UpdateFontCacheStartupTask::AddFaceList(SeekableInputStream* input)
    {
        Object<ztype::FaceListFile> face_list = ztype::FaceListFile::Open(input);
        
        if (face_list == nullptr)
            return;
        
        int c = cache->BuildUpdateList(face_list, cache_dir, false, false);
        
        if (c <= 0)
            return;
        
        build_count += c;
    }
    
    int UpdateFontCacheStartupTask::Analyze()
    {
        printf("\n====================\nbuildfontcache: Will build cache for %i faces\n====================\n\n", build_count);

        // FIXME: Query Glyph Count
        return 1000;
    }
    
    void UpdateFontCacheStartupTask::OnFontCacheProgress(int faces_done, int faces_total, int glyphs_done, int glyphs_total)
    {
        progressListener->SetProgress((faces_done * 1000 / faces_total) + (1000 / faces_total * glyphs_done / glyphs_total));
    }
    
    void UpdateFontCacheStartupTask::Start()
    {
        progressListener->SetStatusMessage("building font cache...");

        Directory::create(cache_dir);
        cache->UpdateCache(false);
        
        progressListener->SetProgress(1000);
    }

    class StartupWorkerThread : public Thread, public IStartupTaskProgressListener
    {
        public:
            List<IStartupTask*> tasks;
        
        volatile StartupState* status;
        
        int progress_initial;
        
        protected:
            virtual void run() override;
        
        public:
            StartupWorkerThread(volatile StartupState *status) : status(status) {}
            virtual ~StartupWorkerThread();
        
            virtual void SetProgress(int points) override;
            virtual void SetStatusMessage(const char* message) override;
    };

    StartupWorkerThread::~StartupWorkerThread()
    {
        iterate2 (task, tasks)
            delete task;
    }

    void StartupWorkerThread::run()
    {
        try
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
                status->currentTask = task;

                progress_initial = status->progress;
            
                task->Start();
            }
        
            SetStatusMessage("loading user interface...");
            status->done = true;
        }
        catch (const SysException& ex)
        {
            Sys::DisplayError(ex, ERR_DISPLAY_UNCAUGHT);

            // FIXME: we need to fail here
        }
    }
    
    void StartupWorkerThread::SetProgress(int points)
    {
        status->progress = progress_initial + points;
    }
    
    void StartupWorkerThread::SetStatusMessage(const char* message)
    {
        status->text = message;
    }
    
    StartupScreen::StartupScreen(IScene *nextScene)
        : nextScene(nextScene)
    {
        prog = nullptr;

        logofont = nullptr;
        bootfont = nullptr;

        logo = nullptr;
        loading = nullptr;
        
        worker = nullptr;
        updateFontCacheStartupTask = nullptr;
    }

    void StartupScreen::PreInit()
    {
        worker = new StartupWorkerThread(&state);
    }
    
    void StartupScreen::Init()
    {
        state.done = false;
        state.text = nullptr;
        state.progress = 0;
        state.progress_max = 0;

        state.currentTask = nullptr;

        frame = 0;
        numFrames = 3;

        loading = render::Graphics::Create("image(" STARTUP_PREFIX "loading.png) anim_hstrip(12, 16)", true);

        ReloadMedia();

        worker->start();

        Var::SetInt("r_batchallocsize", 512);
    }

    void StartupScreen::Exit()
    {
        li::destroy(worker);
        
        li::release(loading);
        ReleaseMedia();

        li::destroy(nextScene);
    }
    
    void StartupScreen::AddTask(zfw::IStartupTask *task, bool inOrder, bool threaded)
    {
        ZFW_ASSERT(inOrder == false)
        ZFW_ASSERT(threaded == true)
        
        worker->tasks.add(task);
    }

    void StartupScreen::DrawScene()
    {
        zr::R::SelectShadingProgram(prog);
        zr::R::ResetView();

        zr::R::DrawFilledRect(Short2(-1, -1), Short2(1, 1), Byte4());

        zr::R::SelectShadingProgram(nullptr);
        zr::R::Set2DView();
        
        const char* const string = "zombie framework";

        Int2 size = logofont->MeasureAsciiString(string);
        Int2 pos = Int2((viewport_w - (64 + 48 + size.x)) / 2, viewport_h / 2);

        logofont->DrawAsciiString(string, pos + Int2(64 + 48, -size.y), RGBA_GREY(0.2f), 0);

        StartupState copy = state;

        if (copy.text != nullptr)
            bootfont->DrawAsciiString(copy.text, Int2(viewport_w / 2, viewport_h * 2 / 3), RGBA_GREY(0.2f), ALIGN_CENTER | ALIGN_MIDDLE);

        if (copy.progress_max > 0)
        {
            const int progress_width = 200;
            const int progress_height = 6;

            const int x0 = viewport_w / 2 - progress_width / 2;
            const int y0 = viewport_h * 2 / 3 + 30;

            zr::R::SetTexture(nullptr);
            zr::R::DrawFilledRect(Short2(x0, y0), Short2(x0 + progress_width, y0 + progress_height), Byte4(COLOUR_GREY(0.2f, 0.4f) * 255.0f));
            zr::R::DrawFilledRect(Short2(x0, y0), Short2(x0 + progress_width * copy.progress / copy.progress_max, y0 + progress_height), Byte4(COLOUR_GREY(0.2f) * 255.0f));
        }

        zr::R::DrawTexture(logo, Short2(pos.x, pos.y - 64));

        if (!copy.done)
            loading->Draw(Short2(viewport_w / 2 - 16, viewport_h / 2 + 32), Short2(32, 32));

        if (copy.currentTask != nullptr)
            copy.currentTask->OnMainThreadDraw();
    }
    
    IUpdateFontCacheStartupTask* StartupScreen::GetUpdateFontCacheStartupTask(bool createIfNeeded)
    {
        if (updateFontCacheStartupTask == nullptr && createIfNeeded)
        {
            updateFontCacheStartupTask = new UpdateFontCacheStartupTask();
            AddTask(updateFontCacheStartupTask, false, true);
        }
        
        return updateFontCacheStartupTask;
    }

    void StartupScreen::OnFrame( double delta )
    {
        if (state.done && ++frame == numFrames)
        {
            Sys::ChangeScene(nextScene);
            nextScene = nullptr;
            return;
        }

        Event_t* event;

        while ( ( event = Event::Pop() ) != nullptr )
        {
            switch ( event->type )
            {
                case EV_VKEY:
                    if ( event->vkey.vk.inclass == IN_OTHER && event->vkey.vk.key == OTHER_CLOSE )
                    {
                        Sys::BreakGameLoop();
                        return;
                    }
                    break;

                case EV_VIEWPORT_RESIZE:
                    viewport_w = event->viewresize.w;
                    viewport_h = event->viewresize.h;
                    SetShaderAttribs();
                    break;
                    
                default:
                    ;
            }
        }

        loading->OnFrame(delta);
    }

    void StartupScreen::ReleaseMedia()
    {
        li::destroy(prog);

        li::release(logo);
        li_tryCall(loading, DropResources());

        li::destroy(logofont);
        li::destroy(bootfont);
    }

    void StartupScreen::ReloadMedia()
    {
        viewport_w = Var::GetInt("r_viewportw", true);
        viewport_h = Var::GetInt("r_viewporth", true);

        prog = render::Loader::LoadShadingProgram(STARTUP_PREFIX "startup");
        SetShaderAttribs();

        logo = render::Loader::LoadTexture(STARTUP_PREFIX "zfw_64.png", true);
        loading->AcquireResources();

        logofont = zr::Font::Open(STARTUP_PREFIX "logofont");
        bootfont = zr::Font::Open(STARTUP_PREFIX "bootfont");

        logofont->SetShadow(0);
        bootfont->SetShadow(0);

        frame = 0;
        numFrames = 3;
    }

    void StartupScreen::SetShaderAttribs()
    {
        const int w = viewport_w, h = viewport_h;

        int half_viewport = prog->GetUniformLocation("half_viewport");

        Float3 val = Float3(w / 2, h / 2, 0.0f);
        val.z = sqrt(val.x * val.x + val.y * val.y);

        prog->SetUniform(half_viewport, val);
    }
}
