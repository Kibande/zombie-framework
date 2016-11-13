
#include "zombie.hpp"

namespace zombie
{
    // Front Side
    static const char* mainMenu_items[] = { "NEW GAME", "LOAD GAME", "EDITOR", "OPTIONS", "ABOUT", "EXIT" };
    static Menu_t mainMenu = { mainMenu_items, 6, 0,
            0.0f, 0.8f, 5.0f, 1.0f,
            0, glm::vec4(0.2f, 0.6f, 1.0f, 0.5f), glm::vec4(0.05f, 0.1f, 0.2f, 0.0f), 1.0f };

    // New Game
    static const char* newGame_items[] = { "SINGLEPLAYER", "MULTIPLAYER", "BACK" };
    static Menu_t newGame = { newGame_items, 3, 0,
            90.0f, 0.8f, 5.0f, 1.0f,
            0, glm::vec4(1.0f, 0.25f, 0.2f, 0.5f), glm::vec4(0.1f, 0.1f, 0.1f, 0.0f), 0.0f };

    // Load Game
    static const char* loadGame_items[] = { "- empty -", "- empty -", "- empty -", "- empty -", "- empty -", "BACK" };
    static Menu_t loadGame = { loadGame_items, 6, 0,
            -90.0f, 0.8f, 5.0f, 1.0f,
            0, glm::vec4(0.4f, 1.0f, 0.1f, 0.5f), glm::vec4(0.1f, 0.1f, 0.1f, 0.0f), 0.0f };

    // Options
    static const char* options_items[] = { "set controls", "display resolution", "fullscreen", "anti-aliasing", "save keyboard & gamepad", "back" };
    static Menu_t options = { options_items, 6, 0,
            0.0f, 0.1f, 0.8f, 0.0f,
            0, glm::vec4(0.2f, 0.6f, 1.0f, 0.5f),
            glm::vec4(0.025f, 0.05f, 0.1f, 0.0f),
            0.0f };

    // Gamepad Configuration
    static const char* confGamepad_items[] = { "player 1", "player 2", "player 3", "player 4", "back" };
    static Menu_t confGamepad = { confGamepad_items, 5, 0,
            90.0f, 0.1f, 0.8f, 0.0f,
            0, glm::vec4(0.2f, 0.6f, 1.0f, 0.5f),
            glm::vec4(0.025f, 0.05f, 0.1f, 0.0f),
            0.0f };

    static const char* gamepadSetup_items[1] = { nullptr };
    static Menu_t gamepadSetup = { gamepadSetup_items, 1, MENU_LOCKED,
            90.0f, 0.1f, 0.5f, 0.0f,
            0, glm::vec4(0.2f, 0.6f, 1.0f, 0.5f),
            glm::vec4(0.025f, 0.05f, 0.1f, 0.0f),
            0.0f };

    // About
    static const char* about_items[] = { "ZOMBIE (alpha)", "", "minexew games", "(c) 2012", "version 0.5.1" };
    static Menu_t about = { about_items, 5, MENU_LOCKED,
            0.0f, 0.5f, 5.0f, 1.0f,
            0, glm::vec4(0.6f, 0.6f, 0.6f, 0.5f), glm::vec4(0.025f, 0.05f, 0.1f, 0.0f), 0.0f };

    // Some parameters
    static const float menu_advanceZ = -0.25f;
    static const float text_scale = 0.003f;

    static const float selected_alpha = 1.0f;
    static const float normal_alpha = 0.4f;

    static const float transition_time = 0.4f;

    static const int ANALOG_THRESHOLD = 32;

    // TODO: move
    static Vkey_t lastAnalog = {-1};
    static float inputFreeze = 0.0f;

    static void DumpInputConfig()
    {
        for (int i = 0; i < MAX_GAMEPADS; i++)
            DumpGamepadCfg(i, gamepads[i]);
    }

    static inline bool PressedEnough( const VkeyState_t& vkey )
    {
        if ( vkey.vk.inclass == IN_OTHER )
            return false;

        if ( vkey.vk.inclass == IN_ANALOG )
            return abs(vkey.value) > 64 * 256;

        return (vkey.flags & VKEY_PRESSED) != 0;
    }

    TitleScene::TitleScene()
    {
    }

    TitleScene::~TitleScene()
    {
    }

    void TitleScene::DrawMenu( Menu_t& menu )
    {
        if ( menu.alpha < 0.01f )
            return;

        glm::vec3 pos(0.0f, 0.0f, 1.0f - (menu.num_options * menu.scale * menu_advanceZ) / 2);

        //IFont* font = (menu.font == 0 ? menuFont : uiFont);
        IFont* font = menuFont;

        for ( int i = 0; i < menu.num_options; i++ )
        {
            const float alpha = (i == menu.selection ? selected_alpha : normal_alpha) * menu.alpha;

            TransformText( font, menu.angle, menu.scale * text_scale, menu.dist_from_center, pos.z );
            font->DrawString( menu.options[i], glm::vec3(0.0f, 0.0f, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, alpha), TEXT_CENTER|TEXT_MIDDLE );
            pos.z += menu.scale * menu_advanceZ;
        }
    }

    void TitleScene::DrawScene()
    {
        R::Clear();

        // settings common for cube & text
        R::SelectShadingProgram( cubeShader );
        R::SetPerspectiveProjection(0.1f, 20.0f, 45.0f);
        R::SetCamera(c);

        // menu cube
        R::SetBlendColour( cubeColour );
        R::SetBlendMode( BLEND_ADD );
        R::SetRenderFlags(R_3D | R_NORMALS | R_COLOURS);
        R::SetTexture( white_tex );

        R::DrawDIMesh( &cube, cubeTransform );

        // menu labels
        R::SetRenderFlags(R_3D | R_UVS | R_COLOURS);
        DrawMenu(mainMenu);
        DrawMenu(newGame);
        DrawMenu(loadGame);
        DrawMenu(options);
        DrawMenu(confGamepad);
        DrawMenu(gamepadSetup);
        DrawMenu(about);

        // 2d overlay
        R::SelectShadingProgram( nullptr );
        R::Set2DView();
        R::SetBlendColour(COLOUR_WHITE);
        R::SetBlendMode(BLEND_NORMAL);
        R::SetRenderFlags(R_UVS | R_COLOURS);

        DrawButtonIcon(r_viewportw->basictypes.i / 2 - 44, r_viewporth->basictypes.i - 32, 0, 0);
        DrawButtonIcon(r_viewportw->basictypes.i / 2 - 24, r_viewporth->basictypes.i - 32, 1, 0);
        DrawButtonIcon(r_viewportw->basictypes.i / 2 + 8, r_viewporth->basictypes.i - 32, 4, 0);
        DrawButtonIcon(r_viewportw->basictypes.i / 2 + 28, r_viewporth->basictypes.i - 32, 5, 0);

        uiFont->DrawString(str_select, glm::vec3(r_viewportw->basictypes.i / 2 - 52, r_viewporth->basictypes.i - 24, 0.0f), COLOUR_WHITE, TEXT_RIGHT | TEXT_MIDDLE | TEXT_SHADOW);
        uiFont->DrawString(str_confirm, glm::vec3(r_viewportw->basictypes.i / 2 + 52, r_viewporth->basictypes.i - 24, 0.0f), COLOUR_WHITE, TEXT_LEFT | TEXT_MIDDLE | TEXT_SHADOW);

#ifdef _DEBUG
        uiFont->DrawString( "DEBUG BUILD", glm::vec3( 20, 20, 0 ), COLOUR_WHITE, TEXT_LEFT|TEXT_TOP );
#endif
    }

    void TitleScene::Exit()
    {
        ReleasePrivateMedia();

        current->alpha = 0.0f;
        mainMenu.alpha = 1.0f;
    }

    void TitleScene::Init()
    {
        current = &mainMenu;
        trans_to = nullptr;

        cubeColour = current->colour;
        cubeShader = nullptr;
        white_tex = nullptr;

        menuFont = nullptr;
        uiFont = nullptr;

        overlay = nullptr;

        // video settings
        Video::EnableKeyRepeat( true );

        // remote media & render settings
        ReloadMedia();

        // local media
        c = world::Camera( glm::vec3(0.0f, 0.0f, 1.0f), current->zoom, 270.0f / 180.0f * f_pi, 15.0f / 180.0f * f_pi );
        media::PresetFactory::CreateCube( glm::vec3( 2.0f, 2.0f, 2.0f ), glm::vec3( 1.0f, 1.0f, 0.0f ), media::ALL_SIDES | media::WITH_NORMALS, &cube );

        // variables
        r_viewportw = Var::LockInt("r_viewportw", false);
        r_viewporth = Var::LockInt("r_viewporth", false);

        str_select = Sys::Localize("SELECT");
        str_confirm = Sys::Localize("CONFIRM/BACK");
    }

    void TitleScene::OnFrame( double delta )
    {
        if ( current == &gamepadSetup && gamepadSetup_items[0] == nullptr )
        {
            // TODO: This will fuck up once localized
            static String derp;

            derp = (String)"Press " + Sys::Localize( Event::GetButtonName( confGamepadIndex ) );

            current->options[0] = derp;
        }

        if ( inputFreeze > 0.0f )
            inputFreeze = (float)glm::max(inputFreeze - delta, 0.0);

        Event_t* event;

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

                    if ( trans_to == nullptr )
                    {
                        if (current == &gamepadSetup)
                        {
                            if (memcmp(&lastAnalog, &event->vkey.vk, sizeof(Vkey_t)) == 0)
                                if ((event->vkey.vk.inclass == IN_ANALOG && abs(event->vkey.value) / 256 < ANALOG_THRESHOLD) || (event->vkey.flags & VKEY_RELEASED))
                                    lastAnalog.inclass = -1;

                            if ( inputFreeze < 0.01f && PressedEnough( event->vkey ) && memcmp(&lastAnalog, &event->vkey.vk, sizeof(Vkey_t)) != 0 )
                            {
                                if ( !Video::IsEsc(event->vkey) )
                                    gamepads[gamepadIndex].buttons[confGamepadIndex++] = event->vkey.vk;
                                else
                                    gamepads[gamepadIndex].buttons[confGamepadIndex++].clear();

                                if ( confGamepadIndex >= NUM_BUTTONS )
                                    TransitionTo(&options);
                                else
                                {
                                    memcpy(&lastAnalog, &event->vkey.vk, sizeof(Vkey_t));

                                    inputFreeze = 0.1f;
                                    gamepadSetup_items[0] = nullptr;
                                }
                            }
                        }
                        else if ( !(current->flags & MENU_LOCKED) && (Video::IsUp(event->vkey) || IsUp(event->vkey)) && event->vkey.triggered() )
                        {
                            if (--current->selection < 0)
                                current->selection = current->num_options-1;
                        }
                        else if ( !(current->flags & MENU_LOCKED) && (Video::IsDown(event->vkey) || IsDown(event->vkey)) && event->vkey.triggered() )
                        {
                            if (++current->selection >= current->num_options)
                                current->selection = 0;
                        }
                        else if ( (Video::IsEnter(event->vkey) || IsA(event->vkey)) && event->vkey.triggered() )
                        {
                            if ( current == &mainMenu )
                            {
                                switch (current->selection)
                                {
                                    case 0: TransitionTo( &newGame ); break;
                                    case 1: TransitionTo( &loadGame ); break;
                                    case 2: Sys::ChangeScene( new EditorScene() ); break;
                                    case 3: TransitionTo( &options ); break;
                                    case 4: TransitionTo( &about ); break;
                                    case 5: Sys::BreakGameLoop(); return;
                                }
                            }
                            else if ( current == &newGame )
                            {
                                switch (current->selection)
                                {
                                    case 0: Sys::ChangeScene( new GameScene() ); break;
                                    case 1: Sys::ChangeScene( nullptr ); break;
                                    default: TransitionTo(&mainMenu);
                                }
                            }
                            else if ( current == &options )
                            {
                                switch (current->selection)
                                {
                                    case 0: TransitionTo( &confGamepad ); break;
                                    case 4: DumpInputConfig(); break;
                                    default: TransitionTo(&mainMenu);
                                }
                            }
                            else if ( current == &confGamepad )
                            {
                                if ( current->selection < 4 )
                                {
                                    gamepadIndex = current->selection;
                                    confGamepadIndex = 0;
                                    gamepadSetup_items[0] = nullptr;

                                    TransitionTo( &gamepadSetup );
                                }
                                else
                                    TransitionTo( &options );
                            }
                            else
                                TransitionTo(&mainMenu);
                        }
                        else if ( (Video::IsEsc(event->vkey) || IsB(event->vkey)) && event->vkey.triggered() && current != &mainMenu )
                        {
                            if ( current == &confGamepad )
                                TransitionTo(&options);
                            else
                                TransitionTo(&mainMenu);
                        }
                    }

                    break;
            }
        }

        // transition in progress?
        if ( trans_to != nullptr )
        {
            // advance the transition
            transition = glm::min<>(transition + (float)delta / transition_time, 1.0f);

            // lerp cube colour, bg colour and text alphas
            cubeColour = current->colour * (1.0f-transition) + trans_to->colour * transition;
            R::SetClearColour(current->bg_colour * (1.0f-transition) + trans_to->bg_colour * transition);

            current->alpha = (1.0f - transition);
            trans_to->alpha = transition;

            // rotate the camera
            c.rotateZ((270.0f + current->angle * (1.0f-transition) + trans_to->angle * transition) * f_pi / 180.0f, true);
            c.zoom(current->zoom * (1.0f-transition) + trans_to->zoom * transition, true);

            if ( transition >= 0.999f )
            {
                current = trans_to;
                trans_to = nullptr;
            }
        }
    }

    void TitleScene::ReleaseMedia()
    {
        ReleasePrivateMedia();
        SharedReleaseMedia();
    }

    void TitleScene::ReleasePrivateMedia()
    {
        li::destroy( cubeShader );
        li::release( white_tex );

        li::destroy( menuFont );
        li::destroy( uiFont );
    }

    void TitleScene::ReloadMedia()
    {
        media::DIBitmap whitetex;

        SharedReloadMedia();

        cubeShader = Loader::LoadShadingProgram( "shader/menu" );
        media::PresetFactory::CreateBitmap2x2( COLOUR_WHITE, media::TEX_RGBA8, &whitetex );
        white_tex = Loader::CreateTexture("", &whitetex);

        menuFont = Loader::OpenFont( "media/font/coal.ttf", 72, 0, 0, 255 );
        uiFont = Loader::OpenFont( "media/font/DejaVuSans.ttf", 10, FONT_BOLD, 0, 255 );

        R::EnableDepthTest( false );
        R::SetClearColour( current->bg_colour );
    }

    void TitleScene::TransformText( IFont* font, float angle, float scale, float y, float z )
    {
        glm::mat4 textTransform;

        // rotate whole side of the cube
        textTransform = glm::rotate( textTransform, -angle * (f_pi / 180.0f), glm::vec3(0.0f, 0.0f, 1.0f) );

        // move text up
        textTransform = glm::translate( textTransform, glm::vec3(0.0f, y, z));

        // rotate to change +Y to -Z
        textTransform = glm::rotate( textTransform, -f_pi * 0.5f, glm::vec3(1.0f, 0.0f, 0.0f) );

        // scale the text down
        textTransform = glm::scale( textTransform, glm::vec3(1.0f,1.0f,1.0f) * scale );

        font->SetTransform( textTransform );
    }

    void TitleScene::TransitionTo( Menu_t* menu )
    {
        transition = 0.0f;
        trans_to = menu;
        //trans_to->selection = 0;
    }
}
