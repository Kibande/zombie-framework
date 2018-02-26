#include <Container/ContainerApp.hpp>
#include <Container/ContainerScene.hpp>

#include <framework/entityworld.hpp>
#include <framework/event.hpp>
#include <framework/messagequeue.hpp>
#include <framework/system.hpp>

#include <framework/errorcheck.hpp>

namespace Container {
    // TODO: move this
    class DefaultSceneStackRenderHandler : public SceneLayerVisitor {
    public:
        DefaultSceneStackRenderHandler(RenderingKit::IRenderingManager* rm) : rm(rm) {}

        virtual void Visit(SceneLayer* layer) override {
            auto clearLayer = dynamic_cast<SceneLayerClear*>(layer);

            if (clearLayer) {
                rm->SetClearColour(clearLayer->GetColor());
                rm->Clear();
                return;
            }

            auto screenSpaceLayer = dynamic_cast<SceneLayerScreenSpace*>(layer);

            if (screenSpaceLayer) {
                rm->SetRenderState(RenderingKit::RK_DEPTH_TEST, 0);
                rm->SetProjectionOrthoScreenSpace(-1.0f, 1.0f);
                screenSpaceLayer->DrawContents();
                return;
            }

            auto a3dLayer = dynamic_cast<SceneLayer3D*>(layer);

            if (a3dLayer) {
                auto cam = a3dLayer->GetCamera();

                if (cam) {
                    rm->SetCamera(a3dLayer->GetCamera());
                    rm->SetRenderState(RenderingKit::RK_DEPTH_TEST, 1);
                    a3dLayer->DrawContents();
                }

                return;
            }
        }

    private:
        RenderingKit::IRenderingManager* rm;
    };

    bool ContainerScene::AcquireResources() {
        ErrorCheck(PreRealize());

        if (sceneLayerUI) {
            uiResMgr->SetTargetState(zfw::IResource2::REALIZED);
            ErrorCheck(uiResMgr->MakeAllResourcesTargetState(true));

            ErrorCheck(uiThemer->AcquireResources());

            auto ui = sceneLayerUI->GetUIContainer();
            ErrorCheck(ui->AcquireResources());
        }

        if (worldResMgr) {
            worldResMgr->SetTargetState(zfw::IResource2::REALIZED);
            ErrorCheck(worldResMgr->MakeAllResourcesTargetState(true));
        }

        ErrorCheck(PostRealize());

        return true;
    }

    void ContainerScene::DrawScene() {
        // Wow...
        DefaultSceneStackRenderHandler renderHandler(app->GetRenderingHandler()->GetRenderingKit()->GetRenderingManager());

        sceneStack.Walk(&renderHandler);
    }

    void ContainerScene::DropResources() {
        if (uiThemer) {
            uiThemer->DropResources();
        }
    }

    bool ContainerScene::Init() {
		SetClearColor(zfw::Float4(1.0, 1.0, 1.0, 1.0));

        if (options & kUseUI)
            InitGameUI();

        if (options & kUseWorld)
            InitWorld();

        // BindDependencies step
        ErrorCheck(PreBindDependencies());

		if (world) {
			zfw::ResourceManagerScope scope(worldResMgr.get());

			ErrorCheck(world->InitAllEntities());
		}

        if (worldResMgr) {
            ErrorCheck(worldResMgr->MakeAllResourcesState(zfw::IResource2::BOUND, true));
        }

        ErrorCheck(PostBindDependencies());

        // Preload step
        ErrorCheck(PrePreload());

        if (worldResMgr) {
            ErrorCheck(worldResMgr->MakeAllResourcesState(zfw::IResource2::PRELOADED, true));
        }

        ErrorCheck(PostPreload());

        // Realize step
        if (!AcquireResources())
            return false;

        if (sceneLayerUI)
            sceneLayerUI->GetUIContainer()->Layout();

        return true;
    }

    void ContainerScene::InitGameUI() {
        auto rk = app->GetRenderingHandler()->GetRenderingKit();
        auto rm = rk->GetRenderingManager();

        // Event Queue
        gameui::Widget::SetMessageQueue(app->GetEventQueue().get());

        // Resource Manager
        uiResMgr.reset(app->GetSystem()->CreateResourceManager2());
        rm->RegisterResourceProviders(uiResMgr.get());

        // UI Themer
        auto uiThemer = RenderingKit::CreateRKUIThemer();
        uiThemer->Init(app->GetSystem(), rk, uiResMgr.get());
        uiThemer->PreregisterFont(nullptr, "path=ContainerAssets/font/DejaVuSans.ttf,size=12");
        this->uiThemer.reset(uiThemer);

        // Scene Layer
        auto layer = std::make_unique<SceneLayerGameUI>(app->GetSystem());
        sceneLayerUI = layer.get();
        sceneStack.PushLayer(std::move(layer));

        sceneLayerUI->GetUIContainer()->SetArea(zfw::Int3(0, 0, 0), rm->GetViewportSize());
    }

    void ContainerScene::InitWorld() {
        auto rk = app->GetRenderingHandler()->GetRenderingKit();
        auto rm = rk->GetRenderingManager();
        auto sys = app->GetSystem();

        // Resource Manager
        worldResMgr.reset(app->GetSystem()->CreateResourceManager2());
        rm->RegisterResourceProviders(worldResMgr.get());

        // Entity World
        world = std::make_unique<zfw::EntityWorld>(sys);

        // Scene Layer
        auto layer = std::make_unique<SceneLayer3D>();
        sceneLayerWorld = layer.get();
        sceneStack.PushLayer(std::move(layer));

        sceneLayerWorld->Add(std::make_unique<SceneNodeEntityWorld>(world.get()));
    }

    void ContainerScene::OnFrame(double delta) {
        zfw::MessageHeader* msg;

        if (sceneLayerUI)
            sceneLayerUI->GetUIContainer()->OnFrame(delta);

        auto eventQueue = app->GetEventQueue();

        while ((msg = eventQueue->Retrieve(li::Timeout(0))) != nullptr) {
            int h = HandleEvent(msg, zfw::h_new);

            if (h >= zfw::h_stop)
                continue;

            if (sceneLayerUI)
                h = sceneLayerUI->GetUIContainer()->HandleMessage(h, msg);

            switch (msg->type) {
            case zfw::EVENT_WINDOW_CLOSE:
                app->GetSystem()->StopMainLoop();
                break;

            case zfw::EVENT_WINDOW_RESIZE: {
                auto data = msg->Data<zfw::EventWindowResize>();
                if (sceneLayerUI)
                    sceneLayerUI->GetUIContainer()->SetArea(zfw::Int3(0, 0, 0), zfw::Int2(data->width, data->height));
                break;
            }
            }
        }

        li::pauseThread(1);
    }

    void ContainerScene::SetClearColor(const zfw::Float4& color) {
        if (!sceneLayerClear) {
            auto layer = std::make_unique<SceneLayerClear>(color);

            sceneLayerClear = layer.get();
            sceneStack.PushLayer(std::move(layer));
        }
        else
            sceneLayerClear->SetColor(color);
    }

    void ContainerScene::Shutdown() {
        sceneStack.Clear();

        DropResources();

        uiThemer.reset();
        uiResMgr.reset();
    }
}
