#include <Container/ContainerScene.hpp>

#include <framework/event.hpp>
#include <framework/messagequeue.hpp>

namespace Container {
	void SceneStack::Walk(SceneLayerVisitor* visitor) {
		for (auto& layer : layers)
			visitor->Visit(layer.get());
	}
}
