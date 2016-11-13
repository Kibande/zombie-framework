
#include "zombie.hpp"

#include <framework/collision.hpp>

float angle = 0.0f;
float panim = 0.0f;

#define ANALOG_LOW_CUTOFF 7
//#define ANALOG_AIM_CUTOFF 20
#define AIM_CUTOFF 30

static const glm::vec3 bodySize(0.25f, 0.25f, 1.6f);

namespace zombie
{
    static int GetAnalogValue( const VkeyState_t& vkey )
    {
        if ( vkey.vk.inclass == IN_KEY || vkey.vk.inclass == IN_JOYBTN || vkey.vk.inclass == IN_JOYHAT )
            return (vkey.flags & VKEY_PRESSED) ? 127 : 0;

        if ( vkey.vk.inclass == IN_ANALOG )
        {
            int value = (vkey.value / 256);

            if (value < ANALOG_LOW_CUTOFF)
                return 0;
            else
                return value;
        }

        return 0;
    }

    GameScene::GameScene()
    {
    }

    GameScene::~GameScene()
    {
        ClearEntities();
    }

    void GameScene::ClearEntities()
    {
        iterate2 (i, entities)
        {
            WorldEnt& ent = i;
            ent.Release();
        }

        entities.clear();
    }

    void GameScene::Exit()
    {
#ifdef ENABLE_COLLDET
        li::destroy(physWorld);
#endif

        ReleaseMedia();
    }

    void GameScene::DrawScene()
    {
        R::Clear();

        c.setCenterPos( playerModel.pos );
        c.setEyePos( playerModel.pos + glm::vec3(0.0f, 0.0f, 12.0f) );
        c.setUpVector( glm::vec3(0.0f, -1.0f, 0.0f) );

        // settings common for cube & text
        R::SelectShadingProgram( worldShader );
        R::SetPerspectiveProjection(1.0f, 500.0f, 45.0f);
        R::SetCamera(c);
        R::SetTexture( white_tex );

        glm::vec3 pos(0.0f, 0.0f, 1.0f), ambient(1.0f, 0.5f, 0.0f), diffuse(1.0f, 1.0f, 1.0f), specular;
        glm::vec3 pos2(4.0f, 0.0f, 2.0f), ambient2(0.2f, 0.2f, 0.2f), diffuse2(0.4f, 0.4f, 0.4f);
        glm::vec3 pos3(4.0f, 4.0f, 2.0f);
        float range(10.0f);

        R::ClearLights();
        R::AddLightPt( pos, ambient, diffuse, specular, range );
        R::AddLightPt( pos2, ambient2, diffuse2, specular, 10.0f );
        R::AddLightPt( pos3, -ambient2, -diffuse2, -specular, 10.0f );

        // ground
        //R::SetBlendColour( glm::vec4(0.1f, 0.1f, 0.1f, 1.0f) );
        R::SetBlendColour( glm::vec4(0.5f, 0.5f, 0.5f, 1.0f) );
        //R::DrawDIMesh( &ground, glm::mat4() );

        DrawHumanoid(playerModel, panim, angle);

        // Draw world
        iterate2 (i, entities)
        {
            const WorldEnt& ent = i;

            R::SetBlendColour( ent.colour );

            if ( ent.type == ENT_CUBE )
                R::DrawDIMesh( &buildCube, glm::scale(glm::translate(glm::mat4(), ent.pos), ent.size) );
            else
                R::DrawDIMesh( &buildPlane, glm::scale(glm::translate(glm::mat4(), ent.pos), glm::vec3(ent.size.x, ent.size.y, 1.0f)) );
        }

        // menu labels

        // 2d overlay
        //R::Set2DView();
        //R::SelectShadingProgram( nullptr );
    }

    void GameScene::Init()
    {
        /*
        menuFont = nullptr;
        uiFont = nullptr;*/

        // video settings
        Video::EnableKeyRepeat( false );

        // collision detection
#ifdef ENABLE_COLLDET
        physWorld = physics::CreateBulletPhysWorld();
        playerCollObj = physWorld->CreateBoxColl(glm::vec3(0.25f, 0.25f, 1.6f));
#endif

        // remote media & render settings
        // INCLUDING MAP LOADING
        // THIS REQUIRES THINGS LIKE PHYSICS ALREADY SET UP
        ReloadMedia();

        // local media
        CreateHumanoid(humanlike, playerModel);
        playerModel.pos = glm::vec3(0.0f, 0.0f, 1.6f);

        //c = world::Camera( playerModel.pos, 15.0f, 270.0f / 180.0f * f_pi, 80.0f / 180.0f * f_pi );
        media::PresetFactory::CreatePlane( glm::vec2( 20.0f, 20.0f ), glm::vec3( 10.0f, 10.0f, 10.0f ), media::ALL_SIDES | media::WITH_NORMALS, &ground );

        media::PresetFactory::CreatePlane( glm::vec2(1.0f, 1.0f), glm::vec3(), media::ALL_SIDES | media::WITH_NORMALS, &buildPlane );
        media::PresetFactory::CreateCube( glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(), media::ALL_SIDES | media::WITH_NORMALS | media::WITH_UVS, &buildCube );

        inputs = 0;
        r_viewportw = Var::LockInt("r_viewportw", false);
        r_viewporth = Var::LockInt("r_viewporth", false);
        in_mousex = Var::LockInt("in_mousex", false);
        in_mousey = Var::LockInt("in_mousey", false);
    }

    void GameScene::LoadMap()
    {
        String saveName = "testmap";

        Reference<SeekableInputStream> mapFile = Sys::OpenInput( "maps/" + saveName + ".ds" );

        datafile::SDStream sds(mapFile.detach());

        MapLoader loader(sds);

        loader.Init();
        loader.LoadMap(this);
        loader.Exit();
    }

    static glm::mat4x4 getPlayerCollisionTransform(const glm::vec3& pos, float angle)
    {
        glm::mat4x4 m = glm::translate( glm::mat4x4(), - glm::vec3(0.25f, 0.25f, 1.6f) / 2.0f );

        m = glm::rotate(m, angle, glm::vec3(0.0f, 0.0f, 1.0f));
        m = glm::translate(m, pos);

        return m;
    }

    void GameScene::OnFrame( double delta )
    {
        Event_t* event;

        static int moveX = 0;
        static int moveY = 0;

        static int aimX = 0;
        static int aimY = 0;

        while ( ( event = Event::Pop() ) != nullptr )
        {
            switch ( event->type )
            {
                case EV_VKEY:
                {
                    if ( event->vkey.vk.inclass == IN_OTHER && event->vkey.vk.key == OTHER_CLOSE )
                    {
                        Sys::BreakGameLoop();
                        return;
                    }

                    int a = GetAnalogValue(event->vkey);

                    if ( IsMUp(event->vkey) )
                        moveY = -a;

                    if ( IsMDown(event->vkey) )
                        moveY = a;

                    if ( IsMLeft(event->vkey) )
                        moveX = -a;

                    if ( IsMRight(event->vkey) )
                        moveX = a;

                    //if ( event->vkey.vk.inclass != IN_ANALOG || a > ANALOG_AIM_CUTOFF )
                    {
                        if ( IsAUp(event->vkey) )
                            aimY = -a;

                        if ( IsADown(event->vkey) )
                            aimY = a;

                        if ( IsALeft(event->vkey) )
                            aimX = -a;

                        if ( IsARight(event->vkey) )
                            aimX = a;
                    }

                    /*if ( event->vkey.flags & VKEY_PRESSED )
                    {
                        if ( event->vkey.vk == 'w' )
                            inputs |= 1;
                        else if ( event->vkey.vk == 's' )
                            inputs |= 2;
                        else if ( event->vkey.vk == 'a' )
                            inputs |= 4;
                        else if ( event->vkey.vk == 'd' )
                            inputs |= 8;
                        else if ( event->vkey.vk == V_LEFTMOUSE )
                            inputs |= 16;
                    }
                    else if (event->vkey.flags & VKEY_RELEASED)
                    {
                        if ( event->vkey.vk == 'w' )
                            inputs &= ~1;
                        else if ( event->vkey.vk == 's' )
                            inputs &= ~2;
                        else if ( event->vkey.vk == 'a' )
                            inputs &= ~4;
                        else if ( event->vkey.vk == 'd' )
                            inputs &= ~8;
                        else if ( event->vkey.vk == V_LEFTMOUSE )
                            inputs &= ~16;
                    }*/

                    if (IsAnyMenu(event->vkey))
                        Sys::ChangeScene(new TitleScene());

                    break;
                }

                case EV_MOUSE_MOVE:
                {
                    // multiply by 2 to reduce aim cutoff effect
                    aimX = in_mousex->basictypes.i * 2 - r_viewportw->basictypes.i;
                    aimY = in_mousey->basictypes.i * 2 - r_viewporth->basictypes.i;
                    break;
                }

                default:
                    ;
            }
        }

        static const float sp = 4.0f;

        /*if (inputs & 1)     playerModel.pos.y -= delta * sp;
        if (inputs & 2)     playerModel.pos.y += delta * sp;
        if (inputs & 4)     playerModel.pos.x -= delta * sp;
        if (inputs & 8)     playerModel.pos.x += delta * sp;*/

#ifdef ENABLE_COLLDET
        if (moveX != 0)
        {
            const glm::vec3 newPos = playerModel.pos + glm::vec3( delta * sp * moveX / 127.0f, 0.0f, 0.0f );
            /*const glm::vec3 c[] = { newPos + glm::vec3(-bodySize.x * 0.5f, -bodySize.y * 0.5f, -bodySize.z), newPos + bodySize };

            if ( !TestAABB_World( c ) )
                playerModel.pos = newPos;*/

            playerCollObj->SetTransform(getPlayerCollisionTransform(newPos, 0));

            if (!physWorld->TestObject(playerCollObj))
                playerModel.pos = newPos;
            else
                playerCollObj->SetTransform(getPlayerCollisionTransform(playerModel.pos, 0));
        }

        if (moveY != 0)
        {
            const glm::vec3 newPos = playerModel.pos + glm::vec3( 0.0f, delta * sp * moveY / 127.0f, 0.0f );
            //const glm::vec3 c[] = { newPos + glm::vec3(-bodySize.x * 0.5f, -bodySize.y * 0.5f, -bodySize.z), newPos + bodySize };

            playerCollObj->SetTransform(getPlayerCollisionTransform(newPos, 0));

            if (!physWorld->TestObject(playerCollObj))
                playerModel.pos = newPos;
            else
                playerCollObj->SetTransform(getPlayerCollisionTransform(playerModel.pos, 0));
        }

        if (physWorld != nullptr)
            physWorld->Update(delta);
#else
        if (moveX != 0 || moveY != 0)
            playerModel.pos += glm::vec3( delta * sp * moveX / 127.0f, delta * sp * moveY / 127.0f, 0.0f );
#endif

        if (abs(aimX) + abs(aimY) > AIM_CUTOFF)
            angle = atan2(float(-aimY), float(aimX)) / f_pi * 180.0f;

        /*if ( inputs & 15 )
        {
            panim += delta * 1.5f;

            while ( panim > 1.0f )
                panim -= 1.0f;
        }
        else
            panim = 0.0f;*/
    }

    void GameScene::OnMapBegin( const char* mapName )
    {
        ClearEntities();
    }

    void GameScene::OnMapEditorData( const glm::vec3& viewPos, float zoom )
    {
    }

    void GameScene::OnMapEntity( const WorldEnt& const_ent )
    {
        size_t i = entities.add(const_ent);

#ifdef ENABLE_COLLDET
        WorldEnt& ent = entities.getUnsafe(i);

        switch (ent.type)
        {
            case ENT_CUBE:
                ent.coll = physWorld->CreateBoxColl(ent.size);
                break;
        }
#endif
    }

    void GameScene::ReleaseMedia()
    {
        li::destroy( worldShader );
        li::release( white_tex );

        /*li::destroy( menuFont );
        li::destroy( uiFont );*/
    }

    void GameScene::ReloadMedia()
    {
        media::DIBitmap whitetex;

        worldShader = Loader::LoadShadingProgram( "shader/world" );
        media::PresetFactory::CreateBitmap2x2( COLOUR_WHITE, media::TEX_RGBA8, &whitetex );
        white_tex = Loader::CreateTexture("", &whitetex);

        /*menuFont = Loader::OpenFont( "media/font/coal.ttf", 72, 0, 0, 255 );
        uiFont = Loader::OpenFont( "media/font/DejaVuSans.ttf", 12, FONT_BOLD, 0, 255 );*/

        R::EnableDepthTest( true );
        R::SetBlendMode( BLEND_NORMAL );
        R::SetClearColour( glm::vec4(0.0f, 0.0f, 0.0f, 0.0f) );

        LoadMap();
    }

    bool GameScene::TestAABB_World(const glm::vec3 c[2])
    {
        iterate2 (i, entities)
        {
            const WorldEnt& ent = i;

            switch (ent.type)
            {
                case ENT_CUBE:
                {
                    const glm::vec3 range[] = { ent.pos, ent.pos + ent.size };

                    if (Collision::TestAABB_AABB(c, range))
                        return true;
                }
            }
        }

        return false;
    }
}
