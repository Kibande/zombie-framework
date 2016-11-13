#pragma once

#include <framework/resourcemanager.hpp>
#include <framework/scene.hpp>

#include <Container/SceneGraph.hpp>
#include <Container/SceneLayerGameUI.hpp>

#include <RenderingKit/gameui/RKUIThemer.hpp>

/*
	ContainerScene lifecycle:

	ContainerScene()

	<dependency injection takes place>

	Init()
		BindDependencies()
			PreBindDependencies()

			resMgr->MakeAllResourcesState(BOUND)

			PostBindDependencies()

		Preload()
			PrePreload()

			resMgr->MakeAllResourcesState(PRELOADED)

			PostPreload()

	AcquireResources()
		PreSceneLoad()

		resMgr->MakeAllResourcesState(REALIZED)

		PostSceneLoad()
*/

namespace Container {
	class ContainerApp;

	class ContainerScene : public zfw::IScene {
	public:
		ContainerScene(ContainerApp* app) : app(app) {}

		virtual bool Init() override;
		virtual void Shutdown() override;

		virtual bool AcquireResources() override;
		virtual void DropResources() override;

		virtual void DrawScene() override;
		virtual void OnFrame(double delta) override;

		// -----

		virtual void InitUI();
		virtual SceneLayerGameUI* GetUILayer() { return sceneLayerUI; }
		virtual void SetClearColor(const zfw::Float4& color);

		virtual void PreBindDependencies() {}

		virtual bool HandleEvent(zfw::MessageHeader* msg) { return false; }

	private:

	protected:
		ContainerApp* app;

		SceneStack sceneStack;

		SceneLayerClear* sceneLayerClear = nullptr;
		SceneLayerGameUI* sceneLayerUI = nullptr;

		// UI - should probably be encapsulated
		std::unique_ptr<zfw::IResourceManager> uiResMgr;
		std::unique_ptr<RenderingKit::IRKUIThemer> uiThemer;
	};
}
