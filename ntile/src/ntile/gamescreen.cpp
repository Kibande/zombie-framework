#include "components/aabbcollision.hpp"
#include "components/motion.hpp"
#include "gamescreen.hpp"
#include "world.hpp"

#include <framework/broadcasthandler.hpp>
#include <framework/colorconstants.hpp>
#include <framework/components/model3d.hpp>
#include <framework/components/position.hpp>
#include <framework/entityworld2.hpp>
#include <framework/errorcheck.hpp>
#include <framework/filesystem.hpp>
#include <framework/graphics.hpp>
#include <framework/profiler.hpp>
#include <framework/resourcemanager.hpp>
#include <framework/varsystem.hpp>

// screenshot
#include <framework/videohandler.hpp>
#include <framework/utility/pixmap.hpp>

#include "luascript.hpp"

// TODO: Fix all that depends on this, EVEN IMPLICITLY
#define CL_TICKRATE 60

#define kScreenshotFormat "png"

namespace ntile
{
    
#ifndef ZOMBIE_CTR
    static bool smaa = false;
#endif

    static GameScreen* s_gameScreen;

    static const char bindingsFileName[] = "ntile.bindings";

    const char* controlNames[] =
    {
        "UP",
        "DOWN",
        "LEFT",
        "RIGHT",
        "ATTACK",
        "BLOCK",
    };

    static const char* controlVarNames[] =
    {
        "bind_up",
        "bind_down",
        "bind_left",
        "bind_right",
        "bind_attack",
        "bind_block",

        "bind_screenshot",
    };

    static ProfilingSection_t profSetup = {"setup"};
    static ProfilingSection_t profPicking = {"picking"};
    static ProfilingSection_t profWorldSetup = {"world setup"};
    static ProfilingSection_t profWorld = {"world"};
    static ProfilingSection_t profEntities = {"entities"};
    static ProfilingSection_t profUI = {"ui"};

    IGameScreen* GetGameScreen()
    {
        return s_gameScreen;
    }

    static Float2 PixelSpaceToScreenSpace(Int2 pixelSpace)
    {
        return Float2(pixelSpace.x / (r_pixelRes.x * 0.5f) - 1.0f, 1.0f - pixelSpace.y / (r_pixelRes.y * 0.5f));
    }

    // ====================================================================== //
    //  class MyOpenFile
    // ====================================================================== //

    unique_ptr<IOStream> MyOpenFile::OpenFile(bool readOnly, bool create)
    {
        // FIXME: normalize fileName
        IFileSystem* ifs = g_sys->GetFileSystem();

        IOStream* ios = nullptr;
        if (!ifs->OpenFileStream(fileName, create ? (kFileMayCreate | kFileTruncate) : 0, nullptr, nullptr, &ios))
            g_sys->DisplayError(g_eb, false);

        return unique_ptr<IOStream>(ios);
    }

    // ====================================================================== //
    //  class GameScreen
    // ====================================================================== //

    GameScreen::GameScreen()
    {
        s_gameScreen = this;

        isTitle = true;
        editingMode = false;

        playerNearestEntity = nullptr;
        
        g_world.daytime = 5 * HOUR_TICKS;
        daytimeIncr = 1;
        
#ifndef ZOMBIE_CTR
        setControlsControl = nullptr;
        setControlsIndex = -1;
#endif

        worldSize = Int2(0, 0);

#ifdef ZOMBIE_CTR
        camEye = Float3(0.0f, 150.0f, 150.0f);
#else

        // Editing
        editing.tool = TOOL_RAISE;
        editing.toolShape = TOOL_CIRCLE;
        editing.toolRadius = 1;
        mouseBlock = Int2(INT16_MIN, INT16_MIN);
        selectedBlock = Int2(INT16_MIN, INT16_MIN);
        mouseEntity = nullptr;
        selectedEntity = nullptr;
        edit_movingEntity = false;
        blocks = nullptr;

        editing_panel = nullptr;
        editing_selected_block = nullptr;
        editing_block_type = nullptr;
        editing_selected_entity = nullptr;
        editing_entity_properties = nullptr;
        editing_entity_class = nullptr;
        editing_toolbar = nullptr;
#endif

        //font_title = nullptr;
        //font_h2 = nullptr;
        //worldTex = nullptr;
        //headsUp = nullptr;

        //worldShader = nullptr;
        //uiShader = nullptr;
        //g_worldVertexFormat = nullptr;
    }

    bool GameScreen::Init()
    {
        g_sys->Printf(kLogInfo, "GameScreen: Initialization");

        unique_ptr<LuaScript> scr(new LuaScript("ntile/scripts/startup"));
        if (!scr->Preload() || !scr->Realize())
            g_sys->DisplayError(g_eb, false);
        scr.reset();

        auto var = g_sys->GetVarSystem();

        IEntityHandler* ieh = g_sys->GetEntityHandler(true);

        ieh->Register("abstract_base",      &CreateInstance<entities::abstract_base>);
        ieh->RegisterExternal("ntile/entities/spawn_player", 0);

        ieh->Register("prop_base",          &CreateInstance<entities::prop_base>);
        ieh->Register("prop_treasurechest", &CreateInstance<entities::prop_treasurechest>);
        //Entity::Register("prop_tree",           &entities::prop_tree::Create);
        //ieh->Register("water_body",         &CreateInstance<entities::water_body>);
        ieh->RegisterExternal("ntile/entities/door_base", 0);
        ieh->RegisterExternal("ntile/entities/prop_tree", 0);
        ieh->RegisterExternal("ntile/entities/shiroi_house", 0);

        //if (g_eb->errorCode != 0)
        //    Sys::DisplayError(g_eb, false);

#ifdef ZOMBIE_STUDIO
        studioBlebManager.Init(g_sys);
        studioBlebManager.MountAllInDirectory("studiobleb");
#endif

        g_res->EnterResourceSection(&sectPrivate);

#ifndef ZOMBIE_CTR
        /*g_worldVertexFormat.reset(ir->CompileVertexFormat(worldShader, 32, worldVertexAttribs));

        g_res->Resource(&font_title,   "path=ntile/font/fat,size=8");
        g_res->Resource(&font_h2,      "path=ntile/font/thin,size=2");
        g_res->Resource(&worldTex,     "path=ntile/worldtex_%i.png,numMipmaps=7,lodBias=-1.0");*/

        //headsUp =           ir->LoadTexture("ntile/headsup.png", 0);
#endif

        //g_res->Resource(&worldShader,   "path=ntile/shaders/world");

        g_res->LeaveResourceSection();

        g_sys->Printf(kLogInfo, "Preloading resources...");
        g_res->SetTargetState(IResource2::PRELOADED);
        g_res->MakeAllResourcesTargetState(false);

        g_sys->Printf(kLogInfo, "Realizing resources...");
        g_res->SetTargetState(IResource2::REALIZED);
        g_res->MakeAllResourcesTargetState(false);
        
        g_sys->Printf(kLogInfo, "Loading complete.");

#ifndef ZOMBIE_CTR
        /*blend_colour =      worldShader->GetUniform("blend_colour");

        sun_dir =           worldShader->GetUniform("sun_dir");
        sun_amb =           worldShader->GetUniform("sun_amb");
        sun_diff =          worldShader->GetUniform("sun_diff");
        sun_spec =          worldShader->GetUniform("sun_spec");

        pt_pos[0] =         worldShader->GetUniform("pt_pos[0]");
        pt_amb[0] =         worldShader->GetUniform("pt_amb[0]");
        pt_diff[0] =        worldShader->GetUniform("pt_diff[0]");
        pt_spec[0] =        worldShader->GetUniform("pt_spec[0]");*/
#endif
        //Blocks::AllocBlocks(Int2(6, 3));
        Blocks::AllocBlocks(Int2(4, 3));

        WorldBlock* p_block = &blocks[0];

        for (int by = 0; by < worldSize.y; by++)
            for (int bx = 0; bx < worldSize.x; bx++)
            {
                p_block->type = BLOCK_SHIROI_OUTSIDE;

                Blocks::GenerateTiles(p_block);

                p_block++;
            }

        p_block = &blocks[0];

        for (int by = 0; by < worldSize.y; by++)
            for (int bx = 0; bx < worldSize.x; bx++)
            {
                Blocks::InitBlock(p_block, bx, by);

                BlockStateChangeEvent ev;
                ev.block = p_block;
                ev.change = BlockStateChange::created;
                g_sys->GetBroadcastHandler()->BroadcastMessage(ev);

                p_block++;
            }

        for (size_t i = 0; i < li_lengthof(controlVarNames); i++)
            var->BindVariable(controlVarNames[i], reflection::ReflectedValue_t(controls[i]), IVarSystem::kReadWrite, 0);

#ifndef ZOMBIE_CTR
        controls[Controls::screenshot] = Vkey_t {VKEY_KEY, -1, KEY_PRINTSCREEN, 0};
        controls[Controls::profiler] = Vkey_t {VKEY_KEY, -1, 'p', 0};
#else
        controls[Controls::screenshot] = Vkey_t {VKEY_JOYBTN, 0, 11, 0};
        controls[Controls::profiler] = Vkey_t {VKEY_JOYBTN, 0, 2, 0};
        controls[Controls::start] = Vkey_t {VKEY_JOYBTN, 0, 3, 0};
#endif

#ifdef ZOMBIE_STUDIO
        controls[Controls::studioRefresh] = Vkey_t {VKEY_KEY, -1, 0x3E, 0};
#endif

        LoadKeyBindings();
        //nui.Init();

#ifndef ZOMBIE_CTR
        //InitUI();
#endif
        // Scripting
        //api = new ScriptAPI(this);
        //g_scr->AddScriptAPI(CreateFrameworkSquirrelAPI());
        //g_scr->AddScriptAPI(CreateGameuiSquirrelAPI());
        //g_scr->AddScriptAPI(CreateNtileSquirrelAPI());
        //g.scr->AddScriptAPI(zfw::AddRef(api));

        const char* map;
        if (var->GetVariable("map", &map, 0))
            StartGame();

        g_sys->Printf(kLogInfo, "GameScreen: Initialization successful.");

        g_sys->GetBroadcastHandler()->BroadcastMessage(WorldSwitchedEvent {});
        return true;
    }

#ifndef ZOMBIE_CTR
    bool GameScreen::InitGameUI()
    {
        /*auto g = g_res->GetResource<IGraphics>("type=image,src=../media_src/sword.fw.png", RESOURCE_REQUIRED, TEXTURE_DEFAULT_FLAGS);

        if (g == nullptr)
            p_LogResourceError();
        else
        {
            gameui::Graphics* inventoryBtn = new gameui::Graphics(&uiThemer, g);
            inventoryBtn->SetAlign(ALIGN_RIGHT | ALIGN_BOTTOM);
            ui->Add(inventoryBtn);
        }*/

        return true;
    }
/*
    void GameScreen::InitUI()
    {
        // What we're actually doing here is making sure that the default fonts will be added in the correct order
        // What we probably SHOULD do instead is insert a dummy font returning (0, 0) size for all queries,
        // as the ui WILL try to layout itself before actual AcquireResources is called
        // FIXME: is this still true and where does it happen
        uiThemer.PreregisterFont("regular", "path=ntile/font/thin,size=2");

        uiThemer.AcquireResources();

        gameui::Widget::SetMessageQueue(g_msgQueue.get());
        ui.reset(new gameui::UIContainer(g_sys));
        ui->SetOnlineUpdate(false);

        gameui::UILoader ldr(g_sys, ui.get(), &uiThemer);

        if (!ldr.Load("ntile/ui/title.cfx2", ui.get(), false))
            g_sys->DisplayError(g_eb, false);
        else
        {
            auto game_version = dynamic_cast<gameui::StaticText*>(ui->Find("game_version"));

            if (game_version != nullptr)
                game_version->SetLabel(APP_VERSION);

            auto dev_mapname = dynamic_cast<gameui::StaticText*>(ui->Find("dev_mapname"));

            if (dev_mapname != nullptr)
                dev_mapname->SetLabel(sprintf_255("Map: %s",
                        g_sys->GetVarSystem()->GetVariableOrEmptyString("startmap")));
        }

        ui->AcquireResources();

        ui->SetArea(Int3(), r_pixelRes);
        ui->SetOnlineUpdate(true);
    }
    */
#endif

    void GameScreen::Shutdown()
    {
#ifndef ZOMBIE_CTR
        //ui.reset();
        //uiThemer.DropResources();
#endif

        Blocks::ReleaseBlocks(blocks, worldSize);

        g_res->ClearResourceSection(&sectPrivate);

    }

    void GameScreen::DrawScene()
    {
#if 0
        /*if (firstFrame)
            firstFrame = 0;
        else
            return;*/

        auto profiler = g_sys->IsProfiling() ? g_sys->GetProfiler() : nullptr;

        if (profiler)
            profiler->EnterSection(profSetup);

        // FIXME: don't reload the matrices all the fucking time

        // set world view (3D)
        // no drawing yet! screen is not even cleared
        ir->SetPerspective(10.0f, 1000.0f);
        ir->SetVFov(vfov);

        //ir->SetView(camPos + camEye, camPos, Float3(0.0f, -0.707f, 0.707f));
        ir->SetView(camPos + camEye, camPos, Float3(0.0f, -0.707f, 0.707f));

        // TODO: better way to do this?
        glm::mat4x4 modelView;
        ir->GetModelView(modelView);
#ifndef ZOMBIE_CTR
        mouseBlock = Int2(INT16_MIN, INT16_MIN);
        mouseEntity = nullptr;

        if (profiler)
        {
            profiler->LeaveSection();
            profiler->EnterSection(profPicking);
        }

        // TODO: this should probably be completely restructured
        // in editing mode do picking
        if (editingMode)
        {
            ir->BeginPicking();
            ir->Clear(RGBA_WHITE);

            // pick for blocks
            DrawBlocks(true, Int2(INT_MIN, INT_MIN));

            // pick for entities
            world->IterateEntities([](IEntity* ent) {
                const uint32_t pickingColour = 0xFF800000 | (ent->GetID());
                ir->SetColourv((const uint8_t*) &pickingColour);

                ent->Draw(&DRAW_ENT_PICKING);
            });

            auto sample = ir->EndPicking(goodMousePos);

            if ((sample & 0x800000) == 0)   // block or entity?
            {
                mouseBlock.x = (sample & 0x7FF) - 1;
                mouseBlock.y = ((sample >> 11) & 0x7FF) - 1;
            }
            else if (sample != 0xFFFFFFFF)
            {
                mouseEntity = world->GetEntityByID(sample & 0x7FFFFF);
            }
        }
#endif
        if (profiler)
        {
            profiler->LeaveSection();
            profiler->EnterSection(profWorldSetup);
        }

        // 3D world now
        ir->Begin3DScene();

        // sets sun dir/colour in shader
        Float3 backgroundColour;

        ir->SetShaderProgram(worldShader);
        SetupWorldLighting(modelView, backgroundColour);

        // this will be done per-block, right?
        worldShader->SetUniformVec4(blend_colour, COLOUR_WHITE);

        ir->Clear(Byte4(backgroundColour * 255.0f, 255));

#ifndef ZOMBIE_CTR
        // temporary: hero's torch
        if (player != nullptr && (daytime < 180 * MINUTE_TICKS || g_world.daytime > 450 * MINUTE_TICKS))
        {
            const Float3 TORCH_COLOUR(0.7f, 0.4f, 0.1f);
            const float BASE_RANGE = 32.0f;
            
            worldShader->SetUniformVec3(pt_pos[0], Float3(modelView
                    * player->GetModelView() * Float4(player->Hack_GetTorchLightPos(), 1.0f)));
            worldShader->SetUniformVec3(pt_amb[0],  BASE_RANGE * 0.4f * TORCH_COLOUR);
            worldShader->SetUniformVec3(pt_diff[0], BASE_RANGE * TORCH_COLOUR);
        }
        else
        {
            worldShader->SetUniformVec3(pt_amb[0], Float3(0.0f, 0.0f, 0.0f));
            worldShader->SetUniformVec3(pt_diff[0], Float3(0.0f, 0.0f, 0.0f));
        }

        // draw world blocks
        ir->SetTextureUnit(0, worldTex);

#else
        glEnable(GL_TEXTURE_2D);

        auto texture = static_cast<CTRTexture*>(worldTex);
        texture->Bind();
#endif

        if (profiler)
        {
            profiler->LeaveSection();
            profiler->EnterSection(profWorld);
        }

        DrawBlocks(false, Int2(mouseBlock));

        if (profiler)
        {
            profiler->LeaveSection();
            profiler->EnterSection(profEntities);
        }

        // draw entities
        if (editingMode)
            // only need to reset this in editing mode
            worldShader->SetUniformVec4(blend_colour, COLOUR_WHITE);

        world->Draw(nullptr);

#ifndef ZOMBIE_CTR
        if (editingMode)
            world->Draw(&DRAW_EDITOR_MODE);

        const float depthUnderCursor = ir->SampleDepth(goodMousePos);

        if (depthUnderCursor < 1.0f)
        {
            Float4 screenSpace = Float4(PixelSpaceToScreenSpace(goodMousePos), 2.0f * depthUnderCursor - 1.0f, 1.0f);

            glm::mat4x4 project, unproject;
            ir->GetProjectionModelView(project);
            unproject = glm::inverse(project);

            Float4 worldSpace = unproject * screenSpace;
            mouseWorldPos = Int3(glm::round(Float3(worldSpace * (1.0f / worldSpace.w))));

            /*Int3 abcd[] = {
                Int3(mouseWorldPos.x - 1, mouseWorldPos.y - 1, mouseWorldPos.z + 1),
                Int3(mouseWorldPos.x - 1, mouseWorldPos.y + 1, mouseWorldPos.z + 1),
                Int3(mouseWorldPos.x + 1, mouseWorldPos.y + 1, mouseWorldPos.z + 1),
                Int3(mouseWorldPos.x + 1, mouseWorldPos.y - 1, mouseWorldPos.z + 1),
            };

            ir->DrawQuad(abcd);*/
        }
#endif

        ir->End3DScene();

        if (profiler)
        {
            profiler->LeaveSection();
            profiler->EnterSection(profUI);
        }

        // 2D now
        ir->SetShaderProgram(uiShader);
        ir->SetOrthoScreenSpace(-100.0f, 100.0f);

        // title/overlay goes here unless GameUI
        if (isTitle)
        {
            font_title->DrawText("NANOTILE", Int3(r_pixelRes / 2, -10), RGBA_BLACK, ALIGN_HCENTER | ALIGN_BOTTOM);
            font_h2->DrawText("Quest of Kyria", Int3(r_pixelRes / 2, -5), RGBA_BLACK, ALIGN_HCENTER | ALIGN_TOP);
        }
        else
        {
        //    ir->DrawTexture(headsUp, Int2((r_pixelRes.x - 256) / 2, 0));
        }

        nui.Draw();

#ifndef ZOMBIE_CTR
        // draw gameui
        if (ui)
            ui->Draw();
#endif
        // temporary
        font_h2->DrawText(sprintf_t<31>("%02i:%02i (x %i)", g_world.daytime / HOUR_TICKS, (daytime % HOUR_TICKS) / MINUTE_TICKS, daytimeIncr),
                Int3(r_pixelRes.x - 4, r_pixelRes.y - 4, 0), RGBA_COLOUR(48, 224, 32), ALIGN_RIGHT | ALIGN_BOTTOM);

        if (playerNearestEntity != nullptr)
            font_h2->DrawText(sprintf_t<31>("near entity %s", playerNearestEntity->GetEntity()->GetName()),
                    Int3(4, r_pixelRes.y - 4, 0), RGBA_COLOUR(48, 32, 224), ALIGN_LEFT | ALIGN_BOTTOM);

#ifdef ZOMBIE_CTR
        u32 size, used;
        ctrglGetCommandBufferUtilization(&size, &used);
        static_cast<CTRTexture*>(worldTex)->Bind();

        ir->SetColour(RGBA_BLACK);
        ir->DrawRect(Int3(0, 0, 0), Int2(400, 1 + 2 + 1 + 2 + 1));

        ir->SetColour(RGBA_COLOUR(0, 255, 0));
        ir->DrawRect(Int3(1, 1, 0), Int2(398 * used / size, 2));

        ir->SetColour(RGBA_COLOUR(0, 0, 255));
        ir->DrawRect(Int3(1, 1 + 2 + 1, 0), Int2(398 * static_cast<CTRRenderer*>(ir)->renderingTime / 33333, 2));
        ir->SetColour(RGBA_COLOUR(255, 0, 0));
        ir->DrawRect(Int3(198, 1 + 2, 0), Int2(1, 4));
#endif
        if (profiler)
            profiler->LeaveSection();
#endif
    }

#if 0
    void GameScreen::DrawBlocks(bool picking, Int2 highlight)
    {
        /*
        const float vfov = this->vfov * f_pi / 180.0f;
        const float hfov = vfov * r_pixelRes.x / r_pixelRes.y;
        const float dist = glm::length(camDist);

        const float halfVisibleW = tan(hfov) * dist;
        const float halfVisibleH = tan(vfov) * dist;

        CFloat2 minVisible = Float2(camPos) + Float2(-halfVisibleW, -halfVisibleH);
        CFloat2 maxVisible = Float2(camPos) + Float2(halfVisibleW, halfVisibleH);

        const int bx1 = maximum<int>((int) floor(minVisible.x / 256.0f), 0);
        const int by1 = maximum<int>((int) floor(minVisible.y / 256.0f), 0);

        const int bx2 = minimum<int>((int) ceil(maxVisible.x / 256.0f), worldSize.x - 1);
        const int by2 = minimum<int>((int) ceil(maxVisible.y / 256.0f), worldSize.y - 1);
        */

        int bx1 = 0, by1 = 0, bx2 = worldSize.x - 1, by2 = worldSize.y - 1;

        // draw the world border in editing mode (to allow adding new blocks)
        if (editingMode)
        {
            bx1--;
            by1--;
            bx2++;
            by2++;
        }

        // draw visible blocks (TODO)
        for (int by = by1; by <= by2; by++)
            for (int bx = bx1; bx <= bx2; bx++)
            {
#ifdef ZOMBIE_STUDIO
                const uint32_t pickingColour = 0xFF000000 | ((by + 1) << 11) | (bx + 1);

                // editing mode: if not picking pass, set block blend colour
                if (!picking && editingMode)
                {
                    if (bx == selectedBlock.x && by == selectedBlock.y)
                        worldShader->SetUniformVec4(blend_colour, Float4(1.0f, 0.75f, 0.75f, 1.0f));
                    else if (bx == highlight.x && by == highlight.y)
                        worldShader->SetUniformVec4(blend_colour, COLOUR_GREY(0.75f));
                    else
                        worldShader->SetUniformVec4(blend_colour, COLOUR_WHITE);
                }

                // world border (editing mode only)
                if (bx < 0 || by < 0 || bx >= worldSize.x || by >= worldSize.y)
                {
                    const Int3 abcd[] = {
                        Int3(bx * 256 + 8,          by * 256 + 8,       20),
                        Int3(bx * 256 + 8,          by * 256 + 256 + 8, 20),
                        Int3(bx * 256 + 256 + 8,    by * 256 + 256 + 8, 20),
                        Int3(bx * 256 + 256 + 8,    by * 256 + 8,       20),
                    };

                    if (!picking)
                    {
                        ir->SetColour(RGBA_GREY(192));
                        ir->DrawQuad(abcd);
                        ir->SetColour(RGBA_WHITE);
                    }
                    else
                    {
                        ir->SetColourv((uint8_t*) &pickingColour);
                        ir->DrawQuad(abcd);
                    }
                }
                else
#endif
                {
                    // actual block, draw iiiiit

                    WorldBlock* block = &blocks[by * worldSize.x + bx];

#ifdef ZOMBIE_STUDIO
                    if (picking)
                        ir->SetColourv((const uint8_t*) &pickingColour);
#endif

                    ir->DrawPrimitives(block->vertexBuf.get(), PRIMITIVE_TRIANGLES, g_worldVertexFormat.get(), 0,
                            TILES_IN_BLOCK_V * TILES_IN_BLOCK_H * 3 * 6);
                }
            }
    }
#endif

//    bool GameScreen::OnAddEntity(EntityWorld* world, IEntity* ent)
//    {
//        IPointEntity* pe = dynamic_cast<IPointEntity*>(ent);
//
//        if (pe == nullptr)
//            return true;
//
//        Short2 blockXY = WorldToBlockXY(Float2(ent->GetPos()));
//
//        if (blockXY.x >= 0 && blockXY.y >= 0 && blockXY.x < worldSize.x && blockXY.y < worldSize.y)
//            blocks[blockXY.y * worldSize.x + blockXY.x].entities.add(pe);
//
//        ICommonEntity* ice = dynamic_cast<ICommonEntity*>(ent);
//
//        if (ice != nullptr)
//            ice->SetMovementListener(this);
//
//        return true;
//    }

    void GameScreen::OnFrame(double delta)
    {
        MessageHeader* msg;

        while ((msg = g_msgQueue->Retrieve(Timeout(0))) != nullptr)
        {
            /*if (nui.HandleMessage(h_new, msg) > h_direct)
            {
                msg->Release();
                continue;
            }*/

            switch (msg->type)
            {
#ifndef ZOMBIE_CTR
                case EVENT_MOUSE_MOVE:
                {
                    auto ev = msg->Data<EventMouseMove>();

                    if (edit_movingEntity)
                    {
                        ZFW_ASSERT(selectedEntity != nullptr)

                        IEntity* ent = selectedEntity->GetEntity();
                        const Float2 tileSize(TILE_SIZE_H, TILE_SIZE_V);
                        const Float2 steps = glm::round(Float2(ev->x - edit_entityMovementOrigin.x, ev->y - edit_entityMovementOrigin.y) / tileSize);
                        ent->SetPos(edit_entityInitialPos + Float3(steps * tileSize, 0.0f));
                    }

                    r_mousePos = Int2(ev->x, ev->y);

                    //if (ui->OnMouseMove(gameui::h_new, ev->x, ev->y) <= gameui::h_indirect)
                    //    goodMousePos = r_mousePos;
                    break;
                }
#endif
                case EVENT_VKEY:
                {
                    auto ev = msg->Data<EventVkey>();

                    int button;
                    bool pressed;

#ifndef ZOMBIE_CTR
                    if (Vkey::IsMouseButtonEvent(ev->input, button, pressed))
                    {
                        if (edit_movingEntity)
                            edit_movingEntity = false;

                        int h = gameui::h_new;

                        //h = ui->OnMouseButton(h, button, pressed, r_mousePos.x, r_mousePos.y);

                        if (button == MOUSEBTN_LEFT)
                        {
                            if (h < 0 && pressed)
                            {
                                if (editingMode)
                                {
                                    Edit_SelectBlock(mouseBlock);
                                    Edit_SelectEntity(mouseEntity);
                                }

                                ICommonEntity* ent = dynamic_cast<ICommonEntity*>(mouseEntity);

                                if (ent != nullptr)
                                    ent->OnClicked(this, MOUSEBTN_LEFT);
                            }
                        }
                        /*else if (button == MOUSEBTN_RIGHT && pressed)
                        {
                            if (editingMode)
                            {
                                if (mouseEntity != nullptr)
                                {
                                    Edit_SelectEntity(mouseEntity);

                                    auto popupMenu = new gameui::Popup(&uiThemer);
                                    popupMenu->SetEasyDismiss(true);

                                    auto table = new gameui::Table(1);

                                    auto moveBtn = new gameui::Button(&uiThemer, "Move");
                                    moveBtn->SetName("editing_move_entity");
                                    table->Add(moveBtn);

                                    auto deleteBtn = new gameui::Button(&uiThemer, "Delete");
                                    deleteBtn->SetName("editing_delete_entity");
                                    table->Add(deleteBtn);

                                    popupMenu->Add(table);
                                    popupMenu->SetFreeFloat(true);
                                    popupMenu->SetPos(Int3(r_mousePos, 0));

                                    ui->Add(popupMenu);
                                }
                                else
                                {
                                    Edit_ToolEdit(mouseWorldPos);
                                }
                            }
                        }*/
                        else if (button == MOUSEBTN_WHEEL_UP) {
                            zombie_assert(!"camEye *= 1.0f / 1.02f;");
                            //camEye *= 1.0f / 1.02f;
                        }

                        else if (button == MOUSEBTN_WHEEL_DOWN) {
                            zombie_assert(!"camEye *= 1.02f;");
                            //camEye *= 1.02f;
                        }

                        break;
                    }

                    if (setControlsIndex >= 0)
                    {
                        if (!Vkey::IsStrongInput(ev->input))
                            break;

                        controls[setControlsIndex] = ev->input.vkey;
                        //Sys::printk("Bound: %s", Event::FormatVkey(ev->vkey.vk));
                        setControlsIndex++;

                        if (setControlsIndex < li_lengthof(controlNames))
                            setControlsControl->SetLabel(controlNames[setControlsIndex]);
                        else
                        {
                            //ui->PopModal(true);
                            setControlsIndex = -1;

                            SaveKeyBindings();
                        }

                        break;
                    }

                    if (ev->input.vkey.type == VKEY_KEY && ev->input.vkey.key == KEY_ESCAPE)
                        g_sys->StopMainLoop();
#endif

                    if (Vkey::Test(ev->input, controls[Controls::screenshot]) && (ev->input.flags & VKEY_PRESSED))
                    {
                        if (!SaveScreenshot())
                            g_sys->DisplayError(g_eb, false);
                        ntile::g_sys->Printf(kLogInfo, "Done.");
                    }

                    if (Vkey::Test(ev->input, controls[Controls::profiler]) && (ev->input.flags & VKEY_PRESSED))
                    {
                        g_sys->ProfileFrame(g_sys->GetFrameCounter() + 1);
                    }

                    if (Vkey::Test(ev->input, controls[Controls::start]) && (ev->input.flags & VKEY_PRESSED))
                    {
                        StartGame();
                    }

#ifdef ZOMBIE_STUDIO
                    if (Vkey::Test(ev->input, controls[Controls::studioRefresh]) && (ev->input.flags & VKEY_PRESSED))
                    {
                        zombie_assert(g_res->MakeAllResourcesState(IResource2::RELEASED, true));

                        if (studioBlebManager.Refresh())
                        {
                            g_sys->Printf(kLogInfo, "GameScreen: Reloading all resources...");
                            g_res->MakeAllResourcesTargetState(false);
                            g_sys->Printf(kLogInfo, "GameScreen: Done.");
                        }
                    }
#endif

                    auto motion = g_ew->GetEntityComponent<Motion>(g_world.playerEntity);

                    if (motion)
                    {
                        if (Vkey::Test(ev->input, controls[Controls::left]))
                            motion->nudgeDirection.x += ((ev->input.flags & VKEY_PRESSED) ? -1 : +1);

                        if (Vkey::Test(ev->input, controls[Controls::right]))
                            motion->nudgeDirection.x += ((ev->input.flags & VKEY_PRESSED) ? +1 : -1);

                        if (Vkey::Test(ev->input, controls[Controls::up]))
                            motion->nudgeDirection.y += ((ev->input.flags & VKEY_PRESSED) ? -1 : +1);

                        if (Vkey::Test(ev->input, controls[Controls::down]))
                            motion->nudgeDirection.y += ((ev->input.flags & VKEY_PRESSED) ? +1 : -1);

//                        if (Vkey::Test(ev->input, controls[Controls::attack]) && (ev->input.flags & VKEY_PRESSED))
//                            player->Hack_SlashAnim();
//
//                        if (Vkey::Test(ev->input, controls[Controls::block]) && (ev->input.flags & VKEY_PRESSED))
//                            player->Hack_ShieldAnim();
                    }

#ifndef ZOMBIE_CTR
                    /*if (ev->input.vkey.type == VKEY_KEY && ev->input.vkey.key == 27 && (ev->input.flags & VKEY_PRESSED))
                    {
                        smaa = !smaa;
                        static_cast<n3d::GLRenderer*>(ir)->SetSMAA(smaa);
                    }*/
#endif

                    break;
                }

                case EVENT_WINDOW_CLOSE:
                    g_sys->StopMainLoop();
                    break;
#ifndef ZOMBIE_CTR
                case EVENT_WINDOW_RESIZE:
                {
                    auto payload = msg->Data<EventWindowResize>();

                    r_pixelRes = Int2(payload->width, payload->height);
                    
                    //ui->SetArea(Int3(), r_pixelRes);
                    //ui->Layout();
                    break;
                }

                case gameui::EVENT_CONTROL_USED:
                {
                    auto payload = msg->Data<gameui::EventControlUsed>();
                    OnEventControlUsed(payload);
                    break;
                }

                case gameui::EVENT_ITEM_SELECTED:
                {
                    auto payload = msg->Data<gameui::EventItemSelected>();
                    OnEventItemSelected(payload);
                    break;
                }

                case gameui::EVENT_LOOP_EVENT:
                {
                    auto payload = msg->Data<gameui::EventLoopEvent>();
                    //ui->OnLoopEvent(payload);
                    break;
                }

                case gameui::EVENT_MOUSE_DOWN:
                {
                    auto payload = msg->Data<gameui::EventMouseDown>();

                    if (payload->widget == editing_toolbar)
                    {
                        const int x = (payload->x - payload->widget->GetPos().x) / 24;
                        const int y = (payload->y - payload->widget->GetPos().y) / 24;

                        if (y == 0)
                        {
                            if (x == 0) { editing.toolShape = TOOL_CIRCLE; editing.toolRadius = 1; }
                            else if (x == 1) { editing.toolShape = TOOL_CIRCLE; editing.toolRadius = 5; }
                            else if (x == 2) { editing.toolShape = TOOL_CIRCLE; editing.toolRadius = 9; }
                            else if (x == 3) { editing.toolShape = TOOL_CIRCLE; editing.toolRadius = 13; }
                            //else if (x == 4) { editing.toolShape = TOOL_CIRCLE; editing.toolRadius = 1; }
                        }
                        else
                            editing.tool = x;
                    }
                }

                case gameui::EVENT_VALUE_CHANGED:
                {
                    auto payload = msg->Data<gameui::EventValueChanged>();

                    if (payload->widget->GetNameString() == "editing_toggle")
                    {
                        editingMode = (payload->value != 0);

                        if (editing_panel != nullptr)
                            editing_panel->SetVisible(editingMode);

                        if (editing_toolbar != nullptr)
                            editing_toolbar->SetVisible(editingMode);
                    }
                    else if (payload->widget->GetNameString() == "fasttime_toggle")
                    {
                        daytimeIncr = (payload->value == 0) ? 1 : 10;
                    }

                    break;
                }
#endif
            }

            msg->Release();
        }

#ifndef ZOMBIE_CTR
        //ui->OnFrame(delta);
#endif
    }

//    void GameScreen::OnRemoveEntity(EntityWorld* world, IEntity* ent)
//    {
//    }

//    void GameScreen::OnSetPos(IPointEntity* pe, const Float3& oldPos, const Float3& newPos)
//    {
//        const Short2 oldBucket = WorldToBlockXY(Float2(oldPos));
//        const Short2 newBucket = WorldToBlockXY(Float2(newPos));
//
//        if (oldBucket != newBucket)
//        {
//            if (oldBucket.x >= 0 && oldBucket.y >= 0 && oldBucket.x < worldSize.x && oldBucket.y < worldSize.y)
//                blocks[oldBucket.y * worldSize.x + oldBucket.x].entities.removeItem(pe);
//
//            if (newBucket.x >= 0 && newBucket.y >= 0 && newBucket.x < worldSize.x && newBucket.y < worldSize.y)
//                blocks[newBucket.y * newBucket.x + newBucket.x].entities.add(pe);
//        }
//
//        if (pe == player.get())
//        {
//            IPointEntity* nearestEntity = nullptr;
//
//            static const float MAX_DIST = 24.0f;
//            float nearestEntityDist = MAX_DIST;
//
//            for (int by = std::max(newBucket.y - 1, 0); by < newBucket.y + 1 && by < worldSize.y; by++)
//                for (int bx = std::max(newBucket.x - 1, 0); bx < newBucket.x + 1 && bx < worldSize.x; bx++)
//                {
//                    for (const auto& ent : blocks[by * worldSize.x + bx].entities)
//                    {
//                        if (ent == pe)
//                            continue;
//
//                        const float dist = glm::length(newPos - ent->GetEntity()->GetPos());
//
//                        if (dist < nearestEntityDist && dist < MAX_DIST)
//                        {
//                            nearestEntityDist = dist;
//                            nearestEntity = ent;
//                        }
//                    }
//                }
//
//            playerNearestEntity = nearestEntity;
//        }
//    }

    void GameScreen::OnTicks(int ticks)
    {
        //ticks *= 4;

        if (isTitle)
        {
//            camPos.x += 6.0f * ticks / CL_TICKRATE;
//
//            if (camPos.x >= worldSize.x * 128.0f + 128.0f)
//            {
//                uint8_t buffer[sizeof(WorldBlock)];
//                WorldBlock* tmp = reinterpret_cast<WorldBlock*>(buffer);
//
//                for (int by = 0; by < worldSize.y; by++)
//                {
//                    WorldBlock* p_block = &blocks[worldSize.x * by];
//
//                    Allocator<WorldBlock>::move(tmp, p_block, 1);
//                    Allocator<WorldBlock>::move(p_block, p_block + 1, worldSize.x - 1);
//                    Allocator<WorldBlock>::move(p_block + worldSize.x - 1, tmp, 1);
//                }
//
//                WorldBlock* p_block = &blocks[0];
//
//                for (int by = 0; by < worldSize.y; by++)
//                    for (int bx = 0; bx < worldSize.x; bx++)
//                    {
//                        Blocks::ResetBlock(p_block, bx, by);
//                        p_block++;
//                    }
//
//                camPos.x -= 256.0f;
//            }
        }

        if (!editingMode)
        {
            while (ticks--)
            {
                //world->OnTick();
                //nui.OnTick();

                g_world.daytime += daytimeIncr;

                if (g_world.daytime > DAY_TICKS)
                    g_world.daytime = 0;
            }
        }
        else
        {
            g_world.daytime = 5 * HOUR_TICKS + 0 * MINUTE_TICKS;
//            camPos += Float3(10 * player->GetMotionVec(), 0.0f);
        }
    }

    void GameScreen::p_LogResourceError()
    {
        const char* desc;

        if (Params::GetValueForKey(g_eb->params, "desc", desc))
        {
            g_sys->Printf(kLogError, "Resource Loading Error: %s", desc);
        }
    }

    bool GameScreen::StartGame()
    {
        //////////////////////////

        g_res->EnterResourceSection(&sectEntities);
        g_res->SetTargetState(IResource2::CREATED);

        const char* map = g_sys->GetVarSystem()->GetVariableOrEmptyString("map");
        int rc = LoadMap(map);

        if (rc != 0)
        {
            const char* map = g_sys->GetVarSystem()->GetVariableOrEmptyString("map");

            ErrorBuffer::SetError3(EX_ASSET_OPEN_ERR, 2,
                    "desc", sprintf_255("Failed to load map '%s'.%s", map, (rc == EX_ASSET_CORRUPTED) ? " The file is corrupted." : ""),
                    "function", li_functionName
                    );
            g_sys->DisplayError(g_eb, false);
            return false;     // FIXME: what do here
        }

        g_res->LeaveResourceSection();

        g_sys->Printf(kLogInfo, "Preloading world resources...");
        g_res->SetTargetState(IResource2::PRELOADED);
        g_res->MakeResourcesInSectionTargetState(&sectEntities, false);

        g_sys->Printf(kLogInfo, "Realizing world resources...");
        g_res->SetTargetState(IResource2::REALIZED);
        g_res->MakeResourcesInSectionTargetState(&sectEntities, false);

        g_sys->Printf(kLogInfo, "World loading complete.");

        //////////////////////////

        isTitle = false;

        auto player = g_ew->CreateEntity();
        g_ew->SetEntityComponent(player, Position{Int3(worldSize * Int2(128, 128), 0)});
        g_ew->SetEntityComponent(player, Model3D{"ntile/models/player"});
        g_ew->SetEntityComponent(player, Motion {});
        g_ew->SetEntityComponent(player, AabbCollision {Float3(-6.0f, -6.0f, 0.0f), Float3(6.0f, 6.0f, 24.0f)});
        g_world.playerEntity = player;

#ifndef ZOMBIE_CTR
        // Set up UI
        //ui->RemoveAll();

        InitGameUI();
        //Edit_InitEditingUI();
#endif

        return true;
    }

    bool GameScreen::LoadKeyBindings()
    {
        auto var = g_sys->GetVarSystem();

        unique_ptr<InputStream> bindings(g_sys->OpenInput(bindingsFileName));

        if (bindings)
            zombie_ErrorLog(g_sys, g_eb, var->DeserializeVariables(bindings.get(), bindingsFileName, IVarSystem::kListedOnly,
                    controlVarNames, li_lengthof(controlVarNames)));

        return true;
    }

    bool GameScreen::SaveKeyBindings()
    {
        auto var = g_sys->GetVarSystem();

        unique_ptr<OutputStream> bindings(g_sys->OpenOutput(bindingsFileName));

        if (bindings)
            zombie_ErrorLog(g_sys, g_eb, var->SerializeVariables(bindings.get(), bindingsFileName, IVarSystem::kListedOnly,
                    controlVarNames, li_lengthof(controlVarNames)));

        return true;
    }

    bool GameScreen::SaveScreenshot()
    {
        auto ivh = g_sys->GetVideoHandler();

        Pixmap_t pm;
        if (ivh->CaptureFrame(&pm))
        {
            auto imch = g_sys->GetMediaCodecHandler(true);
            auto ifs = g_sys->GetFileSystem();

            const char* format = "screenshot%04u." kScreenshotFormat;
            char nameBuffer[256];

            for (unsigned int i = 1; i < 9999; i++)
            {
                snprintf(nameBuffer, sizeof(nameBuffer), format, i);

#ifndef ZOMBIE_CTR
                FSStat_t stat;
                // look for a file that doesn't exist yet
                if (!ifs->Stat(nameBuffer, &stat))
                    break;
#else
                unique_ptr<InputStream> test(g_sys->OpenInput(nameBuffer));
                if (!test)
                    break;
#endif
            }

            g_sys->Printf(kLogInfo, "Saving screenshot %s...", nameBuffer);
            auto encoder = imch->GetEncoderByFileType<IPixmapEncoder>(kScreenshotFormat, nameBuffer, kCodecRequired);

            if (!encoder)
                return false;

            unique_ptr<OutputStream> stream(g_sys->OpenOutput(nameBuffer));

            if (!stream)
                return false;

            return encoder->EncodePixmap(&pm, stream.get(), nameBuffer) == IEncoder::kOK;
        }

        return false;
    }
}
