#pragma once

#include <Container/ContainerInterfaces.hpp>

namespace Container {
    class ContainerApp : public IContainerApp {
    public:
        virtual void SetArgv(int argc, char** argv);

        int Execute();

        std::shared_ptr<zfw::MessageQueue> GetEventQueue() { return eventQueue; }
        IRenderingHandler* GetRenderingHandler() { return renderingHandler.get(); }
        zfw::ISystem* GetSystem() { return sys; }

    protected:
        ContainerApp(const char* appName) : appName(appName) {}

        bool TopLevelRun();

        virtual void Deinit() {}

        virtual bool SysInit();
        virtual void SysDeinit();

        /// Called before ISystem::Startup!
        virtual bool ConfigureFileSystem(zfw::ISystem* sys);

        virtual std::shared_ptr<zfw::IScene> CreateInitialScene();
        virtual std::unique_ptr<IRenderingHandler> CreateRenderingHandler();

        virtual bool AppInit() { return true; }
        virtual void AppDeinit() {}

        const char* appName;
        int argc;
        char** argv;

        zfw::ISystem* sys;
        zfw::ErrorBuffer_t* eb;
        std::shared_ptr<zfw::MessageQueue> eventQueue;

        std::unique_ptr<IRenderingHandler> renderingHandler;
    };

    template <class MyContainerApp>
    int runContainerApp(int argc, char** argv) {
        MyContainerApp app;
        app.SetArgv(argc, argv);
        return app.Execute();
    }
}
