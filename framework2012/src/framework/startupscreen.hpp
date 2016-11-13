#pragma once

#include <framework/framework.hpp>
#include <framework/rendering.hpp>

namespace zfw
{
    class IStartupTaskProgressListener
    {
        public:
            virtual void SetProgress(int points) = 0;
            virtual void SetStatusMessage(const char* message) = 0;
    };
    
    class IStartupTask
    {
        public:
            virtual ~IStartupTask() {}

            virtual int Analyze() = 0;
            virtual void Init(IStartupTaskProgressListener* progressListener) = 0;
            virtual void Start() = 0;

            virtual void OnMainThreadDraw() {}
    };
    
    class IUpdateFontCacheStartupTask : public IStartupTask
    {
        public:
            virtual void AddFaceList(SeekableInputStream* input) = 0;
    };

    class StartupWorkerThread;

    struct StartupState
    {
        bool done;
        const char *text;
        int progress, progress_max;

        IStartupTask* currentTask;
    };
    
    class StartupScreen : public IScene
    {
        StartupState state;
        StartupWorkerThread* worker;
        IScene *nextScene;
        
        IUpdateFontCacheStartupTask* updateFontCacheStartupTask;

        render::IProgram *prog;

        render::ITexture *logo/*, *loading*/;
        zr::Font *logofont, *bootfont;
        render::Graphics *loading;

        int viewport_w, viewport_h;

        //double animDelta;
        int frame, numFrames;

        void SetShaderAttribs();

        public:
            StartupScreen(IScene *nextScene);

            void PreInit();
            void AddTask(IStartupTask* task, bool inOrder, bool threaded);
            IUpdateFontCacheStartupTask* GetUpdateFontCacheStartupTask(bool createIfNeeded);
        
            virtual void Init() override;
            virtual void Exit() override;

            virtual void ReleaseMedia() override;
            virtual void ReloadMedia() override;

            virtual void DrawScene() override;
            virtual void OnFrame( double delta ) override;
    };
}