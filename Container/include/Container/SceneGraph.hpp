#pragma once

#include <vector>

#include <RenderingKit/RenderingKit.hpp>

/*
    Example Scene Stack:

    - SceneLayer Clear (#333333)

    - SceneLayer 3D
        - RenderingKit::ICamera
        - SceneNodeGroup
            - SceneNodeGroup
                - SceneNodeMesh
                - SceneNodeMesh
            - SceneNodeMesh

    - SceneLayer GameUI
        - UIContainer

    - SceneLayer ScreenSpace - Custom
        - custom overlay

*/

namespace Container {
    class SceneLayer;
    class SceneNode;

    class SceneLayerVisitor {
    public:
        virtual void Visit(SceneLayer* layer) = 0;
    };

    class SceneNodeVisitor {
    public:
        virtual void Visit(SceneNode* node) = 0;
    };

    class SceneNode {
    public:
        virtual void Walk(SceneNodeVisitor* visitor) {}
    };

    class SceneNodeEntityWorld : public SceneNode {
    public:
        SceneNodeEntityWorld(zfw::EntityWorld* world) : world(world) {}

        void Draw(zfw::UUID_t* modeOrNull);

    private:
        zfw::EntityWorld* world;
    };

    class SceneNodeGroup : public SceneNode {
    public:
        void Add(std::unique_ptr<SceneNode>&& node) { contents.push_back(std::move(node)); }

        virtual void Walk(SceneNodeVisitor* visitor) override;

    private:
        std::vector<std::unique_ptr<SceneNode>> contents;
    };

    class SceneLayer {
    public:
        virtual ~SceneLayer() {}
    };

    class SceneLayerClear : public SceneLayer {
    public:
        SceneLayerClear(const zfw::Float4& clearColor) : clearColor(clearColor) {}

        const zfw::Float4& GetColor() const { return clearColor; }
        void SetColor(const zfw::Float4& clearColor) { this->clearColor = clearColor; }

    private:
        zfw::Float4 clearColor;
    };

    class SceneLayerScreenSpace : public SceneLayer {
    public:
        virtual void DrawContents() = 0;

    private:
    };

    class SceneLayer3D : public SceneLayer {
    public:
        void Add(std::unique_ptr<SceneNode>&& node) { root.Add(std::move(node)); }
        void DrawContents();
        RenderingKit::ICamera* GetCamera() { return camera.get(); }
        void SetCamera(std::shared_ptr<RenderingKit::ICamera>&& camera) { this->camera = std::move(camera); }

    private:
        std::shared_ptr<RenderingKit::ICamera> camera;
        SceneNodeGroup root;
    };

    class SceneStack {
    public:
        void Clear() { layers.clear(); }
        void PushLayer(std::unique_ptr<SceneLayer>&& layer) { layers.push_back(std::move(layer)); }
        void Walk(SceneLayerVisitor* visitor);

    private:
        std::vector<std::unique_ptr<SceneLayer>> layers;
    };
}
