
#include "client.hpp"

#include <framework/messagequeue.hpp>

#include <littl/File.hpp>
#include <littl/FileName.hpp>
//#include <littl/GzFileStream.hpp>

#include <superscanf.h>

namespace client
{
    WorldCollisionHandler::WorldCollisionHandler() : collidables(POINTENTITY)
    {
        g_world->AddEntityFilter(&collidables);

        dev_drawbounds = Var::LockInt("dev_drawbounds", false, 0);
    }

    WorldCollisionHandler::~WorldCollisionHandler()
    {
        Var::Unlock(dev_drawbounds);

        g_world->RemoveEntityFilter(&collidables);
    }

    bool WorldCollisionHandler::CHCanMove(PointEntity *ent, CFloat3& pos, CFloat3& newPos)
    {
        Float3 new_bbox[2], ent_bbox[2];

        if (!ent->GetBoundingBoxForPos(newPos, new_bbox[0], new_bbox[1]))
            return true;

        auto& entities = collidables.GetList();

        iterate2 (i, entities)
        {
            if (i == ent || !i->GetBoundingBox(ent_bbox[0], ent_bbox[1]))
                continue;

            if (new_bbox[0].x < ent_bbox[1].x && ent_bbox[0].x < new_bbox[1].x
                    && new_bbox[0].y < ent_bbox[1].y && ent_bbox[0].y < new_bbox[1].y
                    && new_bbox[0].z < ent_bbox[1].z && ent_bbox[0].z < new_bbox[1].z)
                return false;
        }

        return true;
    }

    void WorldCollisionHandler::Draw()
    {
        if (VAR_INT(dev_drawbounds) <= 0)
            return;

        zr::R::EnableDepthTest(false);
        zr::R::SetTexture(nullptr);

        Float3 bbox[2];
        auto& entities = collidables.GetList();

        iterate2 (i, entities)
        {
            i->GetBoundingBox(bbox[0], bbox[1]);

            if (VAR_INT(dev_drawbounds) == 1)
                zr::R::DrawFilledRect(Short2(bbox[0]), Short2(bbox[1]), RGBA_COLOUR(255, 0, 0, 80));
            else
                zr::R::DrawBox(bbox[0], bbox[1] - bbox[0], RGBA_COLOUR(255, 0, 0, 80));
        }
    }
    
    static void SpawnZombie(CFloat3& pos)
    {
        auto zombie = new character_zombie(pos);
        
        auto ai = new ai_zombie();
        ai->LinkEntity(zombie);
        
        g_world->AddEntity(zombie);
        g_world->AddEntity(ai);
    }
    
    WorldStage::WorldStage()
    {
    }

    WorldStage::~WorldStage()
    {
    }

    void WorldStage::Init()
    {
        msgQueue = nullptr;

        r_viewportw =       Var::LockInt("r_viewportw",     false);
        r_viewporth =       Var::LockInt("r_viewporth",     false);
        w_fov =             Var::LockFloat("w_fov",         false, 45.0f);
        w_topview =         Var::LockInt("w_topview",       false, 0);
        dev_drawbounds =    Var::LockInt("dev_drawbounds", false, 0);

        msgQueue = MessageQueue::Create();

        g_session = Session::Create(this);
        g_session->Host(Var::GetInt("net_port", true));
        g_session->SetDiscovery(true, 20120, 500);

        g_world = new World();
        g_collHandler = new WorldCollisionHandler();

        Entity::Register("ai_zombie",               &ai_zombie::Create);
        Entity::Register("character_player",        &character_player::Create);
        Entity::Register("character_zombie",        &character_zombie::Create);
        Entity::Register("prop_woodenbox",          &prop_woodenbox::Create);
        Entity::Register("world_background",        &world_background::Create);

        CreateUI();

        AddLayerBG(COLOUR_GREY(0.3f, 1.0f));
        AddLayerWorld(g_world, NO_NAME, this);
        AddLayer(g_collHandler);
        AddLayerUI(uiContainer);
        AddLayerWorld(g_world, DRAW_HUD_LAYER, this);

        player = new character_player(Float3());

        g_world->AddEntity(new prop_woodenbox(Float3(40, 0, 0)));
        g_world->AddEntity(new prop_woodenbox(Float3(80, 0, 0)));
        g_world->AddEntity(new prop_woodenbox(Float3(80, -40, 0)));
        g_world->AddEntity(new prop_woodenbox(Float3(140, -60, 0)));
        g_world->AddEntity(world_background::Create());
        g_world->AddEntity(player);
        SpawnZombie(Float3(200, 0, 0));
        SpawnZombie(Float3(-200, 0, 0));
        
        RestoreWorld();
        ReloadMedia();

        // FIXME: set r_batchalllocsize
    }

    void WorldStage::Exit()
    {
        RemoveLayer(g_collHandler);

        li::destroy(g_collHandler);
        li::destroy(g_world);
        li::destroy(g_session);
        
        li::destroy(msgQueue);

        Var::Unlock(r_viewportw);
        Var::Unlock(r_viewporth);
        Var::Unlock(w_topview);
        Var::Unlock(dev_drawbounds);

        ReleasePrivateMedia();

        uiContainer = nullptr;      // Will be taken care of by ~StageLayerUI
        li::destroy(uiRes);
    }

    void WorldStage::ClearWorld()
    {
        g_world->RemoveAllEntities(true);
    }

    void WorldStage::CreateUI()
    {
        using namespace gameui;

        uiRes = new UIResManager();
        uiContainer = new UIContainer(uiRes);

        uiRes->SetFontPrefix("fontcache/");

        uiRes->Load("client/titleui.cfx2",                          uiContainer);
    }

    void WorldStage::DrawScene()
    {
        DrawStage();
    }

    void WorldStage::OnFrame( double delta )
    {
        uiContainer->OnFrame(delta);

        Event_t* event;

        while ( (event = Event::Pop()) != nullptr )
        {
            /*if (windowDrag && event->type == EV_MOUSE_BTN && (event->mouse.flags & VKEY_RELEASED))
            {
                windowDrag = false;
                continue;
            }*/

            if (TryUIEvent(event) >= 1)
                continue;

            if (OnStageEvent(-1, event) >= 1)
                continue;
            
            switch ( event->type )
            {
                case EV_MOUSE_MOVE:
                    //if (windowDrag)
                    //    Video::MoveWindow(event->mouse.x - windowDragOrigin.x, event->mouse.y - windowDragOrigin.y);
                    break;

                case EV_UICLICK:
                    if (event->ui_item.widget->GetName().beginsWith("net_connectto:"))
                    {
                        char hostname[200];
                        int port;

                        if (2 == ssscanf(event->ui_item.widget->GetName(), 0, "net_connectto:%S:%i",
                            hostname, sizeof(hostname),
                            &port))
                        {
                            ClearWorld();

                            SetStatus((String) "Connecting to '" + hostname + "'...");
                            g_session->ConnectTo(hostname, port);
                        }
                    }
                    /*else if (event->ui_item.widget->GetName().equals("dev_connect"))
                    {
                        SetStatus("Connecting...");
                        g_session->ConnectTo(Var::GetStr("net_hostname", true), Var::GetInt("net_port", true));
                    }*/
                    else if (event->ui_item.widget->GetName().equals("dbg_toggletopview"))
                        VAR_INT(w_topview) = ! VAR_INT(w_topview);
                    else if (event->ui_item.widget->GetName().equals("dev_toggledrawbounds"))
                    {
                        if (++VAR_INT(dev_drawbounds) > 2)
                            VAR_INT(dev_drawbounds) = 0;
                    }
                    else if (event->ui_item.widget->GetName().equals("dev_savegame"))
                    {
                        Reference<OutputStream> saveFile = File::open("2012_sav.txt", "wb9");
                        //Reference<OutputStream> saveFile = File::open("2012_sav.txt", true);
                        ZFW_ASSERT(saveFile != nullptr)
                        
                        g_world->Serialize(saveFile, 0);
                        saveFile->write<int16_t>(player->GetID());
                    }
                    else if (event->ui_item.widget->GetName().equals("dev_loadgame"))
                    {
                        Reference<InputStream> saveFile = File::open("2012_sav.txt", "rb");
                        //Reference<InputStream> saveFile = File::open("2012_sav.txt");
                        ZFW_ASSERT(saveFile != nullptr)
                        g_world->RemoveAllEntities(true);
                        
                        g_world->Unserialize(saveFile, 0);
                        int playerEntID = saveFile->read<int16_t>();
                        player = dynamic_cast<character_player*>(g_world->GetEntityByID(playerEntID));
                        RestoreWorld();
                    }
                    break;

                case EV_VIEWPORT_RESIZE:
                {
                    uiContainer->SetArea(Int2(0, 0), Int2(event->viewresize.w, event->viewresize.h));
                    break;
                }

                case EV_VKEY:
                    if ( event->vkey.vk.inclass == IN_OTHER && event->vkey.vk.key == OTHER_CLOSE )
                    {
                        Sys::BreakGameLoop();
                        return;
                    }
                    else if (Video::IsEsc(event->vkey) && event->vkey.triggered())
                    {
                        // TODO: Anything?
                    }
                    break;
                    
                default:
                    ;
            }
        }
        
        MessageHeader* msg;

        while ((msg = msgQueue->Retrieve(Timeout(0))))
        {
            switch (msg->type)
            {
                case SESSION_CLIENT_ACCEPTED:
                {
                    int clid = msg->Data<SessionClientAccepted>()->clid;

                    ArrayIOStream outBuf;
                    g_world->Serialize(&outBuf, 0);

                    g_session->SendMessageTo(outBuf, clid);
                    break;
                }

                case SESSION_CONNECT_TO_RES:
                {
                    if (msg->Data<SessionRes>()->success)
                        SetStatus("Connected to server.");
                    else
                        SetStatus("Failed to connect.");

                    break;
                }

                case SESSION_HOST_RES:
                {
                    if (msg->Data<SessionRes>()->success)
                        SetStatus("Server listening.");
                    else
                        SetStatus("Unable to start server.");
                    break;
                }

                case SESSION_SET_DISCOVERY_RES:
                {
                    if (msg->Data<SessionRes>()->success)
                        //printf("Server listening.");
                            ;
                    else
                        printf("[Game] failed to bind() for discovery\n");
                    break;
                }

                case SESSION_SERVER_DISCOVERED:
                {
                    auto payload = msg->Data<SessionServerDiscovered>();

                    printf("[Game] discovered server %s:%i\n", payload->serverInfo.hostname.c_str(), payload->serverInfo.port);
                    
                    auto serverlist = dynamic_cast<gameui::WidgetContainer*>(uiContainer->Find("serverlist"));

                    if (serverlist != nullptr)
                    {
                        gameui::Button* button = new gameui::Button(uiRes, payload->serverInfo.serverName + "\n"
                                + payload->serverInfo.hostname + ":" + String::formatInt(payload->serverInfo.port));
                        button->SetName("net_connectto:" + payload->serverInfo.hostname + ":" + String::formatInt(payload->serverInfo.port));
                        button->ReloadMedia();

                        // FIXME
                        serverlist->Add(button);
                        uiContainer->SetArea(Int2(), Int2(VAR_INT(r_viewportw), VAR_INT(r_viewporth)));
                    }

                    break;
                }
            }

            msg->Release();
        }

        g_world->OnFrame(delta);

        //li_tryCall(session, OnFrame(delta));
        OnStageFrame(delta);
    }

    int WorldStage::OnIncomingConnection(const char* remoteHostname, const char* playerName, const char* playerPassword)
    {
        printf("[Game] allowing player '%s' from '%s' (pass '%s')\n", playerName, remoteHostname, playerPassword);
        return 1;
    }

    void WorldStage::ReleaseMedia()
    {
        ReleasePrivateMedia();
        ReleaseSharedMedia();
    }

    void WorldStage::ReleasePrivateMedia()
    {
        uiContainer->ReleaseMedia();
        uiRes->ReleaseMedia();
    }

    void WorldStage::ReloadMedia()
    {
        ReloadSharedMedia();

        uiRes->ReloadMedia();
        uiContainer->ReloadMedia();

        uiContainer->SetArea(Int2(), Int2(VAR_INT(r_viewportw), VAR_INT(r_viewporth)));

        R::EnableDepthTest( false );
        R::SetRenderFlags(R_COLOURS | R_UVS);
    }

    void WorldStage::RestoreWorld()
    {
        auto ctrl = new control_sideview();
        ctrl->LinkEntity(player);
        
        auto& controls = ctrl->GetControls();
        Event::ParseVkey(Var::GetStr("bind_left", false),   controls.keys[control_sideview::K_LEFT]);
        Event::ParseVkey(Var::GetStr("bind_right", false),  controls.keys[control_sideview::K_RIGHT]);
        Event::ParseVkey(Var::GetStr("bind_jump", false),   controls.keys[control_sideview::K_JUMP]);
        
        g_world->AddEntity(ctrl);
    }
    
    void WorldStage::SetStatus(const char* status)
    {
        auto statusinfo = dynamic_cast<gameui::StaticText*>(uiContainer->Find("statusinfo"));

        if (statusinfo != nullptr)
        {
            statusinfo->SetLabel(status);

            // FIXME
            statusinfo->ReloadMedia();
            statusinfo->SetArea(Int2(), Int2(VAR_INT(r_viewportw), VAR_INT(r_viewporth)));
        }
    }

    void WorldStage::SetUpProjection()
    {
        const float vfov = VAR_FLOAT(w_fov) * (f_pi / 180.0f);
        const float camera_z = //(VAR_INT(r_viewporth) / 2) / tan(vfov / 2.0f);
                650.0f;

        const Float3 origin(0, -100, 0);

        if (VAR_INT(w_topview) <= 0)
        {
            // FIXME: clipping planes as variables
            // TODO: auto-determine optimal clipping planes
            zr::R::SetPerspectiveProjection(//250.0f, 1500.0f, vfov);
                                            camera_z - 200.0f, camera_z + 200.0f, vfov);
            zr::R::SetCamera(origin + Float3(0.0f, 0.0f, camera_z), origin, Float3(0.0f, -1.0f, 0.0f));
        }
        else
        {
            zr::R::SetPerspectiveProjection(100.0f, 500.0f, vfov);
            zr::R::SetCamera(Float3(0.0f, -400.0f, 0.0f), Float3(0.0f, 0.0f, 0.0f), Float3(0.0f, 0.0f, -1.0f));
        }

        zr::R::EnableDepthTest(true);
    }
    
    int WorldStage::TryUIEvent(const Event_t* event)
    {
        switch (event->type)
        {
            case EV_MOUSE_BTN:
                if (event->mouse.button == MOUSE_BTN_LEFT)
                    return uiContainer->OnMousePrimary(-1, event->mouse.x, event->mouse.y, (event->mouse.flags & VKEY_PRESSED) ? 1 : 0);
                break;

            case EV_MOUSE_MOVE:
                return uiContainer->OnMouseMove(-1, event->mouse.x, event->mouse.y);

            default:
                ;
        }

        return -1;
    }
}
