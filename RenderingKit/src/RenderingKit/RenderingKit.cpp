
#include "RenderingKitImpl.hpp"

#include <framework/shader_preprocessor.hpp>
#include <framework/system.hpp>

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

    bool RenderingKit::Init(ISystem* sys, ErrorBuffer_t* eb, IRenderingKitHost* host)
    {
        SetEssentials(sys->GetEssentials());

        this->sys = sys;
        this->eb = eb;

        wm.reset(CreateSDLWindowManager(eb, this));

        if (!wm->Init())
            return false;

        rm.reset(CreateRenderingManager(eb, this));

        return true;
    }

    IRenderingManager* RenderingKit::StartupRendering(gsl::span<const char*> vertexAttribNames)
    {
        if (!rm->Startup(vertexAttribNames)) {
            return nullptr;
        }

        return rm.get();
    }
}
