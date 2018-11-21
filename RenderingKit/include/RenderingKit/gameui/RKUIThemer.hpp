#pragma once

#include <gameui/uithemer.hpp>

#include <framework/modulehandler.hpp>

#include <reflection/magic.hpp>

#include "../RenderingKit.hpp"

namespace RenderingKit
{
    class IRKUIThemer : public gameui::UIThemer
    {
        public:
            virtual ~IRKUIThemer() {}

#if ZOMBIE_API_VERSION >= 201901
            virtual bool Init(zfw::ISystem* sys, IRenderingManager* rm, zfw::IResourceManager2* res) = 0;
#elif ZOMBIE_API_VERSION >= 201701
            virtual bool Init(zfw::ISystem* sys, IRenderingKit* rk, zfw::IResourceManager2* res) = 0;
#else
            virtual void Init(zfw::ISystem* sys, IRenderingKit* rk, zfw::IResourceManager* resRef) = 0;
#endif

            virtual IFontFace* GetFont(intptr_t fontIndex) = 0;

            REFL_CLASS_NAME("IRKUIThemer", 1)
    };

#ifdef RENDERING_KIT_STATIC
    IRKUIThemer* CreateRKUIThemer();

    ZOMBIE_IFACTORYLOCAL(RKUIThemer)
#else
    ZOMBIE_IFACTORY(RKUIThemer, "RenderingKit")
#endif
}
