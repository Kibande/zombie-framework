#include "components.hpp"
#include "ContainerMapView.hpp"
#include "RenderSystem.hpp"

#include <framework/colorconstants.hpp>
#include <framework/components/position.hpp>
#include <framework/entityworld2.hpp>

#include <RenderingKit/WorldGeometry.hpp>
#include <RenderingKit/utility/BasicPainter.hpp>
#include <RenderingKit/utility/TexturedPainter.hpp>

#include <vector>

namespace example
{
    using namespace RenderingKit;
    using std::make_unique;

    extern TexturedPainter3D<> s_tp;

    struct PointLightCommonData {
        BasicPainter2D<zfw::Float2> bp;

        std::vector<intptr_t> attachments;

        // Pre-pass - depth shader
        Resource<IMaterial> depthMaterial;

        // Deferred Pass - shader & uniforms
        Resource<IMaterial> deferredMaterial;
        intptr_t lightPos, lightAmbient, lightDiffuse, lightRange, cameraPos;
        intptr_t shadowMapIndex, shadowMatrix;
    };

    RenderSystem::RenderSystem(zfw::IEntityWorld2& ew, RenderingKit::IRenderingManager& rm, shared_ptr<RenderingKit::ICamera> cam) : ew(ew), rm(rm), cam(move(cam)) {
    }

    RenderSystem::~RenderSystem() {
    }

    void RenderSystem::DrawObjects() {
        ew.IterateEntitiesByComponent<SkyBox>([this](intptr_t entityId, SkyBox &component_data) {
            auto iter = skyboxes.find(entityId);

            ITexture* texture;

            if (iter == skyboxes.end()) {
                Resource<ITexture> res;
                zombie_assert(res.ByPath(component_data.texturePath.c_str()));
                texture = *res;
                skyboxes[entityId] = move(res);
            } else {
                texture = iter->second;
            }

            const Float3 size(500.0f, 500.0f, 500.0f);
            s_tp.DrawFilledCuboid(texture, size * 0.5f, -size, RGBA_WHITE);
        });

        ew.IterateEntitiesByComponent<WorldGeometry>([this](intptr_t entityId, WorldGeometry &component_data) {
            auto iter = worldGeoms.find(entityId);

            IWorldGeometry* worldGeom;

            if (iter == worldGeoms.end()) {
                Resource<IWorldGeometry> res;
                zombie_assert(res.ByRecipe(component_data.recipe.c_str()));
                worldGeom = *res;
                worldGeoms[entityId] = move(res);
            } else {
                worldGeom = iter->second;
            }

            worldGeom->Draw();
        });
    }

    void RenderSystem::OnDraw() {
        if (!useDeferred) {
            rm.ClearBuffers(true, true, false);
            rm.SetCamera(cam.get());
            rm.SetRenderState(RK_DEPTH_TEST, 1);

            this->DrawObjects();
        }
        else {
            if (!deferred) {
                // FIXME: this needs to react to WindowResize
                const Int2 res(1280, 720);

                // create Deferred Shading Manager
                deferred = rm.CreateDeferredShadingManager();
                deferred->AddBufferByte4(res, "diffuseBuf");
                deferred->AddBufferFloat4(res, "positionBuf");
                deferred->AddBufferFloat4(res, "normalsBuf");
            }

            // Iterate everything that needs a pre-pass
            ew.IterateEntitiesByComponent<PointLight>([this](intptr_t entityId, PointLight &component_data) {
                if (!pointLightCommon) {
                    pointLightCommon = this->p_InitializePointLightCommonData();
                }

                auto position = ew.GetEntityComponent<Position>(entityId);
                zombie_assert(position);

                auto iter = pointLights.find(entityId);

                PointLightRenderData* renderData;

                if (iter == pointLights.end()) {
                    auto renderDataNew = p_InitializePointLightRenderData(component_data, *position);
                    renderData = renderDataNew.get();
                    pointLights[entityId] = move(renderDataNew);
                } else {
                    renderData = iter->second.get();
                }

                rm.PushRenderBuffer(renderData->depthRenderBuffer.get());
                rm.ClearBuffers(false, true, false);

                rm.SetCamera(renderData->cam.get());
                rm.SetMaterialOverride(*pointLightCommon->depthMaterial);
                rm.SetRenderState(RK_DEPTH_TEST, 1);

                this->DrawObjects();

                rm.SetMaterialOverride(nullptr);

                rm.PopRenderBuffer();
            });

            deferred->BeginScene();

            rm.ClearBuffers(false, true, false);
            rm.SetCamera(cam.get());
            rm.SetRenderState(RK_DEPTH_TEST, 1);

            this->DrawObjects();

            deferred->EndScene();
            rm.SetRenderState(RK_DEPTH_TEST, 0);

            deferred->BeginDeferred();

            ew.IterateEntitiesByComponent<PointLight>([this](intptr_t entityId, PointLight &component_data) {
                auto position = ew.GetEntityComponent<Position>(entityId);
                zombie_assert(position);

                auto iter = pointLights.find(entityId);

                PointLightRenderData* renderData;

                assert(iter != pointLights.end());
                renderData = iter->second.get();

                glm::mat4x4 shadowMatrix;

                if (component_data.castsShadows)
                    renderData->cam->GetProjectionModelViewMatrix(&shadowMatrix);

                rm.VertexCacheFlush();

                auto deferredShader = pointLightCommon->deferredMaterial->GetShader();

                deferredShader->SetUniformFloat3(pointLightCommon->lightPos, component_data.pos);
                deferredShader->SetUniformFloat3(pointLightCommon->lightAmbient, component_data.ambient);
                deferredShader->SetUniformFloat3(pointLightCommon->lightDiffuse, component_data.diffuse);
                deferredShader->SetUniformFloat(pointLightCommon->lightRange, component_data.range);
                //shader->SetUniformFloat3(cameraPos, cameraPos);

                if (component_data.castsShadows) {
                    pointLightCommon->deferredMaterial->SetTextureByIndex(pointLightCommon->shadowMapIndex, renderData->depth.get());

                    deferredShader->SetUniformMat4x4(pointLightCommon->shadowMatrix, shadowMatrix);
                }

                pointLightCommon->bp.DrawFilledRectangle(Float2(-1.0f, -1.0f), Float2(2.0f, 2.0f), RGBA_WHITE);
            });

            deferred->EndDeferred();
        }
    }

    unique_ptr<PointLightCommonData> RenderSystem::p_InitializePointLightCommonData() {
        auto commonData = make_unique<PointLightCommonData>();

        commonData->depthMaterial.ByRecipe("shader=path=sandbox/depthonly", IResourceManager2::kResourcePrivate);

        commonData->deferredMaterial.ByRecipe("shader=path=sandbox/point", IResourceManager2::kResourcePrivate);

        // Set up texture inputs into deferred shader
        deferred->InjectTexturesInto(*commonData->deferredMaterial);
        commonData->shadowMapIndex = commonData->deferredMaterial->SetTexture("shadowMap", nullptr);

        auto deferredShader = commonData->deferredMaterial->GetShader();

        zombie_assert(commonData->bp.InitWithMaterial(&rm, commonData->deferredMaterial));

        commonData->lightPos = deferredShader->GetUniformLocation("lightPos");
        commonData->lightAmbient = deferredShader->GetUniformLocation("lightAmbient");
        commonData->lightDiffuse = deferredShader->GetUniformLocation("lightDiffuse");
        commonData->lightRange = deferredShader->GetUniformLocation("lightRange");

        commonData->cameraPos = deferredShader->GetUniformLocation("cameraPos");

        commonData->shadowMatrix = deferredShader->GetUniformLocation("shadowMatrix");

        return commonData;
    }

    unique_ptr<PointLightRenderData> RenderSystem::p_InitializePointLightRenderData(const PointLight& pointLight, const Position& position) {
        auto renderData = make_unique<PointLightRenderData>();

        renderData->cam = rm.CreateCamera("light_dynamic/cam");
        renderData->cam->SetClippingDist(0.2f, 20.0f);
        renderData->cam->SetPerspective();
        renderData->cam->SetVFov(f_pi * 0.75f);
        renderData->cam->SetView(position.pos, position.pos + Float3(0.0f, 0.0f, -1.0f), Float3(0.0f, -1.0f, 0.0f));

        renderData->depthRenderBuffer = rm.CreateRenderBuffer("light_dynamic/renderBuffer", Int2(1024, 1024),
                /*kRenderBufferColourTexture | */kRenderBufferDepthTexture);
        renderData->depth = renderData->depthRenderBuffer->GetDepthTexture();

        return renderData;
    }

}
