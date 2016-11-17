#include "ContainerMapView.hpp"
#include "MapSelectionScene.hpp"

#include <Container/ContainerApp.hpp>
#include <Container/ContainerScene.hpp>

#include <framework/colorconstants.hpp>
#include <framework/errorcheck.hpp>
#include <framework/event.hpp>
#include <framework/filesystem.hpp>
#include <framework/messagequeue.hpp>
#include <framework/system.hpp>
#include <framework/varsystem.hpp>

#include <gameui/gameui.hpp>

#include <RenderingKit/gameui/RKUIThemer.hpp>

namespace example
{
    using namespace zfw;

    class MapSelectionScene : public Container::ContainerScene
    {
        public:
            MapSelectionScene(Container::ContainerApp* app) : Container::ContainerScene(app, kUseUI | kUseWorld) {}
            virtual ~MapSelectionScene() {}

            virtual bool PreBindDependencies() override;

            virtual int HandleEvent(MessageHeader* msg, int h) override;

            virtual void OnTicks(int ticks) override;

        private:
            gameui::TableLayout* panelTbl = nullptr;

            bool changeScene = false;
    };

    std::shared_ptr<IScene> CreateMapSelectionScene(Container::ContainerApp* app)
    {
        return std::make_shared<MapSelectionScene>(app);
    }

    bool MapSelectionScene::PreBindDependencies()
    {
        this->SetClearColor(COLOUR_WHITE);

        auto sys = app->GetSystem();
        auto imh = sys->GetModuleHandler(true);

        auto font_ui = uiThemer->PreregisterFont("ui",                  "path=media/font/Roboto-Medium.ttf,size=16");
        auto font_ui_title = uiThemer->PreregisterFont("ui_title",      "path=media/font/Roboto-Light.ttf,size=24");
        auto font_ui_footer = uiThemer->PreregisterFont("ui_footer",    "path=media/font/Roboto-Light.ttf,size=16");

        auto ui = this->GetUILayer()->GetUIContainer();
        auto t = uiThemer.get();

        auto footer = new gameui::Panel(t);
        footer->SetAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
        footer->SetExpands(false);
        footer->SetPadding(Int2(15, 15));

        auto footerLabel = new gameui::StaticText(t, "<c:000>sandbox <c:999>by Minexew Games", font_ui_footer);
        footerLabel->SetAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
        footer->Add(footerLabel);
        ui->Add(footer);

        auto panel = new gameui::Panel(t);
        panel->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        panel->SetExpands(false);
        panelTbl = new gameui::TableLayout(1);

        auto title = new gameui::StaticText(t, "Please select a map:", font_ui_title);
        title->SetColour(COLOUR_BLACK);
        panelTbl->Add(title);

        panelTbl->Add(new gameui::Spacer(t, Int2(0, 40)));

        auto ifs = sys->GetFileSystem();
        unique_ptr<IDirectory> dir(ifs->OpenDirectory("", 0));

        while (dir)
        {
            li::String entry = dir->ReadDir();

            if (entry.isEmpty())
                break;

            if (entry.endsWith(".bleb"))
            {
                auto btn = new gameui::Button(t, entry.replaceAll(".bleb", ""));
                btn->SetFont(font_ui);
                panelTbl->Add(btn);
            }
        }

        panel->Add(panelTbl);
        ui->Add(panel);

        return true;
    }

    int MapSelectionScene::HandleEvent(MessageHeader* msg, int h) {
        switch (msg->type)
        {
        case gameui::EVENT_CONTROL_USED:
        {
            auto payload = msg->Data<gameui::EventControlUsed>();
            auto btn = dynamic_cast<gameui::Button*>(payload->widget);

            if (btn)
            {
                auto var = app->GetSystem()->GetVarSystem();
                var->SetVariable("map", btn->GetLabel(), 0);
                changeScene = true;

                if (panelTbl)
                {
                    auto title = new gameui::StaticText(uiThemer.get(), "Loading...",
                        uiThemer->GetFontId("ui_title"));
                    title->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
                    title->SetColour(COLOUR_BLACK);
                    title->AcquireResources();

                    panelTbl->RemoveAll();
                    panelTbl->Add(title);

                    sceneLayerUI->GetUIContainer()->Layout();
                }
            }

            break;
        }
        }

        return h;
    }

    void MapSelectionScene::OnTicks(int ticks) {
        if (changeScene)
            app->GetSystem()->ChangeScene(CreateMapViewScene(app));
    }
}
