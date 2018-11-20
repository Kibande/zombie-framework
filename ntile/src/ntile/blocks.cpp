
#include "world.hpp"

namespace ntile
{
    Int2 worldSize;
    WorldBlock* blocks;

    void Blocks::AllocBlocks(Int2 size, bool copyOld, Int2 copyOffset)
    {
        const Int2 old_worldSize = worldSize;
        WorldBlock* old_blocks = blocks;

        if (!copyOld)
            ReleaseBlocks(old_blocks, old_worldSize);

        worldSize = size;
        blocks = Allocator<WorldBlock>::allocate(worldSize.x * worldSize.y);

        if (copyOld)
        {
            WorldBlock* p_block = &blocks[0];

            for (int by = 0; by < copyOffset.y; by++)
                for (int bx = 0; bx < worldSize.x; bx++)
                {
                    li::constructPointer(&p_block->entities);
                    p_block++;
                }

            for (int by = 0; by < old_worldSize.y; by++)
            {
                WorldBlock* oldBlocks = &old_blocks[by * old_worldSize.x];
                WorldBlock* p_newBlocks = &blocks[(by + copyOffset.y) * worldSize.x];

                for (int bx = 0; bx < copyOffset.x; bx++)
                {
                    li::constructPointer(&p_newBlocks->entities);
                    p_newBlocks++;
                }

                Allocator<WorldBlock>::move(p_newBlocks, oldBlocks, old_worldSize.x);
                p_newBlocks += old_worldSize.x;

                for (int bx = copyOffset.x + old_worldSize.x; bx < worldSize.x; bx++)
                {
                    li::constructPointer(&p_newBlocks->entities);
                    p_newBlocks++;
                }
            }

            p_block += old_worldSize.y * worldSize.x;

            for (int by = old_worldSize.y + copyOffset.y; by < worldSize.y; by++)
                for (int bx = 0; bx < worldSize.x; bx++)
                {
                    li::constructPointer(&p_block->entities);
                    p_block++;
                }

            Allocator<WorldTile>::release(old_blocks);
        }
        else
        {
            WorldBlock* p_block = &blocks[0];

            for (int by = 0; by < worldSize.y; by++)
                for (int bx = 0; bx < worldSize.x; bx++)
                {
                    li::constructPointer(&p_block->entities);
                    p_block++;
                }
        }
    }

    void Blocks::GenerateTiles(WorldBlock* block)
    {
        WorldTile* p_tile = &block->tiles[0][0];

        for (int i = 0; i < TILES_IN_BLOCK_V * TILES_IN_BLOCK_H; i++)
        {
            switch (block->type)
            {
                case BLOCK_SHIROI_OUTSIDE:
                    p_tile->type = 0;
                    p_tile->flags = 0;
                    p_tile->elev = 24 + (rand() % 8);
                    p_tile->material = 0;
                    p_tile->colour[0] = 0xFF;
                    p_tile->colour[1] = 0xFF;
                    p_tile->colour[2] = 0xFF;
                    break;

                case BLOCK_WORLD:
                    p_tile->type = 1;
                    p_tile->flags = 0;
                    p_tile->elev = -2 + (rand() % 5);
                    p_tile->material = 0;
                    p_tile->colour[0] = 0xFF;
                    p_tile->colour[1] = 0xFF;
                    p_tile->colour[2] = 0xFF;
                    break;
            }

            p_tile++;
        }
    }

    void Blocks::InitBlock(WorldBlock* block, int bx, int by)
    {
        Blocks::ResetBlock(block, bx, by);
    }

    void Blocks::ReleaseBlocks(WorldBlock*& blocks, Int2 worldSize)
    {
        WorldBlock* p_block = &blocks[0];

        for (int by = 0; by < worldSize.y; by++)
            for (int bx = 0; bx < worldSize.x; bx++)
            {
                li::destructPointer(&p_block->entities);


                p_block++;
            }

        Allocator<WorldTile>::release(blocks);
    }

    void Blocks::ResetBlock(WorldBlock* block, int bx, int by)
    {
    }
}
