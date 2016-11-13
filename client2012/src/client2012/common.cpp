
#include "client.hpp"

namespace client
{
    // Managed by TitleScene
    Session*        g_session       = nullptr;
    World*          g_world         = nullptr;
    WorldCollisionHandler* g_collHandler = nullptr;

    // Managed here
    zr::Font*       g_headsUpFont   = nullptr;
    
    void ReloadSharedMedia()
    {
        if(g_headsUpFont == nullptr)
            g_headsUpFont = zr::Font::Open("fontcache/client_11pt");
    }
    
    void ReleaseSharedMedia()
    {
        li::destroy(g_headsUpFont);
    }
}