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
        int32_t x, y, z;
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

    class DrawableViewer : public IViewer {
    public:
        void Draw(RenderingKit::IRenderingManager* rm, IEntityWorld2* world, intptr_t entityId);

    private:
        unique_ptr<CharacterModel> model;
    };
}

#endif
