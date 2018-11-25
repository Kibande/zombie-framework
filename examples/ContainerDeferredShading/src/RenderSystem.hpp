#ifndef CONTAINERDEFERREDSHADING_RENDERSYSTEM_HPP
#define CONTAINERDEFERREDSHADING_RENDERSYSTEM_HPP

#include <framework/resourcemanager2.hpp>
#include <framework/system.hpp>

#include <RenderingKit/RenderingKit.hpp>

#include <unordered_map>

namespace example
{
    using namespace zfw;

    class PointLight;
    struct PointLightCommonData;

    struct PointLightRenderData {
        // Shadow-mapping
        shared_ptr<RenderingKit::ICamera> cam;
        shared_ptr<RenderingKit::ITexture> depth;
        shared_ptr<RenderingKit::IRenderBuffer> depthRenderBuffer;
    };

    class RenderSystem : public ISystem {
    public:
        RenderSystem(IEntityWorld2& ew, RenderingKit::IRenderingManager& rm, shared_ptr<RenderingKit::ICamera> cam);
        ~RenderSystem();

        void DrawObjects();
        void OnDraw();

    private:
        unique_ptr<PointLightCommonData> p_InitializePointLightCommonData();
        unique_ptr<PointLightRenderData> p_InitializePointLightRenderData(const PointLight& pointLight, const Position& position);

        IEntityWorld2& ew;
        RenderingKit::IRenderingManager& rm;
        shared_ptr<RenderingKit::ICamera> cam;

        std::unordered_map<intptr_t, Resource<RenderingKit::ITexture>> skyboxes;
        std::unordered_map<intptr_t, Resource<RenderingKit::IWorldGeometry>> worldGeoms;

        // Deferred rendering
        shared_ptr<RenderingKit::IDeferredShadingManager> deferred;
        unique_ptr<PointLightCommonData> pointLightCommon;

        std::unordered_map<intptr_t, unique_ptr<PointLightRenderData>> pointLights;
    };
}

#endif //CONTAINERDEFERREDSHADING_RENDERSYSTEM_HPP
