#include <Container/SceneGraph.hpp>

namespace Container {
    class DefaultSceneNodeRenderHandler : public SceneNodeVisitor {
    public:
        //DefaultSceneNodeRenderHandler(RenderingKit::IRenderingManager* rm) : rm(rm) {}

        virtual void Visit(SceneNode* node) override {
            node->Walk(this);

            auto entityWorld = dynamic_cast<SceneNodeEntityWorld*>(node);

            if (entityWorld) {
                entityWorld->Draw(nullptr);
                return;
            }
        }

    private:
        //RenderingKit::IRenderingManager* rm;
    };

    void SceneLayer3D::DrawContents() {
        // Wow...
        DefaultSceneNodeRenderHandler renderHandler;

        root.Walk(&renderHandler);
    }
}
