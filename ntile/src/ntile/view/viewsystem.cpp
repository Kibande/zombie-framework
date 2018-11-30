#include "ambient.hpp"
#include "view.hpp"

#include "../ntile.hpp"
#include "../viewsystem.hpp"
#include "../world.hpp"

#include <RenderingKit/utility/BasicPainter.hpp>
#include <RenderingKit/utility/RKVideoHandler.hpp>
#include <RenderingKit/utility/ShaderGlobal.hpp>

#include <framework/broadcasthandler.hpp>
#include <framework/colorconstants.hpp>
#include <framework/components/model3d.hpp>
#include <framework/components/position.hpp>
#include <framework/entityworld2.hpp>
#include <framework/resourcemanager2.hpp>
#include <framework/engine.hpp>

#include <unordered_map>

namespace ntile {
    using namespace RenderingKit;
    using std::make_unique;
    using std::shared_ptr;
    using std::unique_ptr;

    IRenderingManager* irm;

    VertexFormat<4> WorldVertex::format {
        sizeof(WorldVertex), {{
            {"in_Position", 0,  RK_ATTRIB_INT_3,    RK_ATTRIB_NOT_NORMALIZED},
            {"in_Normal",   12, RK_ATTRIB_SHORT_3,  0},
            {"in_Color",    20, RK_ATTRIB_UBYTE_4,  0},
            {"in_UV",       24, RK_ATTRIB_FLOAT_2,  0},
        }}
    };

    struct GlobalUniforms {
        ShaderGlobal blend_colour = "blend_colour";
    };

    struct AmbientUniforms {
        ShaderGlobal sun_dir = "sun_dir",
                     sun_amb = "sun_amb",
                     sun_diff = "sun_diff",
                     sun_spec = "sun_spec";
    };

    class ViewSystem : public IBroadcastSubscriber, public IViewSystem {
    public:
        ViewSystem(zfw::IResourceManager2& resourceManager, World& world) : res(resourceManager), world(world) {}

        bool Startup(zfw::IEngine* sys, zfw::ErrorBuffer_t* eb, zfw::MessageQueue* eventQueue) override;

        // zfw::IBroadcastSubscriber
        void OnComponentEvent(intptr_t entityId, IComponentType &type, void *data, ComponentEvent event) final;
        void OnMessageBroadcast(intptr_t type, const void* payload) final;

        // zfw::ISystem
        void OnFrame() final;
        void OnTicks(int ticks) final;

    private:
        void p_OnBlockStateChange(WorldBlock* block, BlockStateChange change);

        IResourceManager2& res;
        World& world;

        unique_ptr<IRenderingKit> rk;
        IRenderingManager* rm;

        // Global setup
        GlobalUniforms globalUniforms;

        Ambient ambient;
        AmbientUniforms ambientUniforms;

        BasicPainter3D<> bp3d;
        shared_ptr<ICamera> cam;
        Float3 camPos, camEye;

        // Blocks
        std::unordered_map<WorldBlock*, unique_ptr<BlockViewer>> blockViewers;
        IMaterial* blockMaterial;

        // Drawables
        std::unordered_map<intptr_t, unique_ptr<DrawableViewer>> drawableViewers;
    };

    // ====================================================================== //
    //  class ViewSystem
    // ====================================================================== //

    IViewSystem* IViewSystem::Create(zfw::IResourceManager2& res, World& world) {
        return new ViewSystem(res, world);
    }

    bool ViewSystem::Startup(zfw::IEngine* sys, zfw::ErrorBuffer_t* eb, zfw::MessageQueue* eventQueue) {
        rk.reset(CreateRenderingKit());
        rk->Init(sys, eb, nullptr);

        auto wm = rk->GetWindowManager();

        wm->LoadDefaultSettings(nullptr);
        wm->ResetVideoOutput();

        this->rm = rk->StartupRendering(CoordinateSystem::rightHanded);
        zombie_ErrorCheck(rm);
        irm = rm;

        rm->RegisterResourceProviders(g_res.get());
        g_res->SetTargetState(IResource2::State_t::REALIZED);

        sys->SetVideoHandler(make_unique<RKVideoHandler>(rm, wm, eventQueue));

        ambient.Init(world.daytime);

        globalUniforms.blend_colour.Bind(rm);

        ambientUniforms.sun_amb.Bind(rm);
        ambientUniforms.sun_diff.Bind(rm);
        ambientUniforms.sun_dir.Bind(rm);
        ambientUniforms.sun_spec.Bind(rm);

        blockMaterial = g_res->GetResource<IMaterial>("shader=path=ntile/shaders/world", 0);
        blockMaterial->SetTextureByIndex(0, g_res->GetResourceByPath<ITexture>("ntile/worldtex_0.png", 0));
        // TODO[mess]: this requires the material to be already realized; further reason to decouple these

        bp3d.Init(rm);

        camPos = Float3(worldSize.x * 128.0f, worldSize.y * 128.0f, 0.0f);
        cam = rm->CreateCamera("ViewSystem::cam");
        cam->SetClippingDist(200, 2000);
        cam->SetPerspective();
        cam->SetVFov(45.0f / 180.0f * 3.14f);

        // Sign up for broadcasts
        auto ibh = sys->GetBroadcastHandler();
        ibh->SubscribeToMessageType<AnimationTrigerEvent>(this);
        ibh->SubscribeToMessageType<BlockStateChangeEvent>(this);
        ibh->SubscribeToMessageType<WorldSwitchedEvent>(this);

        ibh->SubscribeToComponentType<Model3D>(this);

        return true;
    }

    void ViewSystem::OnComponentEvent(intptr_t entityId, IComponentType &type, void *data, ComponentEvent event) {
        if (&type == &GetComponentType<Model3D>()) {
            auto drawable = reinterpret_cast<Model3D*>(data);

            switch (event) {
                case ComponentEvent::created:
                    drawableViewers.emplace(entityId, std::make_unique<DrawableViewer>());
                    break;
                case ComponentEvent::destroyed:
                    drawableViewers.erase(entityId);
                    break;
            }
        }
    }

    void ViewSystem::OnFrame() {
        ambient.SetTime(world.daytime);
        Float3 backgroundColour;
        Float3 sun_ambient;
        Float3 sun_diffuse;
        Float3 sun_direction;

        ambient.CalculateWorldLighting(world.daytime,
                                       backgroundColour,
                                       sun_ambient,
                                       sun_diffuse,
                                       sun_direction);

        globalUniforms.blend_colour = COLOUR_WHITE;

        ambientUniforms.sun_amb = sun_ambient;
        ambientUniforms.sun_diff = sun_diffuse;
        ambientUniforms.sun_dir = sun_direction;

        rm->SetClearColour(Float4(backgroundColour, 1.0f));
        rm->Clear();
        rm->SetRenderState(RK_DEPTH_TEST, true);

        // Set up camera
        auto playerPos = g_ew->GetEntityComponent<Position>(g_world.playerEntity);
        if (playerPos != nullptr) {
            camPos = playerPos->pos;
        }

        auto camEye = Float3(0.0f, -300.0f, 300.0f);
        cam->SetView(camPos + camEye, camPos, Float3(0.0f, 0.707f, 0.707f));
        rm->SetCamera(cam.get());

        // Draw terrain
        //bp3d.DrawGridAround(Float3(512,512, 32), Float3(16, 16, 0), Int2(10, 10), RGBA_BLACK);
        auto worldBlocks = blocks;

        int bx1 = 0, by1 = 0, bx2 = worldSize.x - 1, by2 = worldSize.y - 1;

        // draw visible blocks (TODO)
        for (int by = by1; by <= by2; by++) {
            for (int bx = bx1; bx <= bx2; bx++) {
                WorldBlock* block = &worldBlocks[by * worldSize.x + bx];

                auto& vw = blockViewers[block];
                zombie_assert(vw != nullptr);

                vw->Draw(rm, Int2(bx, by), block, blockMaterial);
            }
        }

        // TODO: Draw entities
        for (const auto& pair : drawableViewers) {
            pair.second->Realize(*g_ew, pair.first, res);
            pair.second->Draw(rm, g_ew.get(), pair.first);
        }

        // TODO: Draw UI
    }

    void ViewSystem::OnMessageBroadcast(intptr_t type, const void* payload) {
        switch (type) {
        case kAnimationTrigger: {
            auto data = reinterpret_cast<const AnimationTrigerEvent*>(payload);

            const auto& iter = drawableViewers.find(data->entityId);

            if (iter != drawableViewers.end()) {
                iter->second->TriggerAnimation(data->animationName);
            }
        }

        case kBlockStateChangeEvent: {
            auto data = reinterpret_cast<const BlockStateChangeEvent*>(payload);
            this->p_OnBlockStateChange(data->block, data->change);
            break;
        }

        case kWorldSwitched: {
            camPos = Float3(worldSize.x * 128.0f, worldSize.y * 128.0f, 0.0f);
            break;
        }
        }
    }

    void ViewSystem::OnTicks(int ticks) {
        for (const auto& pair : drawableViewers) {
            pair.second->OnTicks(ticks);
        }
    }

    void ViewSystem::p_OnBlockStateChange(WorldBlock* block, BlockStateChange change) {
        switch (change) {
        case BlockStateChange::created:
            blockViewers.emplace(block, make_unique<BlockViewer>());
            break;

        case BlockStateChange::updated:
            blockViewers[block]->Update();
            break;

        case BlockStateChange::deleted:
            blockViewers.erase(block);
            break;
        }
    }
}
