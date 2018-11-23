#include <Guier.hpp>

#include <Container/ContainerApp.hpp>
#include <Container/ContainerScene.hpp>

namespace example {
    using Container::ContainerApp;
    using Container::ContainerScene;
    using namespace zfw;
    using namespace guier;

    class ExampleScene : public ContainerScene {
    public:
        ExampleScene(ContainerApp* app) : ContainerScene(app, kUseUI) {}

        virtual bool PreBindDependencies() override {
            this->SetClearColor(Float4(0.2f, 0.2f, 0.3f, 1.0f));

            auto ui = this->GetUILayer()->GetUIContainer();

            /*auto font = uiThemer->PreregisterFont(nullptr, "path=ContainerAssets/font/DejaVuSans.ttf,size=120");

            auto layout = new gameui::TableLayout(1);
            layout->SetColumnGrowable(0, true);
            layout->SetRowGrowable(0, true);
            layout->Add(new gameui::StaticText(uiThemer.get(), "Hello World!", font));
            ui->Add(layout);*/

            g = std::make_unique<Guier>(app->GetSystem(), uiThemer.get(), ui);

            return true;
        }

        void OnFrame(double delta) override {
            ContainerScene::OnFrame(delta);

            if (g)
                gui(*g);

            if (sceneLayerUI)
                sceneLayerUI->GetUIContainer()->Layout();
        }

        void gui(Guier& g);

        std::unique_ptr<Guier> g;

        int counter = 0;
        std::vector<int> numbers;
    };

    class MyContainerApp : public ContainerApp {
    public:
        MyContainerApp() : ContainerApp("ContainerGui") {
        }

        std::shared_ptr<zfw::IScene> CreateInitialScene() {
            return std::make_shared<ExampleScene>(this);
        }
    };

    void ExampleScene::gui(Guier& g) {
        auto ui = g.Ui();
            auto font = Guier_FontBuilder().Path("ContainerAssets/font/DejaVuSans.ttf").Size(120);

            auto layout = ui.TableLayout(1);
            layout.SetColumnGrowable(0, true);
            layout.SetRowGrowable(0, true);
            layout.Save();

                layout.StaticText("Hello World!", font);

                for (size_t i = 0; i < numbers.size(); i++) {
                    auto button = layout.Button(sprintf_255("%d (click to remove!)", numbers[i]));

                    if (button.WasClicked()) {
                        numbers.erase(numbers.begin() + i);
                        i--;
                        layout.Unsave();
                        continue;
                    }
                }

                auto increaseCounter = layout.Button(sprintf_255("Increase counter (%d)", this->counter));
                auto addRandom = layout.Button("Add number");

        ui.Save();

        if (increaseCounter.WasClicked()) {
            this->counter++;
            layout.Unsave();
        }

        if (addRandom.WasClicked()) {
            this->numbers.push_back(rand());
            layout.Unsave();
        }
    }
}

int main(int argc, char** argv) {
    return Container::runContainerApp<example::MyContainerApp>(argc, argv);
}
