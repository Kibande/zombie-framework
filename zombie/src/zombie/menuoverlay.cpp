
#include "zombie.hpp"

namespace zombie
{
    static const float sliderspeed = 0.65f;

    static const glm::vec2 padding(20.0f, 20.0f);

    static const int innerpad = 20;
    static const int minSpacing = 36;

    MenuOverlay::MenuOverlay( const char* query, int id )
        : id(id), query(query), submenu(nullptr)
    {
    }

    int MenuOverlay::AddColour3( const char* label, glm::vec4* value, int flags )
    {
        MenuItem item = { MenuItem::colour3, label, flags, -1, nullptr, nullptr, 0.0f, 0.0f, value };

        return items.add( item );
    }

    int MenuOverlay::AddCommand( const char* label, int cid, int flags )
    {
        MenuItem item = { MenuItem::command, label, flags, cid };

        return items.add( item );
    }

    int MenuOverlay::AddSlider( const char* label, float& value, float min, float max, int flags )
    {
        MenuItem item = { MenuItem::slider, label, flags, -1, nullptr, &value, min, max };

        return items.add( item );
    }

    int MenuOverlay::AddTextField( const char* label, String* value, int flags )
    {
        MenuItem item = { MenuItem::textInput, label, flags, -1, nullptr, nullptr, 0.0f, 0.0f, nullptr, value };

        return items.add( item );
    }

    int MenuOverlay::AddToggle( const char* label, bool& value, int flags )
    {
        MenuItem item = { MenuItem::toggle, label, flags, -1, &value };

        return items.add( item );
    }

    void MenuOverlay::Compile()
    {
        selection = 0;

        while (items[selection].flags & MenuItem::DISABLED)
        {
            if ((unsigned)(++selection) >= items.getLength())
            {
                selection = -1;
                break;
            }
        }

        width = 0;

        if ( !query.isEmpty() )
            width = font_ui_medium->GetTextSize( query ).x;

        for (size_t i = 0; i < items.getLength(); i++)
        {
            switch ( items[i].t )
            {
                case MenuItem::command:
                    width = glm::max( width, font_ui_medium->GetTextSize( items[i].label ).x );
                    break;

                case MenuItem::textInput:
                case MenuItem::toggle:
                case MenuItem::colour3:
                case MenuItem::slider:
                    width = glm::max( width, 2 * (font_ui_medium->GetTextSize( items[i].label ).x + innerpad) );
                    break;
            }
        }

        width += 2 * innerpad;
    }

    MenuOverlay* MenuOverlay::CreateColourEditor( const char* label, glm::vec4* value )
    {
        MenuOverlay* submenu = new MenuOverlay( label, -1 );
        submenu->Init();
        submenu->AddColour3( Sys::Localize("Preview"), value, MenuItem::DISABLED );
        submenu->AddSlider( Sys::Localize("Red"), value->r, 0.0f, 1.0f, 0 );
        submenu->AddSlider( Sys::Localize("Green"), value->g, 0.0f, 1.0f, 0 );
        submenu->AddSlider( Sys::Localize("Blue"), value->b, 0.0f, 1.0f, 0 );
        submenu->AddCommand( Sys::Localize("OK"), -1, 0 );
        submenu->Compile();
        return submenu;
    }

    void MenuOverlay::Draw( const glm::vec2& pos, const glm::vec2& maxSize )
    {
        if ( state == FINISHED )
            return;

        int ls = font_ui_medium->GetLineSkip();
        int spacing = glm::max(ls, minSpacing);
        
        glm::vec2 size = glm::min( glm::vec2( width, ( (!query.isEmpty() ? 1.5f : 0.0f) + items.getLength() ) * spacing + 2 * innerpad ), maxSize );

		R::SelectShadingProgram( nullptr );
		R::SetTexture( nullptr );

        R::SetBlendColour(glm::vec4(0.0f, 0.0f, 0.0f, alpha * 0.85f));
        R::SetRenderFlags( R_UVS | R_COLOURS );
        R::DrawRectangle(pos, pos + size, 0.0f);

        glm::vec3 drawPos = glm::vec3(pos.x + size.x / 2, pos.y + innerpad + spacing * 0.5f, 0.0f);

        if ( !query.isEmpty() )
        {
            font_ui_medium->DrawString( query, drawPos, glm::vec4(1.0f, 1.0f, 1.0f, alpha), TEXT_CENTER | TEXT_MIDDLE );
            drawPos.y += 1.5f * spacing;
        }

        // RUN 1 (text texture)

        for (size_t i = 0; i < items.getLength(); i++)
        {
            const float ialpha = alpha * ((i == selection) ? 0.8f : 0.4f);

            switch ( items[i].t )
            {
                case MenuItem::command:
                    font_ui_medium->DrawString( items[i].label, drawPos, glm::vec4(1.0f, 1.0f, 1.0f, ialpha), TEXT_CENTER | TEXT_MIDDLE );
                    break;

                case MenuItem::textInput:
                case MenuItem::toggle:
                case MenuItem::colour3:
                case MenuItem::slider:
                   font_ui_medium->DrawString( items[i].label, drawPos + glm::vec3(-innerpad, 0.0f, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, ialpha), TEXT_RIGHT | TEXT_MIDDLE );
                   break;
            }
            
            drawPos.y += spacing;
        }

        // RUN 2 (ui stuff texture)

        // TODO: clean this up
        drawPos = glm::vec3(pos.x + size.x / 2, pos.y + innerpad + spacing * 0.5f, 0.0f);

        if ( !query.isEmpty() )
            drawPos.y += 1.5f * spacing;

        for (size_t i = 0; i < items.getLength(); i++)
        {
            const float ialpha = alpha * ((i == selection) ? 0.8f : 0.4f);

            switch ( items[i].t )
            {
                case MenuItem::textInput:
                    font_ui_medium->DrawString( items[i].tvalue->c_str(), drawPos + glm::vec3(innerpad, 0.0f, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, ialpha), TEXT_LEFT | TEXT_MIDDLE );
                    break;

               case MenuItem::toggle:
                   R::SetBlendColour( COLOUR_WHITEA(ialpha) );
                   DrawUIIcon32( (int)drawPos.x + innerpad, (int)drawPos.y - 16, 0, 0 );

                   if ( *items[i].bValue )
                       DrawUIIcon32( (int)drawPos.x + innerpad, (int)drawPos.y - 16, 1, 0 );
                   break;

               case MenuItem::colour3:
                   R::SetBlendColour( COLOUR_WHITEA(ialpha) );
                   DrawUIIcon32( (int)drawPos.x + innerpad, (int)drawPos.y - 16, 0, 0 );

                   R::SetBlendColour( glm::vec4(items[i].cValue->r, items[i].cValue->g, items[i].cValue->b, alpha) );
                   R::DrawRectangle( glm::vec2(drawPos.x + innerpad + 8, drawPos.y - 8), glm::vec2(drawPos.x + innerpad + 24, drawPos.y + 8), 0 );
                   break;

                case MenuItem::slider:
                   R::SetBlendColour( COLOUR_WHITEA(0.5f * ialpha) );
                   R::DrawRectangle( glm::vec2(drawPos.x + innerpad, drawPos.y - 2), glm::vec2(pos.x + size.x - innerpad, drawPos.y + 2), 0 );

                   float x = drawPos.x + innerpad + glm::floor((pos.x + size.x - innerpad) - (drawPos.x + innerpad)) * ((*items[i].value - items[i].min) / (items[i].max - items[i].min));
                   R::SetBlendColour( COLOUR_WHITEA(0.85f * ialpha) );
                   R::DrawRectangle( glm::vec2(x - 2, drawPos.y - 5), glm::vec2(x + 2, drawPos.y + 5), 0 );
                   break;
            }
            
            drawPos.y += spacing;
        }

        /*R::SetBlendColour(glm::vec4(0.0f, 0.0f, 0.0f, alpha * 0.85f));
        R::SetRenderFlags( R_UVS | R_COLOURS );
        R::DrawRectangle(pos, pos + size, 0.0f);

        int divisor = 1 + (!query.isEmpty() ? 2 : 0) + items.getLength() + 1;
        float y = size.y / divisor;

        glm::vec3 drawPos = glm::vec3(pos.x + size.x / 2, pos.y + y * 1.5f, 0.0f);

        if ( !query.isEmpty() )
        {
            font_ui_medium->DrawString( query, drawPos, glm::vec4(1.0f, 1.0f, 1.0f, alpha), TEXT_CENTER | TEXT_MIDDLE );
            drawPos.y += 2*y;
        }

        // RUN 1 (text texture)

        for (size_t i = 0; i < items.getLength(); i++)
        {
            const float ialpha = alpha * ((i == selection) ? 0.8f : 0.4f);

            switch ( items[i].t )
            {
                case MenuItem::command:
                    font_ui_medium->DrawString( items[i].label, drawPos, glm::vec4(1.0f, 1.0f, 1.0f, ialpha), TEXT_CENTER | TEXT_MIDDLE );
                    break;

                case MenuItem::textInput:
                case MenuItem::toggle:
                case MenuItem::colour3:
                case MenuItem::slider:
                   font_ui_medium->DrawString( items[i].label, drawPos + glm::vec3(-innerpad, 0.0f, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, ialpha), TEXT_RIGHT | TEXT_MIDDLE );
                   break;
            }
            
            drawPos.y += y;
        }

        // RUN 2 (ui stuff texture)

        // TODO: clean this up
        drawPos = glm::vec3(pos.x + size.x / 2, pos.y + y * 1.5f, 0.0f);
        
        if ( !query.isEmpty() )
            drawPos.y += 2*y;

        for (size_t i = 0; i < items.getLength(); i++)
        {
            const float ialpha = alpha * ((i == selection) ? 0.8f : 0.4f);

            switch ( items[i].t )
            {
                case MenuItem::textInput:
                    font_ui_medium->DrawString( items[i].tvalue->c_str(), drawPos + glm::vec3(innerpad, 0.0f, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, ialpha), TEXT_LEFT | TEXT_MIDDLE );
                   break;

               case MenuItem::toggle:
                   R::SetBlendColour( COLOUR_WHITEA(ialpha) );
                   DrawUIIcon32( (int)drawPos.x + innerpad, (int)drawPos.y - 16, 0, 0 );

                   if ( *items[i].bValue )
                       DrawUIIcon32( (int)drawPos.x + innerpad, (int)drawPos.y - 16, 1, 0 );
                   break;

               case MenuItem::colour3:
                   R::SetBlendColour( COLOUR_WHITEA(ialpha) );
                   DrawUIIcon32( (int)drawPos.x + innerpad, (int)drawPos.y - 16, 0, 0 );

                   R::SetBlendColour( glm::vec4(items[i].cValue->r, items[i].cValue->g, items[i].cValue->b, alpha) );
                   R::DrawRectangle( glm::vec2(drawPos.x + innerpad + 8, drawPos.y - 8), glm::vec2(drawPos.x + innerpad + 24, drawPos.y + 8), 0 );
                   break;

                case MenuItem::slider:
                   R::SetBlendColour( COLOUR_WHITEA(0.5f * ialpha) );
                   R::DrawRectangle( glm::vec2(drawPos.x + innerpad, drawPos.y - 2), glm::vec2(pos.x + size.x - innerpad, drawPos.y + 2), 0 );

                   float x = drawPos.x + innerpad + glm::floor((pos.x + size.x - innerpad) - (drawPos.x + innerpad)) * ((*items[i].value - items[i].min) / (items[i].max - items[i].min));
                   R::SetBlendColour( COLOUR_WHITEA(0.85f * ialpha) );
                   R::DrawRectangle( glm::vec2(x - 2, drawPos.y - 5), glm::vec2(x + 2, drawPos.y + 5), 0 );
                   break;
            }
            
            drawPos.y += y;
        }*/

        if ( submenu != nullptr )
            submenu->Draw(pos + padding, maxSize);
    }

    void MenuOverlay::Exit()
    {
        if ( submenu != nullptr )
        {
            submenu->Exit();
            li::destroy( submenu );
        }
    }

    int MenuOverlay::GetSelectionID()
    {
        return selection < 0 ? (-1) : items[selection].id;
    }

    void MenuOverlay::Init()
    {
        inputs = 0;
    }

    void MenuOverlay::MoveSlider( MenuItem& item, float delta )
    {
        float t = (*item.value - item.min) / (item.max - item.min);

        t = glm::min( glm::max( t + delta, 0.0f ), 1.0f );

        *item.value = item.min + t * (item.max - item.min);
    }

    void MenuOverlay::OnFrame( double delta )
    {
        Event_t* event;

        if ( state == FINISHED )
            return;

        if ( submenu != nullptr )
        {
            submenu->OnFrame(delta);

            if ( submenu->IsFinished() )
            {
                submenu->Exit();
                li::destroy( submenu );
            }

            return;
        }

        while ( ( event = Event::Pop() ) != nullptr )
        {
            switch ( event->type )
            {
                case EV_UNICODE_IN:
                    if ( selection>=0 )
                    {
                        if ( items[selection].t == MenuItem::textInput )
                        {
                            int uc = event->uni_cp;

                            if (uc == UNI_BACKSP)
                                items[selection].tvalue->set(items[selection].tvalue->dropRightPart(1));
                            else if (uc >= 32)
                                items[selection].tvalue->append(UnicodeChar(uc));
                        }
                    }
                    break;

                case EV_VKEY:
                    if ( event->vkey.vk.inclass == IN_OTHER && event->vkey.vk.key == OTHER_CLOSE )
                    {
                        Event_t cache;
                        memcpy(&cache, event, sizeof(cache));

                        Event::Push( &cache );
                        return;
                    }

                    if ( state == ACTIVE )
                    {
                        if ( event->vkey.flags & VKEY_PRESSED )
                        {
                            if ( IsAnyUp(event->vkey) )     inputs |= 1;
                            if ( IsAnyDown(event->vkey) )   inputs |= 2;
                            if ( IsAnyLeft(event->vkey) )   inputs |= 4;
                            if ( IsAnyRight(event->vkey) )  inputs |= 8;
                        }
                        else
                        {
                            if ( IsAnyUp(event->vkey) )     inputs &= ~1;
                            if ( IsAnyDown(event->vkey) )   inputs &= ~2;
                            if ( IsAnyLeft(event->vkey) )   inputs &= ~4;
                            if ( IsAnyRight(event->vkey) )  inputs &= ~8;
                        }

                        if ( selection>=0 && IsAnyUp(event->vkey) && event->vkey.triggered() )
                        {
                            do
                            {
                                if (--selection < 0)
                                    selection = items.getLength()-1;
                            }
                            while (items[selection].flags & MenuItem::DISABLED);
                        }
                        else if ( selection>=0 && IsAnyDown(event->vkey) && event->vkey.triggered() )
                        {
                            do
                            {
                            if (++selection >= (int) items.getLength())
                                selection = 0;
                            }
                            while (items[selection].flags & MenuItem::DISABLED);
                        }
                        else if ( selection>=0 && IsConfirm(event->vkey) && event->vkey.triggered() )
                        {
                            if ( items[selection].t == MenuItem::colour3 )
                            {
                                submenu = MenuOverlay::CreateColourEditor( items[selection].label, items[selection].cValue );
                            }
                            else if ( items[selection].t == MenuItem::command )
                                FadeOut();
                            else if ( items[selection].t == MenuItem::toggle )
                                *items[selection].bValue = !*items[selection].bValue;
                        }
                        else if ( IsCancel(event->vkey) && event->vkey.triggered() )
                        {
                            selection = -1;
                            FadeOut();
                        }
                    }

                    break;
            }
        }

        if (state == ACTIVE)
        {
            if ( selection>=0 && (inputs & 4) )
            {
                if (items[selection].t == MenuItem::slider)
                    MoveSlider(items[selection], -(float)(delta * sliderspeed) );
            }
            else if ( selection>=0 && (inputs & 8) )
            {
                if (items[selection].t == MenuItem::slider)
                    MoveSlider(items[selection], +(float)(delta * sliderspeed) );
            }
        }
        
        UIOverlay_OnFrame(delta);
    }
}