
#include "zombie.hpp"

namespace zombie
{
    static const float sliderspeed = 0.65f;

    static const glm::vec2 padding(20.0f, 20.0f);

    static const int innerpad = 32;
    static const int iconsize = 32;

    FileChooserOverlay::FileChooserOverlay( Purpose purpose, const char* query, int id, int flags )
        : id(id), flags(flags), purpose(purpose), view(VIEW_LIST32), query(query), numVisible(-1), submenu(nullptr)
    {
    }

    bool FileChooserOverlay::AddDirContents(const char* path, const char* extension)
    {
        extFilter = extension;
        return Sys::ListDir(path, this);
    }

    void FileChooserOverlay::Compile()
    {
        if ( items.getLength() > 0 )
            selection = 0;
        else
            selection = -1;
    }

    MenuOverlay* FileChooserOverlay::CreateFilenameEditor( const char* label, String* value )
    {
        MenuOverlay* submenu = new MenuOverlay("", -1);
        submenu->Init();
        submenu->AddTextField( label, value, 0 );
        submenu->AddCommand( Sys::Localize("OK"), 1, 0 );
        submenu->AddCommand( Sys::Localize("Cancel"), -1, 0 );
        submenu->Compile();
        return submenu;
    }

    MenuOverlay* FileChooserOverlay::CreateOverwriteQuery( const char* label )
    {
        MenuOverlay* submenu = new MenuOverlay(label, -1);
        submenu->Init();
        submenu->AddCommand( Sys::Localize("Yes"), 1, 0 );
        submenu->AddCommand( Sys::Localize("No"), -1, 0 );
        submenu->Compile();
        return submenu;
    }

    void FileChooserOverlay::Draw( const glm::vec2& pos, const glm::vec2& maxSize )
    {
        if ( state == FINISHED )
            return;

        R::SetBlendColour(glm::vec4(0.0f, 0.0f, 0.0f, alpha * 0.85f));
        R::SetRenderFlags( R_UVS | R_COLOURS );
        R::DrawRectangle(pos, pos + maxSize, 0.0f);

        /*
            The dialog is laid out as follows:

                +------------------------
                | innerpad
                +------------------------ 
                | [if !query.isEmpty]
                | spacing * 0.5
                | query anchor
                | spacing
                +------------------------ 
                | spacing * 0.5
                | item anchor
                | spacing * 0.5
                +------------------------
                | ...
                +------------------------
                | innerpad
                +------------------------
        */

        int ls = font_ui_medium->GetLineSkip();
        int spacing = glm::max(ls, iconsize + iconsize / 4);

        numVisible = (int)( ((maxSize.y - 2 * innerpad) / spacing) - (!query.isEmpty() ? 1.5f : 0.0f) );
        viewOffset = glm::max(glm::min(selection + 3, (int)items.getLength()) - numVisible, 0);

        if ( numVisible <= 0 )
            Sys::RaiseException( EX_INTERNAL_STATE, "FileChooserOverlay::Draw", "numVisible out of range 1 .. ? (%i)", numVisible );

        if ( (unsigned)numVisible < items.getLength() )
        {
            const float scrollBarUnit = (maxSize.y - innerpad * 2) / items.getLength();
            static const float scrollBarW = 20.0f;

            const float barY0 = scrollBarUnit * viewOffset;
            const float barY1 = scrollBarUnit * (viewOffset+numVisible);

            R::SetBlendColour( COLOUR_WHITEA(0.2f) );
            R::DrawRectangle(glm::vec2(pos.x + maxSize.x - innerpad - scrollBarW, pos.y + innerpad), glm::vec2(pos.x + maxSize.x - innerpad, pos.y + maxSize.y - innerpad), 0.0f);

            R::SetBlendColour( COLOUR_WHITEA(0.8f) );
            R::DrawRectangle(glm::vec2(pos.x + maxSize.x - innerpad - scrollBarW, pos.y + innerpad + barY0), glm::vec2(pos.x + maxSize.x - innerpad, pos.y + innerpad + barY1), 0.0f);
        }

        glm::vec3 drawPos = glm::vec3(pos.x + innerpad, pos.y + innerpad + spacing * 0.5f, 0.0f);

        if ( !query.isEmpty() )
        {
            font_ui_medium->DrawString( query, glm::vec3(pos.x + maxSize.x / 2, drawPos.y, drawPos.z), glm::vec4(1.0f, 1.0f, 1.0f, alpha), TEXT_CENTER | TEXT_MIDDLE );
            drawPos.y += 1.5f * spacing;
        }

        // RUN 1 (text texture)

        for (intptr_t i = viewOffset; i < (intptr_t)items.getLength() && i < viewOffset + numVisible; i++)
        {
            const float ialpha = alpha * ((i == selection) ? 0.8f : 0.4f);

            font_ui_medium->DrawString( items[i].name, drawPos + glm::vec3(iconsize + 8, 0.0f, 0.0f), glm::vec4(1.0f, 1.0f, 1.0f, ialpha), TEXT_LEFT | TEXT_MIDDLE );
            
            drawPos.y += spacing;
        }

        // RUN 2 (icon texture)

        // TODO: clean this up too
        drawPos = glm::vec3(pos.x + innerpad, pos.y + innerpad + spacing * 0.5f, 0.0f);

        if ( !query.isEmpty() )
            drawPos.y += 1.5f * spacing;

        for (intptr_t i = viewOffset; i < (intptr_t)items.getLength() && i < viewOffset + numVisible; i++)
        {
            if ( items[i].iconIndex >= 0 )
            {
                const float ialpha = alpha * ((i == selection) ? 1.0f : 0.75f);

                R::SetBlendColour( COLOUR_WHITEA(ialpha) );
                DrawFileIcon32((int)drawPos.x, (int)drawPos.y - 16, items[i].iconIndex, 0);
            }
            
            drawPos.y += spacing;
        }

        if ( submenu != nullptr )
            submenu->Draw(pos + padding, maxSize);
    }

    void FileChooserOverlay::Exit()
    {
        if ( submenu != nullptr )
        {
            submenu->Exit();
            li::destroy( submenu );
        }
    }

    const char* FileChooserOverlay::GetSelectionName()
    {
        if ( selection < 0 )
            return nullptr;
        else if ( selection == 0 && items[selection].metadata.type == FileMeta_t::F_UNKNOWN )
            return saveAsName;
        else
            return items[selection].name;
    }

    void FileChooserOverlay::Init()
    {
        if (purpose == P_SAVE_AS)
        {
            FileChooserItem item = { Sys::Localize("Create new file..."), {FileMeta_t::F_UNKNOWN, 0}, 3 };
            items.add(item);
        }
    }

    bool FileChooserOverlay::OnDirEntry(const char* name, const FileMeta_t* meta)
    {
        if (*name == '.')
            return true;

        FileChooserItem item = { name, *meta, -1 };

        if (meta->type == FileMeta_t::F_FILE)
        {
            if (!extFilter.isEmpty())
            {
                if (!item.name.endsWith(extFilter))
                    return true;
                else
                    item.name = item.name.dropRightPart(extFilter.getNumChars());
            }

            item.iconIndex = 2;
        }
        else
        {
            item.iconIndex = 1;
        }

        items.add(item);
        return true;
    }

    void FileChooserOverlay::OnFrame( double delta )
    {
        Event_t* event;

        if ( state == FINISHED )
            return;

        if ( submenu != nullptr )
        {
            submenu->OnFrame(delta);

            if ( submenu->IsFinished() )
            {
                if (submenu->GetSelectionID() == 1)
                    FadeOut();

                submenu->Exit();
                li::destroy( submenu );
            }

            return;
        }

        while ( ( event = Event::Pop() ) != nullptr )
        {
            switch ( event->type )
            {
                /* TODO: search for an entry beginning with 'event->uni_cp'

                case EV_UNICODE_IN:
                    if ( selection>=0 )
                    {
                        if ( items[selection].t == MenuItem::textInput )
                        {
                            int uc = event->uni_cp;

                            if (uc == UNI_BACKSP)
                                items[selection].tvalue->set(items[selection].tvalue->dropRightPart(1));
                            else if (uc >= 32)
                                items[selection].tvalue->append(Unicode::Character(uc));
                        }
                    }
                    break;
                */

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
                        if ( selection>=0 && IsAnyUp(event->vkey) && event->vkey.triggered() )
                        {
                            if (--selection < 0)
                                selection = items.getLength()-1;
                        }
                        else if ( selection>=0 && IsAnyDown(event->vkey) && event->vkey.triggered() )
                        {
                            if (++selection >= (int) items.getLength())
                                selection = 0;
                        }
                        else if ( selection>=0 && IsConfirm(event->vkey) && event->vkey.triggered() )
                        {
                            if ( items[selection].metadata.type == FileMeta_t::F_FILE )
                            {
                                if ( flags & CONFIRM_OVERWRITE )
                                    submenu = FileChooserOverlay::CreateOverwriteQuery( (String) Sys::Localize("Overwrite") + " '" + items[selection].name + "'?" );
                                else
                                    FadeOut();
                            }
                            else if ( items[selection].metadata.type == FileMeta_t::F_UNKNOWN )
                            {
                                submenu = FileChooserOverlay::CreateFilenameEditor( Sys::Localize("File Name:"), &saveAsName );
                            }
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
        
        UIOverlay_OnFrame(delta);
    }
}
