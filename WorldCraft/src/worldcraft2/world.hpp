#pragma once

#include "node.hpp"

namespace worldcraftpro
{
    using namespace RenderingKit;
    using namespace zfw;

    enum ViewportType_t
    {
        kViewportPerspective,
        kViewportTop,
        kViewportFront,
        kViewportLeft,
    };

    struct Viewport
    {
        ViewportType_t type;
        float fov;
        shared_ptr<ICamera> camera;
        
        //bool culling, wireframe, shaded;
    };

    class World
    {
        public:
            String name;
            
            // default viewports: persp, top, front, left
            List<Viewport> viewports;
            
            unique_ptr<WorldNode> worldNode;
            Node *selection;

            shared_ptr<IResourceManager> brushTexMgr;

        public:
            void Init(shared_ptr<IResourceManager> brushTexMgrRef, const char *name);
            void InitOffline();
        
            void ClearSelection() { selection = nullptr; }
            void SetSelection(Node *n) { selection = n; }

            shared_ptr<ITexture> GetBrushTexture(const char* path);
    };
}
