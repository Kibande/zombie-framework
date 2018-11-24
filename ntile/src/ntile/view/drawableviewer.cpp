#include "view.hpp"

#include "../world.hpp"

#include <framework/components/model3d.hpp>
#include <framework/components/position.hpp>
#include <framework/entityworld2.hpp>

namespace ntile {
    using std::make_unique;

    void DrawableViewer::Draw(RenderingKit::IRenderingManager* rm, zfw::IEntityWorld2* world, intptr_t entityId) {
        if (!drawable) {
            drawable = world->GetEntityComponent<Model3D>(entityId);
            position = world->GetEntityComponent<Position>(entityId);
            zombie_assert(drawable);
        }

        if (!model) {
            // TODO: resource management is being given zero thought here

            this->model = make_unique<CharacterModel>(g_eb, g_res.get());
            this->model->Load(drawable->modelPath.c_str());

            auto anim = model->GetAnimationByName("standing");

            if (anim != nullptr)
                model->StartAnimation(anim);
        }

        if (position) {
            this->model->Draw(glm::translate({}, position->pos) * glm::mat4_cast(position->rotation));
        }
        else {
            this->model->Draw(glm::mat4x4());
        }
    }

    void DrawableViewer::OnTicks(int ticks) {
        if (!model) {
            return;
        }

        while (ticks--) {
            model->AnimationTick();
        }
    }

    void DrawableViewer::TriggerAnimation(const char* animationName) {
        if (!model) {
            return;
        }

        auto anim = model->GetAnimationByName(animationName);

        if (anim) {
            model->StartAnimation(anim);
        }
    }
}
