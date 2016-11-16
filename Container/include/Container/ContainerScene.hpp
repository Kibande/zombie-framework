#pragma once

#include <Container/SceneGraph.hpp>
#include <Container/SceneLayerGameUI.hpp>

#include <framework/resourcemanager.hpp>
#include <framework/resourcemanager2.hpp>
#include <framework/scene.hpp>

#define CONTAINER_WITH_RENDERING_KIT

// FIXME: we definitely don't want these here
#include <framework/entityworld.hpp>
#include <RenderingKit/gameui/RKUIThemer.hpp>

/*
    ContainerScene lifecycle:

    ContainerScene()

    <dependency injection takes place>

    Init()
        InitGameUI()
        InitWorld()

        PreBindDependencies()
        all resources -> BindDependencies()
        PostBindDependencies()

        Preload()
        all resources -> Preload()
        PostPreload()

    AcquireResources()
        PreRealize()
        all resources -> Realize()
        PostRealize()
*/

namespace Container {
    class ContainerApp;

    class ContainerScene : public zfw::IScene {
    public:
        enum { kUseUI = 1 };
        enum { kUseWorld = 2 };

        ContainerScene(ContainerApp* app, int options) : app(app), options(options) {}

        virtual bool Init() override;
        virtual void Shutdown() override;

        virtual bool AcquireResources() override;
        virtual void DropResources() override;

        virtual void DrawScene() override;
        virtual void OnFrame(double delta) override;

        // Public API for overriding Scene

        // Scene Layer: Clear

        virtual void SetClearColor(const zfw::Float4& color);

        // Scene Layer: 3D

#ifdef CONTAINER_WITH_RENDERING_KIT
        virtual void InitWorld();
        virtual void SetWorldCamera(std::shared_ptr<RenderingKit::ICamera>&& cam) { sceneLayerWorld->SetCamera(std::move(cam)); }
#endif

        // Scene Layer: GameUI

        virtual void InitGameUI();
        virtual SceneLayerGameUI* GetUILayer() { return sceneLayerUI; }

        // Resource Management

        virtual bool PreBindDependencies() { return true; }

        // Frame-to-frame operations

        virtual bool HandleEvent(zfw::MessageHeader* msg) { return false; }

    private:

    protected:
        ContainerApp* app;
        int options;

        SceneStack sceneStack;

        SceneLayerClear* sceneLayerClear = nullptr;

#ifdef CONTAINER_WITH_RENDERING_KIT
        // 3D - should be encapsulated
        SceneLayer3D* sceneLayerWorld = nullptr;
        std::unique_ptr<zfw::IResourceManager2> worldResMgr;
        std::unique_ptr<zfw::EntityWorld> world;
#endif

        // UI - should be encapsulated
        SceneLayerGameUI* sceneLayerUI = nullptr;
        std::unique_ptr<zfw::IResourceManager> uiResMgr;
        std::unique_ptr<gameui::UIThemer> uiThemer;
    };
}
