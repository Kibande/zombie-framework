#define SHADOW_MAPPING
#include "mapselectionscene.hpp"
#include "sandbox.hpp"

#include <littl+bleb/ByteIOStream.hpp>
#include <littl+bleb/StreamByteIO.hpp>
#include <bleb/repository.hpp>

#include <framework/entityhandler.hpp>
#include <framework/entityworld.hpp>
#include <framework/errorcheck.hpp>
#include <framework/event.hpp>
#include <framework/filesystem.hpp>
#include <framework/messagequeue.hpp>
#include <framework/resourcemanager.hpp>
#include <framework/pointentity.hpp>

#include <RenderingKit/utility/CameraMouseControl.hpp>
#include <RenderingKit/utility/TexturedPainter.hpp>

#include <littl/cfx2.hpp>

#include <vector>

namespace sandbox
{
    using namespace li;

    enum
    {
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

    struct Key_t
    {
        Vkey_t vk;
        bool pressed;
    };

    //// TODO: WGT
    struct WorldVertex_t
    {
        float pos[3];
        float normal[3];
        float uv[2];
    };

    static const VertexAttrib_t worldVertexAttribs[] = {
        { "in_Position",    0,  RK_ATTRIB_FLOAT_3 },
        { "in_Normal",      12, RK_ATTRIB_FLOAT_3 },
        { "in_UV",          24, RK_ATTRIB_FLOAT_2 },
        {}
    };
    ////

    static TexturedPainter3D<> tp;

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    class geom_wgt : public PointEntityBase
    {
        struct MatGrp_t
        {
            shared_ptr<IMaterial> mat;
            shared_ptr<IVertexFormat> vf;

            size_t numVertices;
            shared_ptr<IGeomChunk> geometry;
        };

        public:
            geom_wgt() {}

            virtual bool Init() override;

            virtual void Draw(const UUID_t* modeOrNull) override;

            void AddMatGrp(shared_ptr<IMaterial> material, IShader* shader);
            shared_ptr<IGeomChunk> AllocMatGrpVertices(size_t matIndex, size_t numVertices);
            shared_ptr<IVertexFormat> GetMatGrpVertexFormat(size_t matIndex);

        private:
            shared_ptr<IGeomBuffer> gb;

            List<MatGrp_t> matGrps;
    };

    class light_dynamic : public PointEntityBase
    {
        public:
            light_dynamic() {}
            light_dynamic(const Float3& pos, const Float3& ambient, const Float3& diffuse, float range)
                    : pos(pos), ambient(ambient), diffuse(diffuse), range(range) {}
            static IEntity* Create() { return new light_dynamic; }

            virtual int ApplyProperties(cfx2_Node* properties);
            virtual bool Init() override;

            void DrawDeferred(IDeferredShadingManager* deferred);
            void DrawDepthPass();

            shared_ptr<ITexture> GetDepthTexture();

        private:
            Float3 pos;
            Float3 ambient;
            Float3 diffuse;
            float range;

            // Shadowmapping
            shared_ptr<ICamera> cam;
            shared_ptr<IMaterial> materialOverride;
            shared_ptr<ITexture> depth;
            shared_ptr<IRenderBuffer> renderBuffer;
    };

    class skybox_2d : public PointEntityBase
    {
        public:
            skybox_2d(IEntity* linked_ent = nullptr) : linked_ent(linked_ent) {}

            virtual bool Init() override
            {
                ErrorCheck(tex = g.res->GetResourceByPath<ITexture>("media/texture/skytex.jpg", RESOURCE_REQUIRED, 0));

                return true;
            }

            virtual void Draw(const UUID_t* modeOrNull) override
            {
                //if (linked_ent != nullptr)
                //    pos = linked_ent->GetPos();

                //pos = cameraPosXXX;

                if (modeOrNull == nullptr)
                {
                    const Float3 size(500.0f, 500.0f, 500.0f);
                    tp.DrawFilledCuboid(tex.get(), pos + size * 0.5f, -size, RGBA_WHITE);
                }
            }

        private:
            shared_ptr<ITexture> tex;

            IEntity* linked_ent;
    };

    class SandboxScene : public ISandboxScene
    {
        public:
            virtual bool Init() override;
            virtual void Shutdown() override;

            virtual bool AcquireResources() override;
            virtual void DropResources() override;

            virtual void DrawScene() override;
            virtual void OnFrame(double delta) override;
            virtual void OnTicks(int ticks) override;

        private:
            bool p_LoadWorld(bleb::Repository& mf);

            shared_ptr<ICamera> cam;

            shared_ptr<IDeferredShadingManager> deferred;
            unique_ptr<IDeferredShaderBinding> deferredPointLight;
            List<shared_ptr<light_dynamic>> dynamicLights;

            Key_t controls[kControlsMax];
            bool noclip;
    };

    // ====================================================================== //
    //  class geom_wgt
    // ====================================================================== //

    bool geom_wgt::Init()
    {
        gb = g.rm->CreateGeomBuffer("geom_wgt/gb");
        ErrorCheck(gb);

        IEntityHandler* ieh = g_sys->GetEntityHandler(true);
        ieh->Register("light_dynamic",      &light_dynamic::Create);

        return true;
    }

    void geom_wgt::AddMatGrp(shared_ptr<IMaterial> material, IShader* shader)
    {
        auto& grp = matGrps.addEmpty();
        grp.mat = move(material);
        grp.vf = g.rm->CompileVertexFormat(shader, 32, worldVertexAttribs, false);
        grp.geometry = nullptr;
    }

    shared_ptr<IGeomChunk> geom_wgt::AllocMatGrpVertices(size_t matIndex, size_t numVertices)
    {
        auto& grp = matGrps[matIndex];
        grp.geometry.reset(gb->CreateGeomChunk());
        return grp.geometry;
    }

    void geom_wgt::Draw(const UUID_t* modeOrNull)
    {
        if (modeOrNull != nullptr)
            return;

        iterate2 (i, matGrps)
        {
            g.rm->DrawPrimitives((*i).mat.get(), RK_TRIANGLES, (*i).geometry.get());
        }
    }

    shared_ptr<IVertexFormat> geom_wgt::GetMatGrpVertexFormat(size_t matIndex)
    {
        auto& grp = matGrps[matIndex];
        return grp.vf;
    }

    // ====================================================================== //
    //  class light_dynamic
    // ====================================================================== //

    bool light_dynamic::Init()
    {
#ifdef SHADOW_MAPPING
        cam = g.rm->CreateCamera("light_dynamic/cam");
        cam->SetClippingDist(0.2f, 20.0f);
        cam->SetPerspective();
        cam->SetVFov(f_pi * 0.75f);
        cam->SetView(pos, pos + Float3(0.0f, 0.0f, -1.0f), Float3(0.0f, -1.0f, 0.0f));

        renderBuffer = g.rm->CreateRenderBuffer("light_dynamic/renderBuffer", Int2(1024, 1024),
                kRenderBufferColourTexture | kRenderBufferDepthTexture);
        depth = renderBuffer->GetDepthTexture();

        materialOverride = g.rm->CreateMaterial("light_dynamic/materialOverride",
                g.res->GetResourceByPath<IShader>("sandbox/depthonly", 0, 0));
#endif

        return true;
    }

    int light_dynamic::ApplyProperties(cfx2_Node* properties)
    {
        const char* colour;
        const char* pos;
        double range;

        if (cfx2_get_node_attrib(properties, "colour", &colour) == cfx2_ok)
        {
            Float3 c;

            if (!Util::ParseVec(colour, &c[0], 3, 3))
                return 1;

            this->ambient = 0.5f * c;
            this->diffuse = 0.5f * c;
        }

        if (cfx2_get_node_attrib(properties, "pos", &pos) == cfx2_ok)
            if (!Util::ParseVec(pos, &this->pos[0], 3, 3))
                return 1;

        if (cfx2_get_node_attrib_float(properties, "range", &range) == cfx2_ok)
            this->range = (float) range;

        return 0;
    }

    void light_dynamic::DrawDeferred(IDeferredShadingManager* deferred)
    {
        glm::mat4x4 shadowMatrix;

#ifdef SHADOW_MAPPING
        cam->GetProjectionModelViewMatrix(&shadowMatrix);
#endif

        deferred->DrawPointLight(pos, ambient, diffuse, range, depth.get(), shadowMatrix);
    }

    void light_dynamic::DrawDepthPass()
    {
        g.rm->PushRenderBuffer(renderBuffer.get());
        //glDrawBuffer(GL_NONE);
        g.rm->ClearDepth();

        g.rm->SetCamera(cam.get());
        g.rm->SetMaterialOverride(materialOverride.get());
        g.rm->SetRenderState(RK_DEPTH_TEST, 1);

        g.world->Draw(nullptr);

        g.rm->SetMaterialOverride(nullptr);

        g.rm->PopRenderBuffer();
        //glDrawBuffer(GL_BACK);
    }

    // ====================================================================== //
    //  class SandboxScene
    // ====================================================================== //

    shared_ptr<ISandboxScene> CreateSandboxScene()
    {
        return std::make_shared<SandboxScene>();
    }

    bool SandboxScene::Init()
    {
        g.world.reset(new EntityWorld(g_sys));

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

        noclip = false;

        ErrorPassthru(AcquireResources());
        return true;
    }

    void SandboxScene::Shutdown()
    {
        g.world.reset();

        tp.Shutdown();
        DropResources();
    }

    bool SandboxScene::AcquireResources()
    {
        const Int2 res(1280, 720);

        g.rm->SetClearColour(COLOUR_BLACK);

        // initialize dynamic lights + their shadow maps
        iterate2 (i, dynamicLights)
            ErrorCheck(i->Init());

        // create Deferred Shading Manager
        deferred = g.rm->CreateDeferredShadingManager();
        deferred->AddBufferByte4(res, "diffuseBuf");
        deferred->AddBufferFloat4(res, "positionBuf");
        deferred->AddBufferFloat4(res, "normalsBuf");

        // used in deferred lighting for point lights (TODO: replace with spotlights and omnilights)
        auto pointLightShader = g.res->GetResourceByPath<IShader>("sandbox/point", RESOURCE_REQUIRED, 0);
        ErrorCheck(pointLightShader);

        deferredPointLight = deferred->CreateShaderBinding(pointLightShader);

        //// TODO: WGT
        // load the map
        auto ifs = g_sys->GetFileSystem();
        auto ivs = g_sys->GetVarSystem();

        const char* map;
        zombie_assert(ivs->GetVariable("map", &map, IVarSystem::kVariableMustExist));
        
        InputStream* is_ = nullptr;
        if (!ifs->OpenFileStream(sprintf_255("%s.bleb", map), 0, &is_, nullptr, nullptr))
            return nullptr;

        unique_ptr<InputStream> is(is_);

        InputStreamByteIO bio(is.get());
        bleb::Repository repo(&bio, false);

        if (!repo.open(false))
        {
            return ErrorBuffer::SetError3(EX_ASSET_OPEN_ERR, 2,
                    "desc", sprintf_4095("bleb error: %s", repo.getErrorDesc()),
                    "function", li_functionName
                    ), false;
        }

        ErrorCheck(p_LoadWorld(repo));

        repo.close();
        is.reset();
        //////////////////

        // simple skybox for now
        auto skybox = std::make_shared<skybox_2d>();
        ErrorCheck(skybox->Init());
        g.world->AddEntity(skybox);

        ErrorCheck(tp.Init(g.rm));

        cam = g.rm->CreateCamera("SandboxScene/cam");
        cam->SetClippingDist(0.2f, 510.0f);
        cam->SetPerspective();
        cam->SetVFov(45.0f * f_pi / 180.0f);
        //cam->SetView(Float3(0.0f, 128.0f, 128.0f), Float3(), glm::normalize(Float3(0.0f, -1.0f, 1.0f)));
        //cam->SetView(Float3(0.0f, 0.0f, 28.0f), Float3(0.0f, -1.0f, 28.0f), Float3(0.0f, 0.0f, 1.0f));
        cam->SetView2(Float3(0.0f, 0.0f, 1.6f), -1.0f, 0.0f, 0.0f);

        return true;
    }

    void SandboxScene::DropResources()
    {
        deferredPointLight.reset();
    }

    void SandboxScene::DrawScene()
    {
#ifdef SHADOW_MAPPING
        iterate2 (i, dynamicLights)
            i->DrawDepthPass();
#endif

        // Deferred shading
        deferred->BeginScene();
        // Deferred shading end

        //g.rm->Clear();
        g.rm->ClearDepth();
        g.rm->SetCamera(cam.get());
        g.rm->SetRenderState(RK_DEPTH_TEST, 1);

        g.world->Draw(nullptr);

        // Deferred shading
        deferred->EndScene();
        g.rm->SetRenderState(RK_DEPTH_TEST, 0);

        deferred->BeginShading(deferredPointLight.get(), cam->GetEye());

        iterate2 (i, dynamicLights)
            i->DrawDeferred(deferred.get());

        deferred->EndShading();
        // Deferred shading end
    }

    void SandboxScene::OnFrame(double delta)
    {
        MessageHeader* msg;

        static Int2 r_mousepos = Int2(-1, -1);

        while ((msg = g.msgQueue->Retrieve(Timeout(0))) != nullptr)
        {
            switch (msg->type)
            {
                case EVENT_MOUSE_MOVE:
                {
                    auto payload = msg->Data<EventMouseMove>();
                    
                    if (r_mousepos.x >= 0)
                    {
                        static const float sensitivity = 0.01f;     // radians per pixel

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
                                    g_sys->ChangeScene(IMapSelectionScene::Create());
                            }
                            else if (i == kControlsQuit)
                            {
                                if (payload->input.flags & VKEY_PRESSED)
                                    g_sys->StopMainLoop();
                            }
                            else
                                controls[i].pressed = (payload->input.flags & VKEY_PRESSED) ? 1 : 0;
                        }
                    break;
                }
            }

            msg->Release();
        }
    }

    bool SandboxScene::p_LoadWorld(bleb::Repository& repo)
    {
        ///** FIXME
        shared_ptr<IShader> worldShader = g.res->GetResource<IShader>(
                "path=sandbox/world,outputNames=out_Color out_Position out_Normal", RESOURCE_REQUIRED, 0);
        ErrorCheck(worldShader);
        /// END FIXME

        IEntityHandler* ieh = g_sys->GetEntityHandler(true);

        auto geom = std::make_shared<geom_wgt>();

        ErrorCheck(geom->Init());

        // ================================================================== //
        //  section zombie.WorldMaterials
        // ================================================================== //

        auto materials_ = repo.openStream("zombie.WorldMaterials", 0);

        if (materials_ == nullptr)
            return ErrorBuffer::SetError2(g.eb, EX_ASSET_CORRUPTED, 1,
                    "desc",     "The map is corrupted (missing section zombie.WorldMaterials)"
            ), false;

        ByteIOStream materials(materials_.get());

        while (!materials.eof())
        {
            String desc = materials.readString();

            //IMaterial* material = g.res->GetResource<IMaterial>(desc, RESOURCE_REQUIRED, 0);
            //ErrorCheck(material);

            ///**
            const char* texture;
            if (!Params::GetValueForKey(desc, "texture", texture))
                texture = "path=media/texture/_NULL.jpg";

            auto tex = g.res->GetResource<ITexture>(texture, RESOURCE_REQUIRED, 0);
            ErrorCheck(tex);

            auto material = g.rm->CreateMaterial("derp", worldShader);
            material->SetTexture("tex", move(tex));
            ///**

            geom->AddMatGrp(move(material), worldShader.get());
        }

        materials_.reset();

        // ================================================================== //
        //  section zombie.WorldVertices
        // ================================================================== //

        auto vertices_ = repo.openStream("zombie.WorldVertices", 0);

        if (vertices_ == nullptr)
            return ErrorBuffer::SetError2(g.eb, EX_ASSET_CORRUPTED, 1,
                    "desc",     "The map is corrupted (missing section zombie.WorldVertices)"
            ), false;

        std::vector<uint8_t> vertexBuffer;
        ByteIOStream vertices(vertices_.get());

        for (size_t matIndex = 0; !vertices.eof(); matIndex++)
        {
            uint32_t numVertsForMaterial = 0;

            vertices.readLE(&numVertsForMaterial);

            auto gc = geom->AllocMatGrpVertices(matIndex, numVertsForMaterial);
            zombie_assert(gc != nullptr);

            auto fmt = geom->GetMatGrpVertexFormat(matIndex);
            gc->AllocVertices(fmt.get(), numVertsForMaterial, 0);

            vertexBuffer.resize(numVertsForMaterial * fmt->GetVertexSize());
            vertices.read(&vertexBuffer[0], numVertsForMaterial * fmt->GetVertexSize());

            gc->UpdateVertices(0, &vertexBuffer[0], numVertsForMaterial * fmt->GetVertexSize());
        }

        vertices_.reset();

        // ================================================================== //
        //  section zombie.Entities
        // ================================================================== //

        auto entities_ = repo.openStream("zombie.Entities", 0);
        ByteIOStream entities(entities_.get());

        while (entities_ != nullptr && !entities.eof())
        {
            String entityCfx2 = entities.readString();

            cfx2::Document entityDoc;
            entityDoc.loadFromString(entityCfx2);

            if (entityDoc.getNumChildren() > 0)
            {
                auto ent = shared_ptr<IEntity>(ieh->InstantiateEntity(entityDoc[0].getText(), ENTITY_REQUIRED));
                ErrorLog(g_sys, g.eb, ent != nullptr);

                if (ent != nullptr)
                {
                    ent->ApplyProperties(entityDoc[0].c_node());
                    ErrorPassthru(ent->Init());

                    auto light_dynamic_ = std::dynamic_pointer_cast<light_dynamic>(ent);

                    if (light_dynamic_ != nullptr)
                        dynamicLights.add(light_dynamic_);
                    else
                        g.world->AddEntity(move(ent));
                }
            }
        }

        g.world->AddEntity(geom);
        return true;
    }

    void SandboxScene::OnTicks(int ticks)
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
