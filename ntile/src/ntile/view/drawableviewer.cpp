#include "view.hpp"

#include "../world.hpp"

#include <framework/components/model3d.hpp>
#include <framework/components/position.hpp>
#include <framework/engine.hpp>
#include <framework/entityworld2.hpp>
#include <framework/resourcemanager2.hpp>

#include <RenderingKit/Model.hpp>

namespace ntile {
    using namespace RenderingKit;
    using std::make_unique;

    void DrawableViewer::Draw(IRenderingManager* rm, zfw::IEntityWorld2* world, intptr_t entityId) {
        auto transform = (this->position) ?
                         glm::translate({}, this->position->pos) * glm::mat4_cast(this->position->rotation) :
                         glm::mat4x4();

        if (this->blockyModel) {
            Float3 designScale {TILE_SIZE / 16.0f, TILE_SIZE / 16.0f, TILE_SIZE / 16.0f};
            this->blockyModel->Draw(transform * glm::scale({}, designScale));
        }

        if (this->model) {
            Float3 designScale {TILE_SIZE, TILE_SIZE, TILE_SIZE};
            this->model->Draw(transform * glm::scale(glm::rotate(glm::mat4 {}, f_pi / 2, Float3 {1, 0, 0}), designScale));
        }
    }

    void DrawableViewer::OnTicks(int ticks) {
        if (blockyModel) {
            while (ticks--) {
                blockyModel->AnimationTick();
            }
        }
    }

    bool DrawableViewer::p_TryLoadBlockyModel() {
        this->blockyModel = make_unique<CharacterModel>(g_eb, g_res.get());

        if (this->blockyModel->Load(model3d->modelPath.c_str())) {
            auto anim = blockyModel->GetAnimationByName("standing");

            if (anim != nullptr)
                blockyModel->StartAnimation(anim);

            return true;
        }
        else {
            this->blockyModel.reset();
            return false;
        }
    }

    bool DrawableViewer::p_TryLoadModel(IResourceManager2 &res) {
        model = res.GetResourceByPath<IModel>(model3d->modelPath.c_str(), 0);

        return model != nullptr;
    }

    void DrawableViewer::Realize(IEntityWorld2& world, intptr_t entityId, IResourceManager2 &res) {
        if (!model3d) {
            model3d = world.GetEntityComponent<Model3D>(entityId);
            position = world.GetEntityComponent<Position>(entityId);
            zombie_assert(model3d);             // why call if you've got nothing to say ?!
        }

        if (mustReload) {
            mustReload = false;

            // TODO: resource management is being given zero thought here

            if (!p_TryLoadBlockyModel()) {
                if (!p_TryLoadModel(res)) {
                    g_sys->Printf(kLogWarning, "Failed to load model '%s'", model3d->modelPath.c_str());
                }
            }
        }
    }

    void DrawableViewer::TriggerAnimation(const char* animationName) {
        if (blockyModel) {
            auto anim = blockyModel->GetAnimationByName(animationName);

            if (anim) {
                blockyModel->StartAnimation(anim);
            }
        }
    }
}
