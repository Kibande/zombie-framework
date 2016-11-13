#include <Container/ContainerApp.hpp>
#include <Container/ContainerScene.hpp>

#include <framework/event.hpp>
#include <framework/messagequeue.hpp>

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
		}

	private:
		RenderingKit::IRenderingManager* rm;
	};

	bool ContainerScene::AcquireResources() {
		/*PreSceneLoad();

		PostSceneLoad();*/

		if (sceneLayerUI) {
			if (!uiThemer->AcquireResources())
				return false;

			auto ui = sceneLayerUI->GetUIContainer();
			ui->AcquireResources();
		}

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
		PreBindDependencies();

		if (!AcquireResources())
			return false;

		return true;
	}

	void ContainerScene::InitUI() {
		auto rk = app->GetRenderingHandler()->GetRenderingKit();
		auto rm = rk->GetRenderingManager();

		uiResMgr.reset(app->GetSystem()->CreateResourceManager("uiThemer Resource Manager"));
		rm->RegisterResourceProviders(uiResMgr.get());

		uiThemer.reset(RenderingKit::CreateRKUIThemer());
		uiThemer->Init(app->GetSystem(), rk, uiResMgr.get());
		uiThemer->PreregisterFont(nullptr, "path=ContainerAssets/font/DejaVuSans.ttf,size=12");

		auto layer = std::make_unique<SceneLayerGameUI>(app->GetSystem());
		sceneLayerUI = layer.get();
		sceneStack.PushLayer(std::move(layer));

		sceneLayerUI->GetUIContainer()->SetArea(zfw::Int3(0, 0, 0), rm->GetViewportSize());
	}

	void ContainerScene::OnFrame(double delta) {
		zfw::MessageHeader* msg;

		auto msgQueue = app->GetMessageQueue();

		while ((msg = msgQueue->Retrieve(li::Timeout(0))) != nullptr) {
			if (HandleEvent(msg))
				continue;

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
