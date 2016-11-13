
#include "party.hpp"

namespace party
{
    class ParticlePropertyWidget : public gameui::Widget
    {
        public:
            ;
    };

    ParticleEdScene::ParticleEdScene()
    {
    }

    ParticleEdScene::~ParticleEdScene()
    {
    }

    void ParticleEdScene::CreateUI()
    {
        using namespace gameui;

        uiRes = new UIResManager();
        uiContainer = new WidgetContainer();

        uiRes->Load("contentkit/party/ui.cfx2", uiContainer);
    }

    void ParticleEdScene::DrawScene()
    {
        R::Clear();
        R::Set2DView();

        uiContainer->Draw();

        // settings common for cube & text
        /*R::SelectShadingProgram( cubeShader );
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
#endif*/
    }

    void ParticleEdScene::Exit()
    {
        ReleasePrivateMedia();

        li::destroy(uiContainer);
        li::destroy(uiRes);

        /*current->alpha = 0.0f;
        mainMenu.alpha = 1.0f;*/
    }

    void ParticleEdScene::Init()
    {   
        /*current = &mainMenu;
        trans_to = nullptr;

        cubeColour = current->colour;
        cubeShader = nullptr;
        white_tex = nullptr;

        menuFont = nullptr;
        uiFont = nullptr;

        overlay = nullptr;

        // video settings
        Video::EnableKeyRepeat( true );*/

        // remote media & render settings
        CreateUI();
        ReloadMedia();

        // local media
        /*c = world::Camera( glm::vec3(0.0f, 0.0f, 1.0f), current->zoom, 270.0f / 180.0f * f_pi, 15.0f / 180.0f * f_pi );
        media::PresetFactory::CreateCube( glm::vec3( 2.0f, 2.0f, 2.0f ), glm::vec3( 1.0f, 1.0f, 0.0f ), media::ALL_SIDES | media::WITH_NORMALS, &cube );

        // variables
        r_viewportw = Var::LockInt("r_viewportw", false);
        r_viewporth = Var::LockInt("r_viewporth", false);

        str_select = Sys::Localize("SELECT");
        str_confirm = Sys::Localize("CONFIRM/BACK");*/
    }

    void ParticleEdScene::OnFrame( double delta )
    {
        uiContainer->OnFrame(delta);

        Event_t* event;

        while ( ( event = Event::Pop() ) != nullptr )
        {
            switch ( event->type )
            {
                case EV_VIEWPORT_RESIZE:
                {
                    uiContainer->SetArea(glm::ivec2(0, 0), glm::ivec2(event->viewresize.w, event->viewresize.h));
                    break;
                }

                case EV_UICLICK:
                    if (event->ui_mouse.widget->GetName() == "menu.new")
                    {
                    }
                    else if (event->ui_mouse.widget->GetName() == "menu.exit")
                    {
                        Sys::BreakGameLoop();
                        return;
                    }
                    else if (event->ui_mouse.widget->GetName() == "particleTypes.new")
                    {
                        gameui::ListBox* list;

                        if ((list = dynamic_cast<gameui::ListBox*>(uiContainer->Find("particleTypes.list"))) != nullptr)
                        {
                            gameui::MenuItem* newItem = new gameui::MenuItem(uiRes, "Unnamed_1", nullptr, -1);
                            newItem->SetName(String("particle_type"));
                            newItem->ReloadMedia();
                            list->Add(newItem);
                            list->Relayout();
                        }
                    }
                    break;

                case EV_UIMOUSEDOWN:
                    if (event->ui_mouse.widget->GetName() == "menuBtn")
                    {
                        gameui::Widget* w;

                        if ((w = uiContainer->Find("menu")) != nullptr)
                            w->SetVisible( !w->GetVisible() );

                        if ((w = uiContainer->Find("menuBtnShadow")) != nullptr)
                            if (w != nullptr) w->SetVisible( !w->GetVisible() );
                    }
                    else if (event->ui_mouse.widget->GetName() == "menu.cancel")
                    {
                        gameui::Widget* w;

                        if ((w = uiContainer->Find("menu")) != nullptr)
                            w->SetVisible(false);

                        if ((w = uiContainer->Find("menuBtnShadow")) != nullptr)
                            if (w != nullptr) w->SetVisible(false);
                    }
                    break;

                case EV_VKEY:
                    if ( event->vkey.vk.inclass == IN_OTHER && event->vkey.vk.key == OTHER_CLOSE )
                    {
                        Sys::BreakGameLoop();
                        return;
                    }
                    /*
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
                    }*/

                    break;

                    case EV_MOUSE_BTN:
						if (event->mouse.button == MOUSE_BTN_LEFT)
							uiContainer->OnMousePrimary(-1, event->mouse.x, event->mouse.y, !!(event->mouse.flags & VKEY_PRESSED));
						break;

					case EV_MOUSE_MOVE:
						uiContainer->OnMouseMove(-1, event->mouse.x, event->mouse.y);
						break;
            }
        }

        /*// transition in progress?
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
        }*/
    }

    void ParticleEdScene::ReleaseMedia()
    {
        ReleasePrivateMedia();
        SharedReleaseMedia();
    }

    void ParticleEdScene::ReleasePrivateMedia()
    {
        uiRes->ReleaseMedia();
        //uiContainer->ReleaseMedia();

        /*li::destroy( cubeShader );
        li::destroy( white_tex );

        li::destroy( menuFont );
        li::destroy( uiFont );*/
    }

    void ParticleEdScene::ReloadMedia()
    {
        /*int r_viewportw = Var::GetInt("r_viewportw", true);
        int r_viewporth = Var::GetInt("r_viewporth", true);*/

        //media::DIBitmap whitetex;

        SharedReloadMedia();

        uiRes->ReloadMedia();
        uiContainer->ReloadMedia();

        int r_viewportw = Var::GetInt("r_viewportw", true);
        int r_viewporth = Var::GetInt("r_viewporth", true);

        uiContainer->SetArea(glm::ivec2(), glm::ivec2(r_viewportw, r_viewporth));

        /*cubeShader = Loader::LoadShadingProgram( "shader/menu" );
        media::PresetFactory::CreateBitmap2x2( COLOUR_WHITE, media::TEX_RGBA8, &whitetex );
        white_tex = Loader::CreateTexture("", &whitetex);

        menuFont = Loader::OpenFont( "media/font/coal.ttf", 72, 0, 0, 255 );
        uiFont = Loader::OpenFont( "media/font/DejaVuSans.ttf", 10, FONT_BOLD, 0, 255 );*/

        R::EnableDepthTest( false );
        R::SetRenderFlags(R_COLOURS | R_UVS);
        R::SetClearColour(glm::vec4(0.3f, 0.3f, 0.3f, 0.0f));
    }
}