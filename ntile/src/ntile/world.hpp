#ifndef ntile_world_hpp
#define ntile_world_hpp

#include "ntile.hpp"

namespace ntile {
    struct WorldTile
    {
        uint8_t type;
        uint8_t flags;
        int16_t elev;
        uint8_t material;
        uint8_t colour[3];
    };

    static_assert(sizeof(WorldTile) == 8,           "WorldTile size must be 8");

    struct WorldBlock
    {
        int type;
        //unique_ptr<IVertexBuffer> vertexBuf;
        uint32_t pickingColour;

        List<IPointEntity*> entities;

        // 2k
        WorldTile tiles[TILES_IN_BLOCK_V][TILES_IN_BLOCK_H];

        void* priv_view = nullptr;
    };

    class Blocks
    {
        public:
            static void AllocBlocks(Int2 size, bool copyOld = false, Int2 copyOffset = Int2());
            static void ReleaseBlocks(WorldBlock*& blocks, Int2 worldSize);

            static void InitBlock(WorldBlock* block, int bx, int by);
            static void GenerateTiles(WorldBlock* block);
            static void ResetBlock(WorldBlock* block, int bx, int by);

            //static void UpdateTile(WorldTile* tile, WorldTile* tile_east, WorldTile* tile_south, WorldVertex*& p_vertices);
    };

    enum class BlockStateChange {
        created,
        updated,
        deleted,
    };

    enum {
        // TODO: just use typeID? do we care about serialization for these?
        kNanotileMinEvent = 0,
        kAnimationTrigger,
        kBlockStateChangeEvent,
        kWorldSwitched,
    };

    DECL_MESSAGE(AnimationTrigerEvent, kAnimationTrigger) {
        intptr_t entityId;
        const char* animationName;
    };

    DECL_MESSAGE(BlockStateChangeEvent, kBlockStateChangeEvent) {
        WorldBlock* block;
        BlockStateChange change;
    };

    DECL_MESSAGE(WorldSwitchedEvent, kWorldSwitched) {
    };

    // Utility functions
    inline Short2 WorldToBlockXY(Float2 worldPos)
    {
        return Short2((worldPos - Float2(TILE_SIZE_H * 0.5f, TILE_SIZE_V * 0.5f))
                      * Float2(1.0f / (TILES_IN_BLOCK_H * TILE_SIZE_H), 1.0f / (TILES_IN_BLOCK_V * TILE_SIZE_V)));
    }

    inline Int2 WorldToTileXY(Float2 worldPos)
    {
        return Int2((worldPos - Float2(TILE_SIZE_H * 0.5f, TILE_SIZE_V * 0.5f))
                    * Float2(1.0f / (TILES_IN_BLOCK_H * TILE_SIZE_H), 1.0f / (TILES_IN_BLOCK_V * TILE_SIZE_V)));
    }

    extern Int2 worldSize;
    extern WorldBlock* blocks;
    extern unique_ptr<IEntityWorld2> g_ew;
}

#endif
