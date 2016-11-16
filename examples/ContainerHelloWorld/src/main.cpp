#include <Container/ContainerApp.hpp>
#include <Container/ContainerScene.hpp>

#include <framework/event.hpp>
#include <framework/messagequeue.hpp>
#include <framework/system.hpp>

namespace example {
    using Container::ContainerApp;
    using Container::ContainerScene;
    using namespace zfw;

    class ExampleScene : public ContainerScene {
    public:
        ExampleScene(ContainerApp* app) : ContainerScene(app, kUseUI) {}

        virtual bool PreBindDependencies() override {
            this->SetClearColor(Float4(0.1f, 0.2f, 0.3f, 1.0f));

            auto ui = this->GetUILayer()->GetUIContainer();

            auto font = uiThemer->PreregisterFont(nullptr, "path=ContainerAssets/font/DejaVuSans.ttf,size=120");

            auto layout = new gameui::TableLayout(1);
            layout->SetColumnGrowable(0, true);
            layout->SetRowGrowable(0, true);
            layout->Add(new gameui::StaticText(uiThemer.get(), "Hello World!", font));
            ui->Add(layout);

            return true;
        }
    };

    class MyContainerApp : public ContainerApp {
    public:
        MyContainerApp() : ContainerApp("ContainerHelloWorld") {
        }

        std::shared_ptr<zfw::IScene> CreateInitialScene() {
            return std::make_shared<ExampleScene>(this);
        }
    };
}

int main(int argc, char** argv) {
    return Container::runContainerApp<example::MyContainerApp>(argc, argv);
}
