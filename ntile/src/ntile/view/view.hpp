#ifndef ntile_view_hpp
#define ntile_view_hpp

#include "ntile_model.hpp"

#include "../ntile.hpp"

#include <RenderingKit/RenderingKit.hpp>
#include <RenderingKit/utility/VertexFormat.hpp>

namespace ntile {
    extern RenderingKit::IRenderingManager* irm;

    struct WorldVertex
    {
        float x, y, z;
        int16_t n[4];
        uint8_t rgba[4];
        float u, v;

        static RenderingKit::VertexFormat<4> format;
    };

    static_assert(sizeof(WorldVertex) == 32, "WorldVertex size");

    class IViewer {
    public:
        //...virtual void OnObjectStateChange(void* object, intptr_t state, intptr_t value) {}
    };

    class BlockViewer : public IViewer {
    public:
        void Draw(RenderingKit::IRenderingManager* rm, Int2 blockXY, WorldBlock* block, RenderingKit::IMaterial* mat);

        void Update() { this->needsUpdate = true; }

    private:
        bool needsUpdate = true;

        WorldVertex vertices[TILES_IN_BLOCK_V * TILES_IN_BLOCK_H * 3 * 6];

        shared_ptr<RenderingKit::IGeomBuffer> gb;
        unique_ptr<RenderingKit::IGeomChunk> gc;
    };

    // The irony of this class; its instances are currently scattered around memory, negating any caching advantage of ECS
    class DrawableViewer : public IViewer {
    public:
        void Draw(RenderingKit::IRenderingManager* rm, IEntityWorld2* world, intptr_t entityId);
        void OnTicks(int ticks);
        void Realize(IEntityWorld2& world, intptr_t entityId, IResourceManager2& res);
        void TriggerAnimation(const char* animationName);

    private:
        bool p_TryLoadBlockyModel();
        bool p_TryLoadModel(IResourceManager2 &res);

        Model3D* model3d = nullptr;
        Position* position = nullptr;

        bool mustReload = true;

        unique_ptr<CharacterModel> blockyModel;
        RenderingKit::IModel* model;
    };
}

#endif
