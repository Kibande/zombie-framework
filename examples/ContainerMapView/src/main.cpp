#include "ContainerMapView.hpp"
#include "MapSelectionScene.hpp"

#include <Container/ContainerApp.hpp>
#include <Container/ContainerScene.hpp>

#include <framework/colorconstants.hpp>
#include <framework/event.hpp>
#include <framework/messagequeue.hpp>
#include <framework/pointentity.hpp>
#include <framework/system.hpp>
#include <framework/varsystem.hpp>

#if defined(_DEBUG) && defined(_MSC_VER)
#include <crtdbg.h>
#endif

namespace example {
    using Container::ContainerApp;
    using Container::ContainerScene;

    class MyContainerApp : public ContainerApp {
    public:
        MyContainerApp() : ContainerApp("ContainerMapView") {
        }

        std::shared_ptr<zfw::IScene> CreateInitialScene() {
            auto ivs = GetSystem()->GetVarSystem();

            if (!ivs->GetVariableOrDefault("map", (const char*) nullptr))
                return CreateMapSelectionScene(this);
            else
                return CreateMapViewScene(this);
        }
    };
}

int main(int argc, char** argv) {
#if defined(_DEBUG) && defined(_MSC_VER)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc();
#endif

    return Container::runContainerApp<example::MyContainerApp>(argc, argv);
}
