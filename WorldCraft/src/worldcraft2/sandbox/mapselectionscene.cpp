
#include "mapselectionscene.hpp"

#include <framework/colorconstants.hpp>
#include <framework/engine.hpp>
#include <framework/errorcheck.hpp>
#include <framework/event.hpp>
#include <framework/filesystem.hpp>
#include <framework/messagequeue.hpp>
#include <framework/varsystem.hpp>

#include <gameui/gameui.hpp>

#include <RenderingKit/gameui/RKUIThemer.hpp>

namespace sandbox
{
    class MapSelectionScene : public IMapSelectionScene
    {
        public:
            virtual ~MapSelectionScene() {}

            virtual bool Init() override;
            virtual void Shutdown() override;

            virtual bool AcquireResources() override;
            virtual void DropResources() override;

            virtual void DrawScene() override;
            virtual void OnFrame( double delta ) override;
            virtual void OnTicks(int ticks) override {}

        private:
            bool CreateUI();

            unique_ptr<gameui::UIContainer> ui;
            unique_ptr<RenderingKit::IRKUIThemer> uiThemer;

            gameui::TableLayout* panelTbl = nullptr;

            bool changeScene = false;
    };

    shared_ptr<IMapSelectionScene> IMapSelectionScene::Create()
    {
        return std::make_shared<MapSelectionScene>();
    }

    bool MapSelectionScene::Init()
    {
        ErrorPassthru(CreateUI());
        ErrorPassthru(AcquireResources());

        return true;
    }

    void MapSelectionScene::Shutdown()
    {
        ui.reset();
        uiThemer.reset();
    }

    bool MapSelectionScene::AcquireResources()
    {
        if (!ui->AcquireResources())
            return false;

        ui->SetArea(Int3(), g.rm->GetViewportSize());

        return true;
    }

    void MapSelectionScene::DropResources()
    {
        uiThemer->DropResources();
    }

    bool MapSelectionScene::CreateUI()
    {
        auto imh = g_sys->GetModuleHandler(true);

        uiThemer.reset(TryCreateRKUIThemer(imh));
        ErrorPassthru(uiThemer);

        uiThemer->Init(g_sys, g.rk.get(), g.res.get());

        uiThemer->PreregisterFont("wc2_ui",               "path=media/font/Roboto-Bold.ttf,size=16");
        uiThemer->PreregisterFont("wc2_ui_bold",          "path=media/font/Roboto-Bold.ttf,size=16");
        uiThemer->PreregisterFont("wc2_ui_title",         "path=media/font/Roboto-Light.ttf,size=24");
        uiThemer->PreregisterFont("wc2_ui_footer",        "path=media/font/Roboto-Light.ttf,size=16");

        ErrorPassthru(uiThemer->AcquireResources());

        gameui::Widget::SetMessageQueue(g.msgQueue.get());
        auto t = uiThemer.get();

        ui.reset(new gameui::UIContainer(g_sys));

        auto footer = new gameui::Panel(t);
        footer->SetAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
        footer->SetExpands(false);
        footer->SetPadding(Int2(15, 15));

        auto footerLabel = new gameui::StaticText(t, "<c:000>sandbox <c:999>by Minexew Games", uiThemer->GetFontId("wc2_ui_footer"));
        footerLabel->SetAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
        footer->Add(footerLabel);
        ui->Add(footer);

        auto panel = new gameui::Panel(t);
        panel->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
        panel->SetExpands(false);
        panelTbl = new gameui::TableLayout(1);

        auto title = new gameui::StaticText(t, "Please select a map:", uiThemer->GetFontId("wc2_ui_title"));
        title->SetColour(COLOUR_BLACK);
        panelTbl->Add(title);

        panelTbl->Add(new gameui::Spacer(t, Int2(0, 40)));

        auto ifs = g_sys->GetFileSystem();
        unique_ptr<IDirectory> dir(ifs->OpenDirectory("", 0));

        while (dir)
        {
            li::String entry = dir->ReadDir();

            if (entry.isEmpty())
                break;

            if (entry.endsWith(".bleb"))
            {
                auto btn = new gameui::Button(t, entry.replaceAll(".bleb", ""));
                panelTbl->Add(btn);
            }
        }

        panel->Add(panelTbl);
        ui->Add(panel);

        return true;
    }

    void MapSelectionScene::DrawScene()
    {
        g.rm->SetClearColour(COLOUR_WHITE);
        g.rm->SetProjectionOrthoScreenSpace(-1.0f, 1.0f);
        g.rm->SetRenderState(RK_DEPTH_TEST, 0);
        g.rm->Clear();

        ui->Draw();
    }

    void MapSelectionScene::OnFrame( double delta )
    {
        if (changeScene)
            g_sys->ChangeScene(CreateSandboxScene());

        ui->OnFrame(delta);

        for (MessageHeader* msg = nullptr; (msg = g.msgQueue->Retrieve(li::Timeout(0))) != nullptr; msg->Release())
        {
            int h = gameui::h_new;
            int rc = 0;

            h = ui->HandleMessage(h, msg);

            switch (msg->type)
            {
            case EVENT_WINDOW_CLOSE:
                g_sys->StopMainLoop();
                break;

            case gameui::EVENT_CONTROL_USED:
            {
                auto payload = msg->Data<gameui::EventControlUsed>();
                auto btn = dynamic_cast<gameui::Button*>(payload->widget);

                if (btn)
                {
                    auto var = g_sys->GetVarSystem();
                    var->SetVariable("map", btn->GetLabel(), 0);
                    changeScene = true;

                    if (panelTbl)
                    {
                        auto title = new gameui::StaticText(uiThemer.get(), "Loading...",
                                uiThemer->GetFontId("wc2_ui_title"));
                        title->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
                        title->SetColour(COLOUR_BLACK);
                        title->AcquireResources();

                        panelTbl->RemoveAll();
                        panelTbl->Add(title);

                        ui->Layout();
                    }
                }

                break;
            }
            }
        }
    }
}
