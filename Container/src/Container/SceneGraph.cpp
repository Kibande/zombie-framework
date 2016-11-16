#include <Container/SceneGraph.hpp>

#include <framework/entityworld.hpp>

namespace Container {
    void SceneNodeGroup::Walk(SceneNodeVisitor* visitor) {
        for (auto& node : contents)
            visitor->Visit(node.get());
    }

    void SceneNodeEntityWorld::Draw(zfw::UUID_t* modeOrNull) {
        world->Draw(modeOrNull);
    }

    void SceneStack::Walk(SceneLayerVisitor* visitor) {
        for (auto& layer : layers)
            visitor->Visit(layer.get());
    }
}
