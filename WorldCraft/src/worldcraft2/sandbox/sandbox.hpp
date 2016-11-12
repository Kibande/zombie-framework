#pragma once

#include <framework/base.hpp>
#include <framework/modulehandler.hpp>
#include <framework/scene.hpp>

#include <framework/entity.hpp>

#include <RenderingKit/RenderingKit.hpp>

namespace sandbox
{
    using namespace RenderingKit;
    using namespace zfw;

    struct Globals
    {
        ErrorBuffer_t*      eb;

        shared_ptr<MessageQueue> msgQueue;
        shared_ptr<IResourceManager> res;
        unique_ptr<EntityWorld> world;

        unique_ptr<IRenderingKit> rk;
        IRenderingManager*  rm;
        IWindowManager*     wm;
    };

    extern Globals g;

    extern ISystem*               g_sys;

    class ISandboxScene : public zfw::IScene
    {
    };

    shared_ptr<ISandboxScene> CreateSandboxScene();

    //
    void sandboxMain(int argc, char** argv);
}
