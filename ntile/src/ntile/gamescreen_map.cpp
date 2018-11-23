
#include "gamescreen.hpp"
#include "world.hpp"

#include <framework/broadcasthandler.hpp>

namespace ntile
{
    int GameScreen::LoadMap(const char* map)
    {
        g_sys->Printf(kLogInfo, "GameScreen: Load map %s", map);

        // open map mediafile
        // FIXME: check for "..", "/" in 'map'
        unique_ptr<zshared::MediaFile> mapFile(zshared::MediaFile::Create());

        MyOpenFile of(sprintf_255("ntile/maps/%s.zmf", map));
        if (!mapFile->Open(&of, true, false))
        {
            g_sys->Printf(kLogError, "MediaFile error: %s", mapFile->GetErrorDesc());
            return EX_ASSET_OPEN_ERR;
        }

        // read metadata
        //const char* mapAuthor = mapFile->GetMetadata("media.authored_by");
        //g_sys->Printf(kLogInfo, "GameScreen: '%s' by %s", map, mapAuthor);

        // load blocks
        unique_ptr<InputStream> mapBlocks(mapFile->OpenSection("ntile.MapBlocks"));

        if (mapBlocks == nullptr)
        {
            g_sys->Printf(kLogError, "Map error: section 'ntile.MapBlocks' not found");
            return EX_ASSET_CORRUPTED;
        }

        uint16_t map_w, map_h;

        if (!mapBlocks->readLE<uint16_t>(&map_w) || !mapBlocks->readLE<uint16_t>(&map_h))
            return EX_ASSET_CORRUPTED;

        // TODO: something something broadcast all blocks destroyed

        Blocks::AllocBlocks(Int2(map_w, map_h));

        WorldBlock* p_block = &blocks[0];

        for (int by = 0; by < worldSize.y; by++)
            for (int bx = 0; bx < worldSize.x; bx++)
            {
                uint16_t type;

                if (!mapBlocks->readLE<uint16_t>(&type))
                    return EX_ASSET_CORRUPTED;

                p_block->type = type;

                switch (type)
                {
                    case BLOCK_WORLD:
                    {
                        if (mapBlocks->read(&p_block->tiles[0][0], TILES_IN_BLOCK_H * TILES_IN_BLOCK_V * sizeof(WorldTile)) != TILES_IN_BLOCK_H * TILES_IN_BLOCK_V * sizeof(WorldTile))
                            return EX_ASSET_CORRUPTED;

                        break;
                    }

                    case BLOCK_SHIROI_OUTSIDE:
                    {
                        int16_t minElev, maxElev;

                        if (!mapBlocks->readLE<int16_t>(&minElev) || !mapBlocks->readLE<int16_t>(&maxElev))
                            return EX_ASSET_CORRUPTED;

                        Blocks::GenerateTiles(p_block);

                        break;
                    }
                }

                BlockStateChangeEvent ev;
                ev.block = p_block;
                ev.change = BlockStateChange::created;
                g_sys->GetBroadcastHandler()->BroadcastMessage(ev);

                p_block++;
            }

        /*p_block = &blocks[0];

        for (int by = 0; by < worldSize.y; by++)
            for (int bx = 0; bx < worldSize.x; bx++)
            {
                p_block->vertexBuf.reset(ir->CreateVertexBuffer());
                p_block->vertexBuf->Alloc(TILES_IN_BLOCK_V * TILES_IN_BLOCK_H * 3 * 6 * sizeof(WorldVertex));
                Blocks::ResetBlock(p_block, bx, by);
                p_block++;
            }*/

        mapBlocks.reset();

        // load entities
        unique_ptr<InputStream> entities(mapFile->OpenSection("ntile.Entities"));
        
        if (entities == nullptr)
        {
            g_sys->Printf(kLogError, "Map error: section 'ntile.Entities' not found");
            return EX_ASSET_CORRUPTED;
        }
        
        g_sys->Printf(kLogInfo, "GameScreen: World loading...");
        NOT_IMPLEMENTED();
        // if (!world->Unserialize(entities.get(), 0))
        // {
        //     if (g_eb->errorCode == EX_OBJECT_UNDEFINED)
        //         ErrorBuffer::SetError(g_eb, EX_SERIALIZATION_ERR,
        //             "desc", sprintf_255("Failed to load map '%s'. The map is corrupted or was created by an incompatible version of the game.", map),
        //             "function", li_functionName,
        //             nullptr);

        //     g_sys->DisplayError(g_eb, false);
        //     return EX_ASSET_CORRUPTED;
        // }
        // else
        //     g_sys->Printf(kLogInfo, "GameScreen: World loading successful.");

        /*auto water = std::make_shared<entities::water_body>();
        water->SetPos(Float3(256.0f, 256.0f, 16.0f));
        zombie_assert(water->Init());
        world->AddEntity(water);*/

        entities.reset();

        // FIXME: EditingModeInit
        /*
            ICommonEntity* ice = ent->GetInterface<ICommonEntity>();

            if (ice != nullptr && g_allowEditingMode)
                ice->EditingModeInit();
        */

        return 0;
    }
}
