#include "view.hpp"

#include "../world.hpp"

#include <framework/components/drawable.hpp>
#include <framework/components/position.hpp>
#include <framework/entityworld2.hpp>

namespace ntile {
    using std::make_unique;

    void DrawableViewer::Draw(RenderingKit::IRenderingManager* rm, zfw::IEntityWorld2* world, intptr_t entityId) {
        if (!drawable) {
            drawable = world->GetEntityComponent<Drawable>(entityId);
            position = world->GetEntityComponent<Position>(entityId);
            zombie_assert(drawable);
        }

        if (!model) {
            // TODO: resource management is being given zero thought here

            this->model = make_unique<CharacterModel>(g_eb, g_res.get());
            this->model->Load(drawable->modelPath.c_str());
        }

        this->model->Draw(position ? glm::translate({}, position->pos) : glm::mat4x4());
    }
}
