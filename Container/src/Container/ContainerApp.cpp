#include <Container/ContainerApp.hpp>
#include <Container/ContainerScene.hpp>
#include <Container/RenderingHandler.hpp>

#include <framework/engine.hpp>
#include <framework/errorbuffer.hpp>
#include <framework/filesystem.hpp>
#include <framework/messagequeue.hpp>
#include <framework/varsystem.hpp>

namespace Container {
    bool ContainerApp::ConfigureFileSystem(zfw::IEngine* sys) {
        zfw::IFSUnion* fsUnion = sys->GetFSUnion();
        fsUnion->AddFileSystem(sys->CreateStdFileSystem(".", zfw::kFSAccessStat | zfw::kFSAccessRead), 100);

        return true;
    }

    std::shared_ptr<zfw::IScene> ContainerApp::CreateInitialScene() {
        return std::make_shared<ContainerScene>(this, 0);
    }

    std::unique_ptr<IRenderingHandler> ContainerApp::CreateRenderingHandler() {
        return std::make_unique<RenderingHandler>(sys, eventQueue);
    }

    int ContainerApp::Execute() {
        if (!TopLevelRun()) {
            if (sys && eb)
                sys->DisplayError(eb, true);
        }

        AppDeinit();

        renderingHandler.reset();

        SysDeinit();

        return 0;
    }

    bool ContainerApp::TopLevelRun() {
        if (!SysInit())
            return false;

        constexpr bool renderingEnabled = true;

        if (renderingEnabled) {
            renderingHandler = CreateRenderingHandler();
            
            if (!renderingHandler || !renderingHandler->RenderingInit())
                return false;
        }

        if (!AppInit())
            return false;

        sys->ChangeScene(CreateInitialScene());
        sys->RunMainLoop();
        sys->ReleaseScene();

        return true;
    }

    void ContainerApp::SetArgv(int argc, char** argv) {
        this->argc = argc;
        this->argv = argv;
    }

    /*void ContainerApp::SetSystem(IEngine) {
        this->argc = argc;
        this->argv = argv;
    }*/

    bool ContainerApp::SysInit() {
        zfw::ErrorBuffer_t* eb;
        zfw::ErrorBuffer::Create(eb);

        auto sys = zfw::CreateEngine();

        if (!sys->Init(eb, 0))
            return false;

        sys->ParseArgs1(argc, argv);

        auto var = sys->GetVarSystem();
        var->SetVariable("appName", appName, 0);

        if (!ConfigureFileSystem(sys))
            return false;

        if (!sys->Startup())
            return false;

        //this->SetSystem(sys, eb);
        this->sys = sys;
        this->eb = eb;

        eventQueue = std::shared_ptr<zfw::MessageQueue>(zfw::MessageQueue::Create());

        return true;
    }

    void ContainerApp::SysDeinit() {
        if (sys)
            sys->Shutdown();

        zfw::ErrorBuffer::Release(eb);
    }
}
