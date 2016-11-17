#include "ContainerMapView.hpp"
#include "MapSelectionScene.hpp"

#include <Container/ContainerApp.hpp>
#include <Container/ContainerScene.hpp>

#include <framework/colorconstants.hpp>
#include <framework/entityhandler.hpp>
#include <framework/entityworld.hpp>
#include <framework/errorcheck.hpp>
#include <framework/event.hpp>
#include <framework/filesystem.hpp>
#include <framework/messagequeue.hpp>
#include <framework/resourcemanager.hpp>
#include <framework/system.hpp>
#include <framework/pointentity.hpp>
#include <framework/varsystem.hpp>

#include <RenderingKit/utility/CameraMouseControl.hpp>
#include <RenderingKit/utility/TexturedPainter.hpp>

#include <vector>

namespace example {
    enum {
        kControlsUp,
        kControlsDown,
        kControlsLeft,
        kControlsRight,
        kControlsZoomIn,
        kControlsZoomOut,
        kControlsNoclip,
        kControlsMenu,
        kControlsQuit,
        kControlsMax
    };

    struct Key_t {
        Vkey_t vk;
        bool pressed;
    };

    TexturedPainter3D<> g_tp;

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    class MapViewScene : public Container::ContainerScene {
        public:
            MapViewScene(Container::ContainerApp* app) : Container::ContainerScene(app, kUseWorld) {}

            virtual bool PreBindDependencies() override;
            virtual bool PostPreload() override;

            virtual int HandleEvent(MessageHeader* msg, int h) override;

            virtual void OnTicks(int ticks) override;

        private:
            bool p_LoadEntities();

            Key_t controls[kControlsMax];
            bool noclip = false;
            std::string mapName;

            shared_ptr<ICamera> cam;
    };

    // ====================================================================== //
    //  class MapViewScene
    // ====================================================================== //

    shared_ptr<zfw::IScene> CreateMapViewScene(Container::ContainerApp* app)
    {
        return std::make_shared<MapViewScene>(app);
    }

    bool MapViewScene::PreBindDependencies()
    {
        this->SetClearColor(RGBA_BLACK);

        auto sys = app->GetSystem();

        controls[kControlsUp].vk.type = VKEY_KEY;
        controls[kControlsUp].vk.key = 'w';
        controls[kControlsUp].pressed = false;

        controls[kControlsDown].vk.type = VKEY_KEY;
        controls[kControlsDown].vk.key = 's';
        controls[kControlsDown].pressed = false;

        controls[kControlsLeft].vk.type = VKEY_KEY;
        controls[kControlsLeft].vk.key = 'a';
        controls[kControlsLeft].pressed = false;

        controls[kControlsRight].vk.type = VKEY_KEY;
        controls[kControlsRight].vk.key = 'd';
        controls[kControlsRight].pressed = false;

        controls[kControlsZoomIn].vk.type = VKEY_KEY;
        controls[kControlsZoomIn].vk.key = '-';
        controls[kControlsZoomIn].pressed = false;

        controls[kControlsZoomOut].vk.type = VKEY_KEY;
        controls[kControlsZoomOut].vk.key = '=';
        controls[kControlsZoomOut].pressed = false;

        controls[kControlsNoclip].vk.type = VKEY_KEY;
        controls[kControlsNoclip].vk.key = 'n';
        controls[kControlsNoclip].pressed = false;

        controls[kControlsMenu].vk.type = VKEY_KEY;
        controls[kControlsMenu].vk.key = 'm';
        controls[kControlsMenu].pressed = false;

        controls[kControlsQuit].vk.type = VKEY_KEY;
        controls[kControlsQuit].vk.key = 'q';
        controls[kControlsQuit].pressed = false;

        auto rm = app->GetRenderingHandler()->GetRenderingKit()->GetRenderingManager();
        ErrorCheck(g_tp.Init(rm));

        // Open Map
        auto ivs = sys->GetVarSystem();

        const char* map;
        zombie_assert(ivs->GetVariable("map", &map, IVarSystem::kVariableMustExist));
        this->mapName = map;

        auto fs = sys->CreateBlebFileSystem(sprintf_255("%s.bleb", map), kFSAccessStat | kFSAccessRead);
        sys->GetFSUnion()->AddFileSystem(move(fs), 10);

        char params[2048];
        zombie_assert(Params::BuildIntoBuffer(params, sizeof(params), 2,
                "path",         map,
                "worldShader", "path=ContainerMapView/singlePass"
        ));

        this->world->AddEntity(std::make_shared<Ent_WorldGeometry>(params));
        this->world->AddEntity(std::make_shared<Ent_SkyBox>());

        cam = rm->CreateCamera("ContainerMapView Camera");
        cam->SetClippingDist(0.2f, 510.0f);
        cam->SetPerspective();
        cam->SetVFov(45.0f * f_pi / 180.0f);
        //cam->SetView(Float3(0.0f, 128.0f, 128.0f), Float3(), glm::normalize(Float3(0.0f, -1.0f, 1.0f)));
        //cam->SetView(Float3(0.0f, 0.0f, 28.0f), Float3(0.0f, -1.0f, 28.0f), Float3(0.0f, 0.0f, 1.0f));
        cam->SetViewWithCenterDistanceYawPitch(Float3(0.0f, 0.0f, 1.6f), -1.0f, 0.0f, 0.0f);

        SetWorldCamera(shared_ptr<ICamera>(cam));

        return true;
    }

    bool MapViewScene::PostPreload() {
        ErrorCheck(p_LoadEntities());

        return true;
    }
    
    bool MapViewScene::p_LoadEntities() {
        auto sys = app->GetSystem();

        IEntityHandler* ieh = sys->GetEntityHandler(true);

        // ================================================================== //
        //  section zombie.Entities
        // ================================================================== //

        // Entities are optional
        unique_ptr<InputStream> entities(sys->OpenInput(sprintf_255("%s/entities", mapName.c_str())));

        while (entities && !entities->eof())
        {
            auto entityCfx2 = entities->readString();

            cfx2::Document entityDoc;
            entityDoc.loadFromString(entityCfx2);

            if (entityDoc.getNumChildren() > 0)
            {
                auto ent = shared_ptr<IEntity>(ieh->InstantiateEntity(entityDoc[0].getText(), ENTITY_REQUIRED));
                ErrorLog(sys, GetErrorBuffer(), ent != nullptr);

                if (ent != nullptr)
                {
                    ent->ApplyProperties(entityDoc[0].c_node());
                    world->AddEntity(move(ent));
                }
            }
        }

        return true;
    }

    int MapViewScene::HandleEvent(MessageHeader* msg, int h) {
        static Int2 r_mousepos = Int2(-1, -1);

        switch (msg->type)
        {
        case EVENT_MOUSE_MOVE:
        {
            auto payload = msg->Data<EventMouseMove>();

            if (r_mousepos.x >= 0)
            {
                // Mouse sensitivity in radians per pixel
                static const float sensitivity = 0.005f;

                cam->CameraRotateZ(-(payload->x - r_mousepos.x) * sensitivity, false);
                cam->CameraRotateXY((payload->y - r_mousepos.y) * sensitivity, false);
            }

            r_mousepos = Int2(payload->x, payload->y);
            break;
        }

        case EVENT_VKEY:
        {
            auto payload = msg->Data<EventVkey>();

            for (int i = 0; i < kControlsMax; i++)
                if (Vkey::Test(payload->input, controls[i].vk))
                {
                    if (i == kControlsNoclip)
                    {
                        if (payload->input.flags & VKEY_PRESSED)
                            noclip = !noclip;
                    }
                    else if (i == kControlsMenu)
                    {
                        if (payload->input.flags & VKEY_PRESSED)
                            app->GetSystem()->ChangeScene(CreateMapSelectionScene(app));
                    }
                    else if (i == kControlsQuit)
                    {
                        if (payload->input.flags & VKEY_PRESSED)
                            app->GetSystem()->StopMainLoop();
                    }
                    else
                        controls[i].pressed = (payload->input.flags & VKEY_PRESSED) ? 1 : 0;
                }
            break;
        }
        }

        return h;
    }

    void MapViewScene::OnTicks(int ticks)
    {
        static const float kCamSpeed = 0.1f;
        static const float kCamZoomSpeed = 0.1f;

        Float3 cameraDir = cam->GetDirection();

        if (!noclip)
            cameraDir.z = 0.0f;

        if (controls[kControlsUp].pressed)
            cam->CameraMove(Float3(cameraDir.x, cameraDir.y, cameraDir.z) * kCamSpeed * (float) ticks);
        if (controls[kControlsDown].pressed)
            cam->CameraMove(Float3(-cameraDir.x, -cameraDir.y, -cameraDir.z) * kCamSpeed * (float) ticks);
        if (controls[kControlsLeft].pressed)
            cam->CameraMove(Float3(cameraDir.y, -cameraDir.x, 0.0f) * kCamSpeed * (float) ticks);
        if (controls[kControlsRight].pressed)
            cam->CameraMove(Float3(-cameraDir.y, cameraDir.x, 0.0f) * kCamSpeed * (float) ticks);

        if (controls[kControlsZoomIn].pressed)
            cam->CameraZoom(-1.0f * kCamZoomSpeed * (float) ticks, false);

        if (controls[kControlsZoomOut].pressed)
            cam->CameraZoom(1.0f * kCamZoomSpeed * (float) ticks, false);
    }
}
