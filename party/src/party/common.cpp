
#include "party.hpp"

namespace party
{
    IFont *font_ui_medium = nullptr, *font_ui_big = nullptr;
    ITexture* tex_buttons = nullptr, *tex_fileicons = nullptr, *tex_uiwidgets = nullptr;

    void SharedReleaseMedia()
    {
        li::destroy( font_ui_medium );
        li::destroy( font_ui_big );
        li::release( tex_buttons );
        li::release( tex_fileicons );
        li::release( tex_uiwidgets );
    }

    void SharedReloadMedia()
    {
        if ( font_ui_medium == nullptr )
            font_ui_medium = Loader::OpenFont( "media/font/DejaVuSans.ttf", 24, FONT_BOLD, 0, 255 );

        if ( font_ui_big == nullptr )
            font_ui_big = Loader::OpenFont( "media/font/DejaVuSans.ttf", 36, FONT_BOLD, 0, 255 );

        if ( tex_buttons == nullptr )
            tex_buttons = Loader::LoadTexture( "media/buttons.png", true );

        if ( tex_fileicons == nullptr )
            tex_fileicons = Loader::LoadTexture( "media/fileicons.png", true );

        if ( tex_uiwidgets == nullptr )
            tex_uiwidgets = Loader::LoadTexture( "media/uiwidgets.png", true );
    }
}