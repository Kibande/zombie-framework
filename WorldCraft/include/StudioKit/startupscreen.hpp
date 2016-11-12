#pragma once

#include <framework/framework.hpp>
#include <framework/modulehandler.hpp>
#include <framework/scene.hpp>

#include <RenderingKit/RenderingKit.hpp>

#include <reflection/magic.hpp>

namespace StudioKit
{
    using namespace zfw;

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

    class IStartupScreen : public IScene
    {
        public:
            virtual bool Init(ISystem* sys, RenderingKit::IRenderingManager* rm, shared_ptr<IScene> nextScene) = 0;

            virtual void AddTask(IStartupTask* task, bool inOrder, bool threaded) = 0;

            REFL_CLASS_NAME("IStartupScreen", 1)
    };

#ifdef STUDIO_KIT_STATIC
    IStartupScreen* CreateStartupScreen();

    ZOMBIE_IFACTORYLOCAL(StartupScreen)
#else
    ZOMBIE_IFACTORY(StartupScreen, "StudioKit")
#endif
}
