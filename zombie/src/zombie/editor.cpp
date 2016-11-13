
#include "zombie.hpp"

static bool herp = true;

enum { CMENU_BUILD, CMENU_MAIN, CMENU_PROPERTIES };
enum { C_BUILD_CUBOID, C_BUILD_PLANE, C_DELETE_ENT, C_EXIT_EDITOR, C_LOAD, C_MOVE_ENT, C_NEW, C_RESIZE_ENT, C_SAVE, C_SAVE_AS, C_SAVE_AS_OK };

const auto COLOUR_RED = glm::vec4(1.0, 0.0, 0.0, 1.0);
const auto COLOUR_GREEN = glm::vec4(0.0, 1.0, 0.0, 1.0);
const auto COLOUR_BLUE = glm::vec4(0.0, 0.0, 1.0, 1.0);

namespace zombie
{
    static const double glowSpeed = 2.5;

    static const glm::vec2 overlayPos( 50.0f, 50.0f );

    //static String saveAs;

    EditorScene::EditorScene() : mode( MODE_SEL ), selectedEnt(-1), zoom(10.0f)
    {
    }

    EditorScene::~EditorScene()
    {
    }

    void EditorScene::DrawScene()
    {
        R::Clear();

        c = world::Camera( viewPos + glm::vec3(0.5f, 0.5f, 0.5f), zoom, 270.0f / 180.0f * f_pi, 45.0f / 180.0f * f_pi );

        // settings common for cube & text
        R::EnableDepthTest( true );
        R::SelectShadingProgram( worldShader );
        R::SetPerspectiveProjection(1.0f, 500.0f, 45.0f);
        R::SetCamera(c);
        R::SetTexture( white_tex );

        glm::vec3 pos(-5.0f, 0.0f, 1.0f), ambient(1.0f, 0.5f, 0.0f), diffuse(1.0f, 1.0f, 1.0f), specular;
        float range(10.0f);

        R::ClearLights();
        //R::SetAmbient( glm::vec3(0.5f, 0.5f, 0.5f) );
        R::SetAmbient( glm::vec3(COLOUR_WHITE) );
        
        R::SetTexture( nullptr );

        // ground
        //R::SetBlendColour( glm::vec4(0.1f, 0.1f, 0.1f, 1.0f) );
        
        //R::DrawDIMesh( &ground, glm::mat4() );

        // draw the grid
        R::SetRenderFlags( R_3D | R_UVS | R_COLOURS );

        R::SetBlendColour( COLOUR_RED );
        R::DrawLine( glm::vec3(0.0f, 0.0f, 0.01f), glm::vec3(1.0f, 0.0f, 0.01f) );

        R::SetBlendColour( COLOUR_GREEN );
        R::DrawLine( glm::vec3(0.0f, 0.0f, 0.01f), glm::vec3(0.0f, 1.0f, 0.01f) );

        R::SetBlendColour( COLOUR_BLUE );
        R::DrawLine( glm::vec3(0.0f, 0.0f, 0.01f), glm::vec3(0.0f, 0.0f, 1.01f) );

        R::SetBlendColour( COLOUR_GREY(0.5f) );

        static const float r = 20.0f;

        for ( float x = viewPos.x - r; x <= viewPos.x + r; x += 1.0f )
            R::DrawLine( glm::vec3(x, viewPos.y - r, 0.005f), glm::vec3(x, viewPos.y + r, 0.005f) );
       
        for ( float y = viewPos.y - r; y <= viewPos.y + r; y += 1.0f )
            R::DrawLine( glm::vec3(viewPos.x - r, y, 0.005f), glm::vec3(viewPos.x + r, y, 0.005f) );        

        // Draw world
        iterate2 (i, entities)
        {
            const WorldEnt& ent = i;

            R::SetBlendColour( ent.colour );

            if ( ent.type == ENT_CUBE )
                R::DrawDIMesh( &buildCube, glm::scale(glm::translate(glm::mat4(), ent.pos), ent.size) );
            else
                R::DrawDIMesh( &buildPlane, glm::scale(glm::translate(glm::mat4(), ent.pos), ent.size) );

            if (i.getIndex() == selectedEnt )
            {
                const glm::vec3 offset = ent.size * 0.01f * 0.0f;

                glm::vec3 pos, size;

                if (mode == MODE_MOVE_ENT || mode == MODE_RESIZE_ENT)
                {
                    pos = buildPos;
                    size = buildSize;
                }
                else
                {
                    pos = ent.pos;
                    size = ent.size;
                }

                R::SetBlendMode(BLEND_ADD);
                R::EnableDepthTest( false );
                R::SetBlendColour(COLOUR_WHITEA(glm::abs(selectionGlow) * 0.2f));
                R::DrawDIMesh( &buildCube, glm::scale(glm::translate(glm::mat4(), pos - offset), size + 2.0f * offset) );
                R::SetBlendMode(BLEND_NORMAL);
                R::EnableDepthTest( true );
            }
        }

        // Do this after all other world renderings (we're going no-depthtest)
        R::EnableDepthTest( false );
        R::SetTexture( selection_tex );
        R::SetBlendColour( COLOUR_WHITE );

        if ( mode == MODE_INSERT_ENT )
        {
            switch ( entType )
            {
                case ENT_CUBE: R::DrawDIMesh( &buildCube, glm::scale(glm::translate(glm::mat4(), buildPos), buildSize) ); break;
                case ENT_PLANE: R::DrawDIMesh( &buildPlane, glm::scale(glm::translate(glm::mat4(), buildPos), buildSize) ); break;
            }
        }
        else
        {
            if ( viewPos.z > 0.01f || viewPos.z < -0.01f )
            {
                //R::DrawTexture( selection_tex, glm::vec3(viewPos.x, viewPos.y, 0.0f), glm::vec2(1.0f, 1.0f) );

                R::SetBlendColour( glm::vec4(1.0f,1.0f,1.0f, 0.5f) );
                R::DrawRectangle( glm::vec2(viewPos), glm::vec2(viewPos.x+1.0f, viewPos.y+1.0f), 0.0f );
                R::SetBlendColour( COLOUR_WHITE );
            }

            R::DrawDIMesh( &buildCube, glm::translate(glm::mat4(), glm::vec3(viewPos.x, viewPos.y, viewPos.z)) );
        }

        // menu labels

        // 2d overlay
        R::EnableDepthTest( false );
        R::SelectShadingProgram( nullptr );
        R::Set2DView();
        R::SetTexture( nullptr );
        R::SetRenderFlags(R_UVS | R_COLOURS);

        uiFont->DrawString( Sys::tmpsprintf(50, "CURSOR: (%g, %g, %g)", viewPos.x, viewPos.y, viewPos.z), glm::vec3( 20, 20, 0 ), COLOUR_WHITE, TEXT_LEFT|TEXT_TOP );

        if ( mode == MODE_INSERT_ENT )
        {
            switch ( entType )
            {
                case ENT_CUBE: 
                    uiFont->DrawString( Sys::tmpsprintf(50, "POS: (%g, %g, %g) SIZE: (%g, %g, %g)", buildPos.x, buildPos.y, buildPos.z, buildSize.x, buildSize.y, buildSize.z),
                        glm::vec3( 20, 20 + uiFont->GetLineSkip(), 0 ), COLOUR_WHITE, TEXT_LEFT|TEXT_TOP );
                    break;
        
                case ENT_PLANE:
                    uiFont->DrawString( Sys::tmpsprintf(50, "POS: (%g, %g, %g) SIZE: (%g, %g)", buildPos.x, buildPos.y, buildPos.z, buildSize.x, buildSize.y),
                        glm::vec3( 20, 20 + uiFont->GetLineSkip(), 0 ), COLOUR_WHITE, TEXT_LEFT|TEXT_TOP );
                    break;
            }
        }

        DrawButtonIcon(r_viewportw->basictypes.i / 2 + 8, r_viewporth->basictypes.i - 72, 6, 0);

        DrawButtonIcon(r_viewportw->basictypes.i / 2 - 84, r_viewporth->basictypes.i - 52, 7, 0);
        DrawButtonIcon(r_viewportw->basictypes.i / 2 - 64, r_viewporth->basictypes.i - 52, 10, 0);
        DrawButtonIcon(r_viewportw->basictypes.i / 2 - 44, r_viewporth->basictypes.i - 52, 0, 0);
        DrawButtonIcon(r_viewportw->basictypes.i / 2 - 24, r_viewporth->basictypes.i - 52, 1, 0);
        DrawButtonIcon(r_viewportw->basictypes.i / 2 + 8, r_viewporth->basictypes.i - 52, 5, 0);

        DrawButtonIcon(r_viewportw->basictypes.i / 2 - 124, r_viewporth->basictypes.i - 32, 0, 0);
        DrawButtonIcon(r_viewportw->basictypes.i / 2 - 104, r_viewporth->basictypes.i - 32, 1, 0);
        DrawButtonIcon(r_viewportw->basictypes.i / 2 - 84, r_viewporth->basictypes.i - 32, 2, 0);
        DrawButtonIcon(r_viewportw->basictypes.i / 2 - 64, r_viewporth->basictypes.i - 32, 3, 0);
        DrawButtonIcon(r_viewportw->basictypes.i / 2 - 44, r_viewporth->basictypes.i - 32, 8, 0);
        DrawButtonIcon(r_viewportw->basictypes.i / 2 - 24, r_viewporth->basictypes.i - 32, 9, 0);
        DrawButtonIcon(r_viewportw->basictypes.i / 2 + 8, r_viewporth->basictypes.i - 32, 4, 0);
        //DrawButtonIcon(r_viewportw->basictypes.i / 2 + 28, r_viewporth->basictypes.i - 32, 5, 0);

        uiFont->DrawString(str_insert, glm::vec3(r_viewportw->basictypes.i / 2 + 32, r_viewporth->basictypes.i - 64, 0.0f), COLOUR_WHITE, TEXT_LEFT | TEXT_MIDDLE | TEXT_SHADOW);

        uiFont->DrawString("ZOOM", glm::vec3(r_viewportw->basictypes.i / 2 - 92, r_viewporth->basictypes.i - 44, 0.0f), COLOUR_WHITE, TEXT_RIGHT | TEXT_MIDDLE | TEXT_SHADOW);
        uiFont->DrawString("EDIT", glm::vec3(r_viewportw->basictypes.i / 2 + 32, r_viewporth->basictypes.i - 44, 0.0f), COLOUR_WHITE, TEXT_LEFT | TEXT_MIDDLE | TEXT_SHADOW);

        uiFont->DrawString(str_move, glm::vec3(r_viewportw->basictypes.i / 2 - 132, r_viewporth->basictypes.i - 24, 0.0f), COLOUR_WHITE, TEXT_RIGHT | TEXT_MIDDLE | TEXT_SHADOW);
        uiFont->DrawString(str_select, glm::vec3(r_viewportw->basictypes.i / 2 + 32, r_viewporth->basictypes.i - 24, 0.0f), COLOUR_WHITE, TEXT_LEFT | TEXT_MIDDLE | TEXT_SHADOW);

        iterate2 ( e, overlays )
            e->Draw( overlayPos, glm::vec2( r_viewportw->basictypes.i, r_viewporth->basictypes.i ) - 2.0f * overlayPos );
    }

    void EditorScene::Exit()
    {
        reverse_iterate2 ( e, overlays )
        {
            e->Exit();
            delete e;
        }

        ReleaseMedia();
    }

    int EditorScene::FindEntAtPos( const glm::vec3& pos, int index )
    {
        iterate2(i, entities)
        {
            const WorldEnt& ent = i;

            bool isAtPos = ( pos.x >= ent.pos.x && pos.x < ent.pos.x + ent.size.x && pos.y >= ent.pos.y && pos.y < ent.pos.y + ent.size.y );

            if (ent.type == ENT_CUBE)
                isAtPos = isAtPos && ( pos.z >= ent.pos.z && pos.z < ent.pos.z + ent.size.z );
            else if (ent.type == ENT_PLANE)
                isAtPos = isAtPos && ( pos.z == ent.pos.z );

            if (isAtPos)
                return i.getIndex();
        }

        return -1;
    }

    void EditorScene::Init()
    {
        //menuFont = nullptr;
        uiFont = nullptr;

        // video settings
        Video::EnableKeyRepeat( true );

        // remote media & render settings
        ReloadMedia();

        // local media
        //c = world::Camera( viewPos, 45.0f, 270.0f / 180.0f * f_pi, 45.0f / 180.0f * f_pi );
        media::PresetFactory::CreatePlane( glm::vec2(1.0f, 1.0f), glm::vec3(), media::ALL_SIDES | media::WITH_NORMALS, &buildPlane );
        media::PresetFactory::CreateCube( glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(), media::ALL_SIDES | media::WITH_UVS, &buildCube );

        inputs = 0;
        inputInterval = 0.0f;
        selectionGlow = 0.0f;
        r_viewportw = Var::LockInt("r_viewportw", false);
        r_viewporth = Var::LockInt("r_viewporth", false);
        in_mousex = Var::LockInt("in_mousex", false);
        in_mousey = Var::LockInt("in_mousey", false);

        str_insert = Sys::Localize("BUILD");
        str_move = Sys::Localize("MOVE CURSOR");
        str_select = Sys::Localize("SELECT");

        NewMap();
    }

    void EditorScene::LoadMap()
    {
        Reference<SeekableInputStream> mapFile = Sys::OpenInput( "maps/" + saveName + ".ds" );

        datafile::SDStream sds(mapFile.detach());

        MapLoader loader(sds);

        loader.Init();
        loader.LoadMap(this);
        loader.Exit();
    }

    void EditorScene::NewMap()
    {
        viewPos = glm::vec3(0.0f, 0.0f, 0.0f);
        entities.clear(false);

        saveName.clear();
    }

    void EditorScene::OnFrame( double delta )
    {
        Event_t* event;

        static const float ms = 1.0f;

        reverse_iterate2 ( e, overlays )
        {
            e->OnFrame( delta );

            if ( e->IsFinished() )
            {
                FileChooserOverlay* fileChooser = dynamic_cast<FileChooserOverlay*>( *e );
                
                if ( fileChooser != nullptr && fileChooser->GetSelection() >= 0 )
                {
                    int cmd = fileChooser->GetID();

                    switch ( cmd )
                    {
                        case C_LOAD:
                            saveName = fileChooser->GetSelectionName();
                            LoadMap();
                            break;

                        case C_SAVE_AS:
                            saveName = fileChooser->GetSelectionName();
                            SaveMap();
                            break;
                    }
                }

                MenuOverlay* menu = dynamic_cast<MenuOverlay*>( *e );
                
                if ( menu != nullptr )
                {
                    int cmd = menu->GetSelectionID();

                    switch ( cmd )
                    {
                        case C_BUILD_CUBOID:
                            mode = MODE_INSERT_ENT;
                            buildOrigin = viewPos;

                            selectedEnt = -1;
                            entType = ENT_CUBE;
                            break;

                        case C_BUILD_PLANE:
                            mode = MODE_INSERT_ENT;
                            buildOrigin = viewPos;

                            selectedEnt = -1;
                            entType = ENT_PLANE;
                            break;

                        case C_DELETE_ENT:
                            entities.remove(selectedEnt);
                            UpdateSelection();
                            break;

                        case C_EXIT_EDITOR:
                            Sys::ChangeScene( new TitleScene() );
                            break;

                        case C_LOAD:
                        {
                            FileChooserOverlay* fileChooser = new FileChooserOverlay(FileChooserOverlay::P_OPEN, Sys::Localize("Select a file to open:"), C_LOAD, 0);
                            fileChooser->Init();
                            fileChooser->AddDirContents("maps", ".ds");
                            fileChooser->Compile();
                            overlays.add( fileChooser );
                            break;
                        }

                        case C_MOVE_ENT:
                            buildOrigin = viewPos;
                            viewPos = entities[selectedEnt].pos;
                            buildSize = entities[selectedEnt].size;
                            mode = MODE_MOVE_ENT;
                            break;

                        case C_NEW:
                            NewMap();
                            break;

                        case C_RESIZE_ENT:
                            buildOrigin = entities[selectedEnt].pos;
                            viewPos = buildOrigin + entities[selectedEnt].size;
                            
                            if (entType == ENT_CUBE)
                                viewPos -= glm::vec3(1.0f, 1.0f, 1.0f);
                            else
                                viewPos -= glm::vec3(1.0f, 1.0f, 0.0f);

                            mode = MODE_RESIZE_ENT;
                            break;

                        case C_SAVE:
                            SaveMap();
                            break;

                        case C_SAVE_AS:
                        {
                            FileChooserOverlay* fileChooser = new FileChooserOverlay(FileChooserOverlay::P_SAVE_AS, Sys::Localize("Save as:"), C_SAVE_AS, FileChooserOverlay::CONFIRM_OVERWRITE);
                            fileChooser->Init();
                            fileChooser->AddDirContents("maps", ".ds");
                            fileChooser->Compile();
                            overlays.add( fileChooser );
                            break;
                        }

                        /*case C_SAVE_AS_OK:
                            saveName = saveAs;
                            SaveMap();
                            break;*/
                    }
                }

                e->Exit();
                delete e;
                overlays.removeItem( e );
            }
        }

        while ( ( event = Event::Pop() ) != nullptr )
        {
            switch ( event->type )
            {
                case EV_VKEY:
                    if ( event->vkey.vk.inclass == IN_OTHER && event->vkey.vk.key == OTHER_CLOSE )
                    {
                        Sys::BreakGameLoop();
                        return;
                    }

                    if ( event->vkey.flags & VKEY_PRESSED )
                    {
                        if ( IsAnyUp(event->vkey) )     inputs |= 1;
                        if ( IsAnyDown(event->vkey) )   inputs |= 2;
                        if ( IsAnyLeft(event->vkey) )   inputs |= 4;
                        if ( IsAnyRight(event->vkey) )  inputs |= 8;
                        if ( IsL(event->vkey) )      inputs |= 16;
                        if ( IsR(event->vkey) )      inputs |= 32;
                        if ( IsY(event->vkey) )      inputs |= 64;
                    }
                    else
                    {
                        if ( IsAnyUp(event->vkey) )     inputs &= ~1;
                        if ( IsAnyDown(event->vkey) )   inputs &= ~2;
                        if ( IsAnyLeft(event->vkey) )   inputs &= ~4;
                        if ( IsAnyRight(event->vkey) )  inputs &= ~8;
                        if ( IsL(event->vkey) )      inputs &= ~16;
                        if ( IsR(event->vkey) )      inputs &= ~32;
                        if ( IsY(event->vkey) )      inputs &= ~64;
                    }

                    if ( mode == MODE_SEL )
                    {
                        if ( IsA(event->vkey) && event->vkey.triggered() )
                        {
                            if (selectedEnt >= 0)
                            {
                                MenuOverlay* menu = new MenuOverlay( Sys::Localize("PROPERTIES"), CMENU_PROPERTIES );
                                menu->Init();
                                menu->AddColour3( Sys::Localize("Colour"), &entities[selectedEnt].colour, 0 );
                                menu->AddCommand( Sys::Localize("Texture"), -1, 0 );
                                menu->AddToggle( Sys::Localize("Solid?"), herp, 0 );
                                menu->Compile();
                                overlays.add( menu );
                            }
                        }
                        else if ( IsB(event->vkey) && event->vkey.triggered() )
                        {
                            if (selectedEnt >= 0)
                            {
                                MenuOverlay* menu = new MenuOverlay( Sys::Localize("EDIT") );
                                menu->Init();
                                menu->AddCommand( Sys::Localize("Move"), C_MOVE_ENT, 0 );
                                menu->AddCommand( Sys::Localize("Resize"), C_RESIZE_ENT, 0 );
                                menu->AddCommand( Sys::Localize("Delete"), C_DELETE_ENT, 0 );
                                menu->Compile();
                                overlays.add( menu );
                            }
                        }
                        else if ( IsX(event->vkey) && event->vkey.triggered() )
                        {
                            MenuOverlay* menu = new MenuOverlay( Sys::Localize("BUILD"), CMENU_BUILD );
                            menu->Init();
                            menu->AddCommand( Sys::Localize("Plane"), C_BUILD_PLANE, 0 );
                            menu->AddCommand( Sys::Localize("Cuboid"), C_BUILD_CUBOID, 0 );
                            menu->AddCommand( Sys::Localize("Cancel"), -1, 0 );
                            menu->Compile();
                            overlays.add( menu );
                        }
                        else if ( IsAnyMenu(event->vkey) && event->vkey.triggered() )
                        {
                            MenuOverlay* menu = new MenuOverlay( "", CMENU_MAIN );
                            menu->Init();
                            menu->AddCommand( Sys::Localize("New Map"), C_NEW, 0 );
                            menu->AddCommand( Sys::Localize("Load Map"), C_LOAD, 0 );

                            if ( !saveName.isEmpty() )
                                menu->AddCommand( Sys::Localize("Save Map As '" + saveName + "'"), C_SAVE, 0 );

                            menu->AddCommand( Sys::Localize("Save Map As..."), C_SAVE_AS, 0 );
                            menu->AddCommand( Sys::Localize("Exit Editor"), C_EXIT_EDITOR, 0 );
                            menu->Compile();
                            overlays.add( menu );
                        }
                    }
                    
                    if ( mode == MODE_INSERT_ENT )
                    {
                        if ( IsA(event->vkey) && event->vkey.triggered() )
                        {
                            WorldEnt ent = { entType, buildPos, buildSize, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f), "" };
                            entities.add(ent);

                            mode = MODE_SEL;
                        }
                        else if ( IsB(event->vkey) && event->vkey.triggered() )
                        {
                            viewPos = buildOrigin;
                            mode = MODE_SEL;
                        }
                    }
                    else if ( mode == MODE_MOVE_ENT )
                    {
                        if ( IsA(event->vkey) && event->vkey.triggered() )
                        {
                            entities[selectedEnt].pos = buildPos;
                            mode = MODE_SEL;
                        }
                        else if ( IsB(event->vkey) && event->vkey.triggered() )
                        {
                            viewPos = buildOrigin;
                            mode = MODE_SEL;
                        }
                    }
                    else if ( mode == MODE_RESIZE_ENT )
                    {
                        if ( IsA(event->vkey) && event->vkey.triggered() )
                        {
                            entities[selectedEnt].pos = buildPos;
                            entities[selectedEnt].size = buildSize;
                            mode = MODE_SEL;
                        }
                        else if ( IsB(event->vkey) && event->vkey.triggered() )
                        {
                            viewPos = buildOrigin;
                            mode = MODE_SEL;
                        }
                    }

                    break;
            }
        }

        if ( inputs != 0 && inputInterval <= 0.0f )
        {
            if ( !(inputs & 64) )
            {
                if ( inputs & 1 )
                    viewPos.y -= ms;
                if ( inputs & 2 )
                    viewPos.y += ms;
                if ( inputs & 4 )
                    viewPos.x -= ms;
                if ( inputs & 8 )
                    viewPos.x += ms;
                if ( inputs & 16 )
                    viewPos.z -= ms;
                if ( inputs & 32 )
                    viewPos.z += ms;
            }
            else
            {
                if ( (inputs & 1) && zoom > 1.5f )
                    zoom *= 1.0f / 1.2f;
                if ( (inputs & 2) )
                    zoom *= 1.2f;
            }

            inputInterval = 0.1f;

            // update selection
            if (mode == MODE_SEL)
                UpdateSelection();
        }

        if ( inputInterval > 0.0f )
            inputInterval -= (float) delta;

        selectionGlow += (float)(delta * glowSpeed);

        while ( selectionGlow > 1.0f )
            selectionGlow -= 2.0f;

        UpdateBuildingMinMax();
    }

    void EditorScene::OnMapBegin( const char* mapName )
    {
    }

    void EditorScene::OnMapEditorData( const glm::vec3& viewPos, float zoom )
    {
        this->viewPos = viewPos;
        this->zoom = zoom;
    }

    void EditorScene::OnMapEntity( const WorldEnt& ent )
    {
        entities.add( ent );
    }

    void EditorScene::ReleaseMedia()
    {
        li::destroy( worldShader );
        li::release( white_tex );

        li::release( selection_tex );

        //li::destroy( menuFont );
        li::destroy( uiFont );
    }

    void EditorScene::ReloadMedia()
    {
        media::DIBitmap whitetex;

        worldShader = Loader::LoadShadingProgram( "shader/world" );
        media::PresetFactory::CreateBitmap2x2( COLOUR_WHITE, media::TEX_RGBA8, &whitetex );
        white_tex = Loader::CreateTexture("", &whitetex);

        selection_tex = Loader::LoadTexture( "editor/selection.png", true );

        //menuFont = Loader::OpenFont( "media/font/coal.ttf", 72, 0, 0, 255 );
        uiFont = Loader::OpenFont( "media/font/DejaVuSans.ttf", 12, FONT_BOLD, 0, 255 );

        R::SetBlendMode( BLEND_NORMAL );
        R::SetClearColour( glm::vec4(0.0f, 0.0f, 0.0f, 0.0f) );
    }

    void EditorScene::SaveMap()
    {
        Reference<OutputStream> mapFile = Sys::OpenOutput("maps/" + saveName + ".ds", STORAGE_LOCAL);

        datafile::SDSWriter sw(mapFile.detach());

        MapWriter wr(sw);

        wr.Init("");

        wr.EntitiesBegin();

        iterate2(i, entities)
            wr.EntitiesAdd( i );

        wr.EntitiesEnd();

        wr.EditorData( viewPos, zoom );

        wr.Exit();
    }

    void EditorScene::UpdateBuildingMinMax()
    {
        if ( mode == MODE_MOVE_ENT )
        {
            buildPos = viewPos;
            return;
        }

        if ( entType == ENT_CUBE )
        {
            const glm::vec3 min = glm::min(viewPos, buildOrigin);
            const glm::vec3 max = glm::max(viewPos, buildOrigin) + glm::vec3(1.0f, 1.0f, 1.0f);

            buildPos = min;
            buildSize = max - min;
        }
        else if ( entType == ENT_PLANE )
        {
            buildOrigin.z = viewPos.z;

            const glm::vec3 min = glm::min(viewPos, buildOrigin);
            const glm::vec3 max = glm::max(viewPos, buildOrigin) + glm::vec3(1.0f, 1.0f, 0.0f);

            buildPos = min;
            buildSize = max - min;
        }
    }

    void EditorScene::UpdateSelection()
    {
        selectedEnt = FindEntAtPos( viewPos );

        if ( selectedEnt >= 0 )
            entType = entities[selectedEnt].type;
    }
}