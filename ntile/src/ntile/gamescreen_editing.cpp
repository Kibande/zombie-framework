
#include "gamescreen.hpp"

#include <framework/colorconstants.hpp>
#include <framework/varsystem.hpp>

namespace ntile
{
    enum { g_allowEditingMode = 1 };

    static const int Edit_blockTypes[] =
    {
        BLOCK_WORLD,
        BLOCK_SHIROI_OUTSIDE,
    };

    extern const char* controlNames[];

    int GameScreen::Edit_CreateMap()
    {
        auto var = g_sys->GetVarSystem();

        const char* map = var->GetVariableOrEmptyString("startmap");
        var->SetVariable("map", map, 0);

        unique_ptr<zshared::MediaFile> mapFile(zshared::MediaFile::Create());

        MyOpenFile of(sprintf_255("ntile/maps/%s.zmf", map));
        if (!mapFile->Open(&of, false, true))
        {
            g_sys->Printf(kLogError, "MediaFile error: %s", mapFile->GetErrorDesc());
            return EX_WRITE_ERR;
        }

        mapFile->SetMetadata("media.original_name", map);
        mapFile->SetMetadata("media.authored_by", var->GetVariableOrEmptyString("map_author"));
        mapFile->SetMetadata("media.authored_using", "name=" APP_TITLE ",version=" APP_VERSION ",vendor=" APP_VENDOR);

        // Create an empty SHIROI_OUTSIDE block
        unique_ptr<OutputStream> mapBlocks(mapFile->OpenOrCreateSection("ntile.MapBlocks"));
        mapBlocks->writeLE<uint16_t>(1);
        mapBlocks->writeLE<uint16_t>(1);
        mapBlocks->writeLE<uint16_t>(BLOCK_SHIROI_OUTSIDE);
        mapBlocks->writeLE<int16_t>(0);
        mapBlocks->writeLE<int16_t>(0);
        mapBlocks.reset();

        unique_ptr<OutputStream> entities(mapFile->OpenOrCreateSection("ntile.Entities"));
        
        if (!world->Serialize(entities.get(), 0))
            g_sys->DisplayError(g_eb, false);

        entities.reset();
        
        return 0;
    }

    void GameScreen::Edit_InitEditingUI()
    {
        // this is called on map startup! (the widgets are invisible by default)
        // TODO: do this when editing mode is activated for the first time instead

        ui->SetOnlineUpdate(false);

        gameui::UILoader ldr(g_sys, ui.get(), &uiThemer);

        if (!ldr.Load("ntile/ui/editing.cfx2", ui.get(), true))
            g_sys->DisplayError(g_eb, false);
        else
        {
            editing_panel = dynamic_cast<gameui::Panel*>(ui->Find("editing_panel"));

            if (editing_panel != nullptr)
                editing_panel->SetVisible(editingMode);

            editing_selected_block = dynamic_cast<gameui::StaticText*>(ui->Find("editing_selected_block"));
            editing_block_type = dynamic_cast<gameui::ComboBox*>(ui->Find("editing_block_type"));

            if (editing_block_type != nullptr)
            {
                editing_block_type->Add(new gameui::StaticText(&uiThemer, "BLOCK_WORLD"));
                editing_block_type->Add(new gameui::StaticText(&uiThemer, "BLOCK_SHIROI_OUTSIDE"));
            }

            editing_selected_entity = dynamic_cast<gameui::StaticText*>(ui->Find("editing_selected_entity"));
            editing_entity_properties = dynamic_cast<gameui::TableLayout*>(ui->Find("editing_entity_properties"));
            editing_entity_class = dynamic_cast<gameui::ComboBox*>(ui->Find("editing_entity_class"));

            if (editing_entity_class != nullptr)
            {
                IEntityHandler* ieh = g_sys->GetEntityHandler(true);
                ieh->IterateEntityClasses(this);
            }

            editing_toolbar = dynamic_cast<gameui::StaticImage*>(ui->Find("editing_toolbar"));

            if (editing_toolbar != nullptr)
                editing_toolbar->SetVisible(editingMode);
        }

        ui->Layout();
        ui->SetOnlineUpdate(true);
    }

    void GameScreen::Edit_InsertBlock(Short2 pos, int type)
    {
        const Int2 oldWorldSize = worldSize;
        Int2 newWorldSize = worldSize;
        Int2 copyOffset;

        if (selectedBlock.x < 0)
        {
            selectedBlock.x = 0;
            newWorldSize.x++;
            copyOffset.x = 1;
        }
        else if (selectedBlock.x >= oldWorldSize.x)
            newWorldSize.x++;

        if (selectedBlock.y < 0)
        {
            selectedBlock.y = 0;
            newWorldSize.y++;
            copyOffset.y = 1;
        }
        else if (selectedBlock.y >= oldWorldSize.y)
            newWorldSize.y++;

        Blocks::AllocBlocks(newWorldSize, true, copyOffset);

        WorldBlock* p_block = &blocks[0];

        for (int by = 0; by < worldSize.y; by++)
            for (int bx = 0; bx < worldSize.x; bx++)
            {
                if (bx < copyOffset.x || bx >= oldWorldSize.x + copyOffset.x
                    || by < copyOffset.y || by >= oldWorldSize.y + copyOffset.y)
                {
                    if (bx == selectedBlock.x && by == selectedBlock.y)
                        p_block->type = Edit_blockTypes[type];
                    else
                        p_block->type = BLOCK_SHIROI_OUTSIDE;

                    Blocks::GenerateTiles(p_block);
                    Blocks::InitBlock(p_block, bx, by);
                }
                else
                    Blocks::ResetBlock(p_block, bx, by);

                p_block++;
            }

        if (copyOffset.x > 0 || copyOffset.y > 0)
        {
            const Float3 worldOffset(copyOffset.x * 256.0f, copyOffset.y * 256.0f, 0.0f);
            camPos += worldOffset;

            iterate2 (i, world->GetEntityList())
                (*i)->SetPos((*i)->GetPos() + worldOffset);
        }
    }

    int GameScreen::Edit_SaveMap()
    {
        const char* map = g_sys->GetVarSystem()->GetVariableOrEmptyString("map");

        unique_ptr<zshared::MediaFile> mapFile(zshared::MediaFile::Create());

        MyOpenFile of(sprintf_255("ntile/maps/%s.zmf", map));
        if (!mapFile->Open(&of, false, true))
            return EX_WRITE_ERR;

        mapFile->SetMetadata("media.authored_using", "name=" APP_TITLE ",version=" APP_VERSION ",vendor=" APP_VENDOR);

        // ntile.MapBlocks
        unique_ptr<OutputStream> mapBlocks(mapFile->OpenOrCreateSection("ntile.MapBlocks"));

        if (mapBlocks == nullptr)
            return EX_ASSET_CORRUPTED;

        if (!mapBlocks->writeLE<uint16_t>(worldSize.x)
                || !mapBlocks->writeLE<uint16_t>(worldSize.y))
            return EX_WRITE_ERR;

        WorldBlock* p_block = &blocks[0];

        for (int by = 0; by < worldSize.y; by++)
            for (int bx = 0; bx < worldSize.x; bx++)
            {
                if (!mapBlocks->writeLE<uint16_t>(p_block->type))
                    return EX_WRITE_ERR;

                switch (p_block->type)
                {
                    case BLOCK_WORLD:
                        if (mapBlocks->write(&p_block->tiles[0][0], TILES_IN_BLOCK_H * TILES_IN_BLOCK_V * sizeof(WorldTile)) != TILES_IN_BLOCK_H * TILES_IN_BLOCK_V * sizeof(WorldTile))
                            return EX_WRITE_ERR;
                        break;

                    case BLOCK_SHIROI_OUTSIDE:
                        if (!mapBlocks->writeLE<int16_t>(0) || !mapBlocks->writeLE<int16_t>(0))
                            return EX_WRITE_ERR;
                        break;
                }

                p_block++;
            }

        mapBlocks.reset();

        // ntile.Entities
        unique_ptr<OutputStream> entities(mapFile->OpenOrCreateSection("ntile.Entities"));
        
        if (!entities)
            return EX_ASSET_CORRUPTED;
        
        world->Serialize(entities.get(), 0);
        
        entities.reset();
        
        return 0;
    }

    void GameScreen::Edit_SelectBlock(Short2 blockXY)
    {
        selectedBlock = mouseBlock;

        if (editing_selected_block == nullptr || editing_block_type == nullptr)
            return;

        if (selectedBlock.x == INT16_MIN || selectedBlock.y == INT16_MIN)
        {
            editing_selected_block->SetLabel("---");
            editing_block_type->SetSelection(-1);
            return;
        }

        int type = -1, selection = -1;

        if (selectedBlock.x >= 0 && selectedBlock.y >= 0
                && selectedBlock.x < worldSize.x && selectedBlock.y < worldSize.y)
        {
            type = blocks[selectedBlock.y * worldSize.x + selectedBlock.x].type;
        }

        for (int i = 0; type != -1 && i < lengthof(Edit_blockTypes); i++)
        {
            if (Edit_blockTypes[i] == type)
            {
                selection = i;
                break;
            }
        }

        editing_selected_block->SetLabel(sprintf_63("%i, %i (%i)", selectedBlock.x, selectedBlock.y, type));
        editing_block_type->SetSelection(selection);
    }

    void GameScreen::Edit_SelectEntity(IEntity* entity)
    {
        selectedEntity = dynamic_cast<IEntityReflection*>(entity);

        if (editing_selected_entity == nullptr || editing_entity_properties == nullptr)
            return;

        if (selectedEntity != nullptr)
        {
            editing_selected_entity->SetLabel(selectedEntity->GetEntity()->GetName());

            const size_t numProperties = selectedEntity->GetNumProperties();
            entityProperties.setLength(numProperties, true);
            selectedEntity->GetRWProperties(entityProperties.c_array(), numProperties);

            editing_entity_properties->RemoveAll();

            for (size_t i = 0; i < numProperties; i++)
            {
                auto label = new gameui::StaticText(&uiThemer, entityProperties[i].desc.name);
                label->SetAlign(ALIGN_LEFT | ALIGN_VCENTER);
                label->SetExpands(false);

                gameui::Widget* value = new gameui::Button(&uiThemer, DataModel::ToString(entityProperties[i]));
                value->SetAlign(ALIGN_RIGHT | ALIGN_VCENTER);
                value->SetExpands(false);
                value->SetName(sprintf_255("ed_%s", entityProperties[i].desc.name));

                editing_entity_properties->Add(label);
                editing_entity_properties->Add(value);
            }
        }
        else
        {
            editing_selected_entity->SetLabel("---");
            editing_entity_properties->RemoveAll();
        }
    }

    template <bool randomize>
    void GameScreen::Edit_CircleTerrainChange(Int2 tileXY, int radius, int raise, int min)
    {
        for (int yy = std::max<int>(tileXY.y - radius, 0); yy <= tileXY.y + radius; yy++)
        {
            const int by = yy / TILES_IN_BLOCK_V;

            if (by >= worldSize.y)
                break;

            const int y = yy - tileXY.y;
            const int x = (int)(sqrt(radius * radius - y * y) + 0.5f);

            const int minX = std::max<int>(tileXY.x - x, 0);

            WorldBlock* block = nullptr;
            WorldTile* p_tile = nullptr, * p_tile_east = nullptr, * p_tile_south = nullptr;

            for (int xx = minX; xx <= tileXY.x + x; xx++)
            {
                if (p_tile == nullptr || xx % TILES_IN_BLOCK_H == 0)
                {
                    const int bx = xx / TILES_IN_BLOCK_H;

                    if (bx >= worldSize.x)
                        break;

                    block = &blocks[by * worldSize.x + bx];

                    if (block->type != BLOCK_WORLD)
                    {
                        xx = (bx + 1) * TILES_IN_BLOCK_H - 1;   // -1 compensates for 'xx++'
                        continue;
                    }

                    p_tile = &block->tiles[yy % TILES_IN_BLOCK_V][xx % TILES_IN_BLOCK_H];
                    p_tile_east = &block->tiles[yy % TILES_IN_BLOCK_V][xx % TILES_IN_BLOCK_H + 1];

                    if (yy % TILES_IN_BLOCK_V == TILES_IN_BLOCK_V - 1)
                    {
                        WorldBlock* block_south = (by + 1 < worldSize.x) ? &blocks[(by + 1) * worldSize.x + bx] : nullptr;
                        p_tile_south = &block_south->tiles[0][xx % TILES_IN_BLOCK_H];
                    }
                    else
                        p_tile_south = &block->tiles[yy % TILES_IN_BLOCK_V + 1][xx % TILES_IN_BLOCK_H];
                }

                if (xx % TILES_IN_BLOCK_H == TILES_IN_BLOCK_H - 1)
                {
                    const int bx = xx / TILES_IN_BLOCK_H;

                    WorldBlock* block_east = (bx + 1 < worldSize.x) ? &blocks[by * worldSize.x + (bx + 1)] : nullptr;
                    p_tile_east = (block_east != nullptr) ? &block_east->tiles[yy % TILES_IN_BLOCK_V][0] : nullptr;
                }

                if (!randomize)
                    p_tile->elev += raise;
                else
                    p_tile->elev = min + (rand() % raise);

                p_tile++;

                // Wow! This might be a nullptr! (if we're on the edge of the map)
                // In that case though, it will be reset in the next iteration anyway
                p_tile_east++;

                if (p_tile_south != nullptr)
                    p_tile_south++;
            }
        }

        const int minBlockX = std::max<int>((tileXY.x - radius) / TILES_IN_BLOCK_H, 0);
        const int maxBlockX = std::min<int>((tileXY.x + radius) / TILES_IN_BLOCK_H, worldSize.x - 1);

        const int minBlockY = std::max<int>((tileXY.y - radius) / TILES_IN_BLOCK_V, 0);
        const int maxBlockY = std::min<int>((tileXY.y + radius) / TILES_IN_BLOCK_V, worldSize.y - 1);

        for (int by = minBlockY; by <= maxBlockY; by++)
        {
            WorldBlock* p_block = &blocks[by * worldSize.x + minBlockX];

            for (int bx = minBlockX; bx <= maxBlockX; bx++)
            {
                auto vertices = static_cast<WorldVertex*>(p_block->vertexBuf->Map(false, true));

                Blocks::InitAllTiles(Short2(bx, by), vertices);
                Blocks::UpdateAllTiles(Short2(bx, by), vertices);

                p_block->vertexBuf->Unmap();
                p_block++;
            }
        }
    }

    void GameScreen::Edit_ToolEdit(Int3 worldPos)
    {
        const Int2 tileXY = Int2((worldPos.x - TILE_SIZE_H / 2) / TILE_SIZE_H, (worldPos.y - TILE_SIZE_V / 2) / TILE_SIZE_V);

        int k = 4;

        switch (editing.tool)
        {
            case TOOL_RAISE:
                if (editing.toolShape == TOOL_CIRCLE)
                    Edit_CircleTerrainChange<false>(tileXY, editing.toolRadius / 2, k, 0);
                break;

            case TOOL_LOWER:
                if (editing.toolShape == TOOL_CIRCLE)
                    Edit_CircleTerrainChange<false>(tileXY, editing.toolRadius / 2, -k, 0);
                break;

            case TOOL_RANDOMIZE:
                if (editing.toolShape == TOOL_CIRCLE)
                    Edit_CircleTerrainChange<true>(tileXY, editing.toolRadius / 2, 8, 24);
                break;
        }
    }

    int GameScreen::OnEntityClass(const char* className)
    {
        editing_entity_class->Add(new gameui::StaticText(&uiThemer, className));
        return 1;
    }

    void GameScreen::OnEventControlUsed(gameui::EventControlUsed* payload)
    {
        if (payload->widget->GetNameString().isEmpty())
            return;

        if (payload->widget->GetNameString() == "new_game")
        {
            auto var = g_sys->GetVarSystem();
            var->SetVariable("map", var->GetVariableOrEmptyString("startmap"), 0);
            StartGame();
        }
        else if (payload->widget->GetNameString() == "set_controls")
        {
            gameui::Panel* panel = new gameui::Panel(&uiThemer);
            gameui::Table* table = new gameui::Table(2);
                table->Add(new gameui::StaticText(&uiThemer, "Press key for: "));
                setControlsControl = new gameui::StaticText(&uiThemer, controlNames[setControlsIndex = 0]);
                table->Add(setControlsControl);
            panel->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
            panel->SetColour(COLOUR_GREY(0.3f, 0.9f));
            panel->SetExpands(false);
            panel->SetPadding(Int2(25, 25));
            panel->Add(table);
            ui->PushModal(panel);
        }
        else if (payload->widget->GetNameString() == "quit")
            g_sys->StopMainLoop();
        else if (payload->widget->GetNameString() == "dev_newmap")
        {
            int rc = Edit_CreateMap();

            // TODO: wat do
            /*if (rc != 0)
                Sys::ApplicationError(rc,
                        "desc", Sys::tmpsprintf(200, "Failed to create a new map."),
                        "function", li_functionName,
                        nullptr);*/

            if (rc == 0)
                StartGame();
        }
        else if (payload->widget->GetNameString() == "editing_savemap")
        {
            int rc = Edit_SaveMap();

            g_sys->Printf(kLogDebugInfo, "Edit_SaveMap: rc=%i", rc);
        }
        else if (payload->widget->GetNameString() == "editing_move_entity")
        {
            ZFW_ASSERT(selectedEntity != nullptr)

            edit_movingEntity = true;
            edit_entityMovementOrigin = r_mousePos;
            edit_entityInitialPos = selectedEntity->GetEntity()->GetPos();

            // FIXME: Disgusting hack
            //ui->OnMouseButton(-1, 0, true, 0, 0);
        }
        else if (payload->widget->GetNameString() == "editing_delete_entity")
        {
            ZFW_ASSERT(selectedEntity != nullptr)

            world->RemoveEntity(selectedEntity->GetEntity());
            selectedEntity = nullptr;
            Edit_SelectEntity(nullptr);

            // FIXME: Disgusting hack
            //ui->OnMouseButton(-1, 0, true, 0, 0);
        }
        else if (payload->widget->GetNameString() == "editing_property_ok" || payload->widget->GetNameString() == "editing_property_cancel")
        {
            gameui::TextBox* value;

            if (payload->widget->GetNameString() == "editing_property_ok"
                    && (value = dynamic_cast<gameui::TextBox*>(ui->Find("editing_property_value"))) != nullptr)
                DataModel::UpdateFromString(*editedProperty, value->GetText());

            ui->PopModal(true);
        }
        else if (strncmp(payload->widget->GetNameString(), "ed_", 3) == 0 && selectedEntity != nullptr)
        {
            iterate2 (i, entityProperties)
            {
                if (strcmp(payload->widget->GetName() + 3, (*i).desc.name) == 0)
                {
                    // TODO: Declare this in a .cfx2

                    editedProperty = &(*i);

                    gameui::Popup* popup = new gameui::Popup(&uiThemer);
                    gameui::Table* table1 = new gameui::Table(1);

                        gameui::Table* table2 = new gameui::Table(3);
                            table2->Add(new gameui::StaticText(&uiThemer, "property"));
                            table2->Add(new gameui::StaticText(&uiThemer, "type"));
                            table2->Add(new gameui::StaticText(&uiThemer, "value"));

                            table2->Add(new gameui::StaticText(&uiThemer, (*i).desc.name));
                            table2->Add(new gameui::StaticText(&uiThemer, DataModel::GetTypeName((*i).desc.type)));

                            gameui::TextBox* textBox = new gameui::TextBox(&uiThemer);
                            textBox->SetName("editing_property_value");
                            textBox->SetText(DataModel::ToString(*i));
                            table2->Add(textBox);

                            table2->SetColumnGrowable(0, true);
                            table2->SetColumnGrowable(1, true);
                            table2->SetColumnGrowable(2, true);
                            table2->SetSpacing(Int2(10, 10));
                        table1->Add(table2);

                        gameui::Table* table3 = new gameui::Table(2);
                            gameui::Button* ok = new gameui::Button(&uiThemer, "OK");
                            ok->SetName("editing_property_ok");
                            table3->Add(ok);

                            gameui::Button* cancel = new gameui::Button(&uiThemer, "Cancel");
                            cancel->SetName("editing_property_cancel");
                            table3->Add(cancel);

                            table3->SetColumnGrowable(0, true);
                            table3->SetColumnGrowable(1, true);
                        table1->Add(table3);

                    popup->Add(table1);
                    popup->SetAlign(ALIGN_HCENTER | ALIGN_VCENTER);
                    popup->SetName("editing_property_dlg");

                    ui->PushModal(popup);
                    return;
                }
            }
        }
    }

    void GameScreen::OnEventItemSelected(gameui::EventItemSelected* payload)
    {
        if (payload->widget == editing_block_type)
        {
            if (selectedBlock.x == INT16_MIN || selectedBlock.y == INT16_MIN)
                return;

            if (selectedBlock.x >= 0 && selectedBlock.y >= 0
                    && selectedBlock.x < worldSize.x && selectedBlock.y < worldSize.y)
            {
                WorldBlock* block = &blocks[selectedBlock.y * worldSize.x + selectedBlock.x];

                block->type = Edit_blockTypes[payload->itemIndex];

                Blocks::GenerateTiles(block);
                Blocks::ResetBlock(block, selectedBlock.x, selectedBlock.y);
            }
            else
                Edit_InsertBlock(selectedBlock, payload->itemIndex);
        }
        else if (payload->widget == editing_entity_class)
        {
            auto label = dynamic_cast<gameui::StaticText*>(editing_entity_class->GetSelectedItem());

            if (label != nullptr)
            {
                static const Float3 tileSize(TILE_SIZE_H, TILE_SIZE_V, 1.0f);
                            
                IEntityHandler* ieh = g_sys->GetEntityHandler(true);
                shared_ptr<IEntity> ent = shared_ptr<IEntity>(ieh->InstantiateEntity(label->GetLabel(), ENTITY_REQUIRED));

                if (ent == nullptr)
                    g_sys->DisplayError(g_eb, false);
                else
                {
                    ent->Init();
                    ent->SetPos(glm::round(camPos / tileSize) * tileSize);

                    ICommonEntity* ice = dynamic_cast<ICommonEntity*>(ent.get());

                    if (ice != nullptr && g_allowEditingMode)
                        ice->EditingModeInit();
                            
                    world->AddEntity(ent);
                }
            }
        }
    }
}
