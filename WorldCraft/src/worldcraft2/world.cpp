
#include "world.hpp"
#include "worldcraftpro.hpp"

namespace worldcraftpro
{
    void World::Init(shared_ptr<IResourceManager> brushTexMgr, const char* name)
    {
        this->name = name;
        
        InitOffline();

        // set default cameras
        viewports.addEmpty().type = kViewportLeft;
        viewports.addEmpty().type = kViewportPerspective;
        viewports.addEmpty().type = kViewportTop;
        viewports.addEmpty().type = kViewportFront;
        
        for (size_t i = 0; i < viewports.getLength(); i++)
        {
            viewports[i].fov = 45.0f;
            /*viewports[i].culling = true;
            viewports[i].wireframe = false;
            viewports[i].shaded = true;*/

            viewports[i].camera = g.rm->CreateCamera(sprintf_15("viewport%i", i));
            viewports[i].camera->SetClippingDist(0.2f, 150.0f);

            if (viewports[i].type == kViewportPerspective)
                viewports[i].camera->SetPerspective();
            else
                viewports[i].camera->SetOrthoFakeFOV();
            
            viewports[i].camera->SetVFov(viewports[i].fov * f_pi / 180.0f);

            switch (viewports[i].type)
            {
                case kViewportPerspective:  viewports[i].camera->SetView2( Float3(0.0f, 0.0f, 1.0f), 30.0f, 270.0f / 180.0f * f_pi, 30.0f / 180.0f * f_pi ); break;
                case kViewportTop:          viewports[i].camera->SetView( Float3( 0.0f, 0.0f, 10.0f ), Float3(), Float3( 0.0f, -1.0f, 0.0f ) ); break;
                case kViewportFront:        viewports[i].camera->SetView( Float3( 0.0f, 10.0f, 0.0f ), Float3(), Float3( 0.0f, 0.0f, 1.0f ) ); break;
                case kViewportLeft:         viewports[i].camera->SetView( Float3( -10.0f, 0.0f, 0.0f ), Float3(), Float3( 0.0f, 0.0f, 1.0f ) ); break;
            }
        }

        this->brushTexMgr = brushTexMgr;
    }

    void World::InitOffline()
    {
        worldNode.reset(new WorldNode(this));
        selection = nullptr;
    }

    shared_ptr<ITexture> World::GetBrushTexture(const char* path)
    {
        return brushTexMgr->GetResourceByPath<ITexture>(path, 0, 0);
    }
}
