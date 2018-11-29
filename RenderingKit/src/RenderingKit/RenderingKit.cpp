
#include "RenderingKitImpl.hpp"

#include <framework/shader_preprocessor.hpp>
#include <framework/engine.hpp>

namespace RenderingKit
{
    using namespace zfw;

    IRenderingKit* CreateRenderingKit()
    {
        return new RenderingKit();
    }

    RenderingKit::RenderingKit()
    {
        sys = nullptr;
        eb = nullptr;
    }

    zfw::ShaderPreprocessor* RenderingKit::GetShaderPreprocessor()
    {
        if (shaderPreprocessor == nullptr)
            shaderPreprocessor.reset(sys->CreateShaderPreprocessor());

        return shaderPreprocessor.get();
    }

    bool RenderingKit::Init(IEngine* sys, ErrorBuffer_t* eb, IRenderingKitHost* host)
    {
        SetEssentials(sys->GetEssentials());

        this->sys = sys;
        this->eb = eb;

        wm.reset(CreateSDLWindowManager(eb, this));

        if (!wm->Init())
            return false;

        return true;
    }

    IRenderingManager* RenderingKit::StartupRendering(CoordinateSystem coordSystem)
    {
        rm = IRenderingManagerBackend::Create(eb, this, coordSystem);

        if (!rm->Startup()) {
            return nullptr;
        }

        return rm.get();
    }
}
