#include "view.hpp"

#include "../world.hpp"

#include <framework/components/model3d.hpp>
#include <framework/components/position.hpp>
#include <framework/engine.hpp>
#include <framework/entityworld2.hpp>

namespace ntile {
    using std::make_unique;

    void DrawableViewer::Draw(RenderingKit::IRenderingManager* rm, zfw::IEntityWorld2* world, intptr_t entityId) {
        if (!model3d) {
            model3d = world->GetEntityComponent<Model3D>(entityId);
            position = world->GetEntityComponent<Position>(entityId);
            zombie_assert(model3d);
        }

        if (mustReload) {
            mustReload = false;

            // TODO: resource management is being given zero thought here
            this->model = make_unique<CharacterModel>(g_eb, g_res.get());

            if (this->model->Load(model3d->modelPath.c_str())) {
                auto anim = model->GetAnimationByName("standing");

                if (anim != nullptr)
                    model->StartAnimation(anim);
            }
            else {
                this->model.reset();
                g_sys->Log(kLogWarning, "Failed to load model '%s'", model3d->modelPath.c_str());
            }
        }

        if (this->model) {
            if (position) {
                this->model->Draw(glm::translate({}, position->pos) * glm::mat4_cast(position->rotation));
            } else {
                this->model->Draw(glm::mat4x4());
            }
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
