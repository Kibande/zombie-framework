#include "view.hpp"

#include "../world.hpp"

#include <framework/aspects/drawable.hpp>
#include <framework/aspects/position.hpp>
#include <framework/entityworld2.hpp>

namespace ntile {
    using std::make_unique;

    void DrawableViewer::Draw(RenderingKit::IRenderingManager* rm, zfw::IEntityWorld2* world, intptr_t entityId) {
        if (!drawable) {
            drawable = world->GetEntityAspect<Drawable>(entityId);
            position = world->GetEntityAspect<Position>(entityId);
            zombie_assert(drawable);
        }

        if (!model) {
            // TODO: resource management is being given zero thought here

            this->model = make_unique<CharacterModel>(g_eb, g_res.get());
            this->model->Load(drawable->modelPath.c_str());
        }

        this->model->Draw();
    }
}
