#include "view.hpp"

#include "../world.hpp"

namespace ntile {
    static const Normal_t normal_up[4] =    { 0, 0, INT16_MAX, 0 };
    static const Normal_t normal_east[4] =  { INT16_MAX, 0, 0, 0 };
    static const Normal_t normal_south[4] = { 0, -INT16_MAX, 0, 0 };

    static const Normal_t normal_max = 32767;
    static const Normal_t normal_min = -32768;

    void InitAllTiles(Short2 blockXY, WorldVertex* p_vertices) {
        //WorldBlock* block = &blocks[blockXY.y * worldSize.x + blockXY.x];

        for (int y = 0; y < TILES_IN_BLOCK_V; y++) {
            int32_t xx1 = TILE_SIZE_H / 2 + blockXY.x * TILES_IN_BLOCK_H * TILE_SIZE_H;
            int32_t xx2 = xx1 + TILE_SIZE_H;

            const int32_t yy1 = TILE_SIZE_V / 2 + (blockXY.y * TILES_IN_BLOCK_V + y) * TILE_SIZE_V;
            const int32_t yy2 = yy1 + TILE_SIZE_V;

            for (int x = 0; x < TILES_IN_BLOCK_H; x++) {
                for (int i = 0; i < 18; i++)
                    memset(&p_vertices[i].rgba[0], 0xFF, 4);

                // this
                memcpy(p_vertices[0].n, normal_up, sizeof(normal_up));
                memcpy(p_vertices[1].n, normal_up, sizeof(normal_up));
                memcpy(p_vertices[2].n, normal_up, sizeof(normal_up));
                memcpy(p_vertices[3].n, normal_up, sizeof(normal_up));
                memcpy(p_vertices[4].n, normal_up, sizeof(normal_up));
                memcpy(p_vertices[5].n, normal_up, sizeof(normal_up));

                p_vertices[0].x = xx1;
                p_vertices[0].y = yy1;
                p_vertices[0].u = 0.0f;
                p_vertices[0].v = 0.0f;

                p_vertices[1].x = xx2;
                p_vertices[1].y = yy2;
                p_vertices[1].u = 1.0f;
                p_vertices[1].v = 1.0f;

                p_vertices[2].x = xx1;
                p_vertices[2].y = yy2;
                p_vertices[2].u = 0.0f;
                p_vertices[2].v = 1.0f;

                p_vertices[3].x = xx2;
                p_vertices[3].y = yy2;
                p_vertices[3].u = 1.0f;
                p_vertices[3].v = 1.0f;

                p_vertices[4].x = xx1;
                p_vertices[4].y = yy1;
                p_vertices[4].u = 0.0f;
                p_vertices[4].v = 0.0f;

                p_vertices[5].x = xx2;
                p_vertices[5].y = yy1;
                p_vertices[5].u = 1.0f;
                p_vertices[5].v = 0.0f;

                // eastern
                memcpy(p_vertices[6].n, normal_east, sizeof(normal_east));
                memcpy(p_vertices[7].n, normal_east, sizeof(normal_east));
                memcpy(p_vertices[8].n, normal_east, sizeof(normal_east));
                memcpy(p_vertices[9].n, normal_east, sizeof(normal_east));
                memcpy(p_vertices[10].n, normal_east, sizeof(normal_east));
                memcpy(p_vertices[11].n, normal_east, sizeof(normal_east));

                p_vertices[6].x = xx2;
                p_vertices[6].y = yy2;
                p_vertices[6].u = 0.0f;
                p_vertices[6].v = 0.0f;

                p_vertices[7].x = xx2;
                p_vertices[7].y = yy1;
                p_vertices[7].u = 1.0f;
                p_vertices[7].v = 1.0f;

                p_vertices[8].x = xx2;
                p_vertices[8].y = yy2;
                p_vertices[8].u = 0.0f;
                p_vertices[8].v = 1.0f;

                p_vertices[9].x = xx2;
                p_vertices[9].y = yy1;
                p_vertices[9].u = 1.0f;
                p_vertices[9].v = 1.0f;

                p_vertices[10].x = xx2;
                p_vertices[10].y = yy2;
                p_vertices[10].u = 0.0f;
                p_vertices[10].v = 0.0f;

                p_vertices[11].x = xx2;
                p_vertices[11].y = yy1;
                p_vertices[11].u = 1.0f;
                p_vertices[11].v = 0.0f;

                // southern
                memcpy(p_vertices[12].n, normal_south, sizeof(normal_south));
                memcpy(p_vertices[13].n, normal_south, sizeof(normal_south));
                memcpy(p_vertices[14].n, normal_south, sizeof(normal_south));
                memcpy(p_vertices[15].n, normal_south, sizeof(normal_south));
                memcpy(p_vertices[16].n, normal_south, sizeof(normal_south));
                memcpy(p_vertices[17].n, normal_south, sizeof(normal_south));

                p_vertices[12].x = xx1;
                p_vertices[12].y = yy2;
                p_vertices[12].u = 0.0f;
                p_vertices[12].v = 0.0f;

                p_vertices[13].x = xx2;
                p_vertices[13].y = yy2;
                p_vertices[13].u = 1.0f;
                p_vertices[13].v = 1.0f;

                p_vertices[14].x = xx1;
                p_vertices[14].y = yy2;
                p_vertices[14].u = 0.0f;
                p_vertices[14].v = 1.0f;

                p_vertices[15].x = xx2;
                p_vertices[15].y = yy2;
                p_vertices[15].u = 1.0f;
                p_vertices[15].v = 1.0f;

                p_vertices[16].x = xx1;
                p_vertices[16].y = yy2;
                p_vertices[16].u = 0.0f;
                p_vertices[16].v = 0.0f;

                p_vertices[17].x = xx2;
                p_vertices[17].y = yy2;
                p_vertices[17].u = 1.0f;
                p_vertices[17].v = 0.0f;

                p_vertices += 18;

                xx1 += 16;
                xx2 += 16;
            }
        }
    }

    void UpdateTile(WorldTile* tile, WorldTile* tile_east, WorldTile* tile_south, WorldVertex*& p_vertices)
    {
        for (int i = 0; i < 6; i++)
        {
            p_vertices[i].z = tile->elev;

            memcpy(&p_vertices[i].rgba[0], &tile->colour[0], 3);
        }

        p_vertices += 6;

        Normal_t normal_x;

        if (tile_east != nullptr)
        {
            normal_x = (tile->elev > tile_east->elev) ? normal_max : normal_min;

            memcpy(&p_vertices[0].rgba[0], &tile->colour[0], 3);
            memcpy(&p_vertices[1].rgba[0], &tile_east->colour[0], 3);
            memcpy(&p_vertices[2].rgba[0], &tile_east->colour[0], 3);

            memcpy(&p_vertices[3].rgba[0], &tile_east->colour[0], 3);
            memcpy(&p_vertices[4].rgba[0], &tile->colour[0], 3);
            memcpy(&p_vertices[5].rgba[0], &tile->colour[0], 3);

            p_vertices[0].z = tile->elev;
            p_vertices[1].z = tile_east->elev;
            p_vertices[2].z = tile_east->elev;

            p_vertices[3].z = tile_east->elev;
            p_vertices[4].z = tile->elev;
            p_vertices[5].z = tile->elev;
        }
        else
        {
            normal_x = (tile->elev > 0) ? normal_max : normal_min;

            memcpy(&p_vertices[0].rgba[0], &tile->colour[0], 3);
            memcpy(&p_vertices[1].rgba[0], &tile->colour[0], 3);
            memcpy(&p_vertices[2].rgba[0], &tile->colour[0], 3);

            memcpy(&p_vertices[3].rgba[0], &tile->colour[0], 3);
            memcpy(&p_vertices[4].rgba[0], &tile->colour[0], 3);
            memcpy(&p_vertices[5].rgba[0], &tile->colour[0], 3);

            p_vertices[0].z = tile->elev;
            p_vertices[1].z = 0;
            p_vertices[2].z = 0;

            p_vertices[3].z = 0;
            p_vertices[4].z = tile->elev;
            p_vertices[5].z = tile->elev;
        }

        p_vertices[0].n[0] = normal_x;
        p_vertices[1].n[0] = normal_x;
        p_vertices[2].n[0] = normal_x;
        p_vertices[3].n[0] = normal_x;
        p_vertices[4].n[0] = normal_x;
        p_vertices[5].n[0] = normal_x;

        p_vertices += 6;

        Normal_t normal_y;

        if (tile_south != nullptr)
        {
            normal_y = (tile->elev > tile_south->elev) ? normal_max : normal_min;

            memcpy(&p_vertices[0].rgba[0], &tile->colour[0], 3);
            memcpy(&p_vertices[1].rgba[0], &tile_south->colour[0], 3);
            memcpy(&p_vertices[2].rgba[0], &tile_south->colour[0], 3);

            memcpy(&p_vertices[3].rgba[0], &tile_south->colour[0], 3);
            memcpy(&p_vertices[4].rgba[0], &tile->colour[0], 3);
            memcpy(&p_vertices[5].rgba[0], &tile->colour[0], 3);

            p_vertices[0].z = tile->elev;
            p_vertices[1].z = tile_south->elev;
            p_vertices[2].z = tile_south->elev;

            p_vertices[3].z = tile_south->elev;
            p_vertices[4].z = tile->elev;
            p_vertices[5].z = tile->elev;
        }
        else
        {
            normal_y = (tile->elev > 0) ? normal_max : normal_min;

            memcpy(&p_vertices[0].rgba[0], &tile->colour[0], 3);
            memcpy(&p_vertices[1].rgba[0], &tile->colour[0], 3);
            memcpy(&p_vertices[2].rgba[0], &tile->colour[0], 3);
            memcpy(&p_vertices[3].rgba[0], &tile->colour[0], 3);
            memcpy(&p_vertices[4].rgba[0], &tile->colour[0], 3);
            memcpy(&p_vertices[5].rgba[0], &tile->colour[0], 3);

            p_vertices[0].z = tile->elev;
            p_vertices[1].z = 0;
            p_vertices[2].z = 0;

            p_vertices[3].z = 0;
            p_vertices[4].z = tile->elev;
            p_vertices[5].z = tile->elev;
        }

        p_vertices[0].n[1] = normal_y;
        p_vertices[1].n[1] = normal_y;
        p_vertices[2].n[1] = normal_y;
        p_vertices[3].n[1] = normal_y;
        p_vertices[4].n[1] = normal_y;
        p_vertices[5].n[1] = normal_y;

        p_vertices += 6;
    }

    void UpdateAllTiles(Short2 blockXY, WorldVertex* p_vertices)
    {
        WorldBlock* block = &blocks[blockXY.y * worldSize.x + blockXY.x];
        WorldBlock* block_east = (blockXY.x + 1 < worldSize.x) ? &blocks[blockXY.y * worldSize.x + blockXY.x + 1] : nullptr;
        WorldBlock* block_south = (blockXY.y + 1 < worldSize.y) ? &blocks[(blockXY.y + 1) * worldSize.x + blockXY.x] : nullptr;

        for (int y = 0; y < TILES_IN_BLOCK_V; y++)
        {
            WorldTile* p_tile = &block->tiles[y][0];
            WorldTile* tile_east = (block_east != nullptr) ? &block_east->tiles[y][0] : nullptr;

            WorldTile* p_tile_south;

            if (y + 1 < TILES_IN_BLOCK_V)
                p_tile_south = &block->tiles[y + 1][0];
            else
                p_tile_south = (block_south != nullptr) ? &block_south->tiles[0][0] : nullptr;

            if (p_tile_south != nullptr)
            {
                for (int x = 0; x < TILES_IN_BLOCK_H - 1; x++)
                {
                    UpdateTile(p_tile, p_tile + 1, p_tile_south, p_vertices);

                    p_tile++;
                    p_tile_south++;
                }

                UpdateTile(p_tile, tile_east, p_tile_south, p_vertices);
            }
            else
            {
                for (int x = 0; x < TILES_IN_BLOCK_H - 1; x++)
                {
                    UpdateTile(p_tile, p_tile + 1, nullptr, p_vertices);

                    p_tile++;
                }

                UpdateTile(p_tile, tile_east, nullptr, p_vertices);
            }
        }
    }

    void BlockViewer::Draw(RenderingKit::IRenderingManager* rm, Int2 blockXY, WorldBlock* block, RenderingKit::IMaterial* mat) {
        if (!gc) {
            if (!gb) {
                gb = rm->CreateGeomBuffer("blockviewer");       // TODO: share!
            }

            gc = gb->AllocVertices(WorldVertex::format, TILES_IN_BLOCK_V * TILES_IN_BLOCK_H * 3 * 6, 0);
            zombie_assert(gc);

            InitAllTiles(blockXY, vertices);

            gc->UpdateVertices(0, vertices, sizeof(vertices));
        }

        if (needsUpdate) {
            UpdateAllTiles(blockXY, vertices);

            gc->UpdateVertices(0, vertices, sizeof(vertices));

            needsUpdate = false;
        }

        rm->DrawPrimitives(mat, RenderingKit::RK_TRIANGLES, gc.get());
    }
}