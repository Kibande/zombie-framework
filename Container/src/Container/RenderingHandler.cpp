#include <Container/RenderingHandler.hpp>

#include <framework/engine.hpp>
#include <framework/errorcheck.hpp>
#include <framework/pixmap.hpp>
#include <framework/utility/essentials.hpp>

#include <RenderingKit/utility/RKVideoHandler.hpp>

namespace Container {
    using namespace RenderingKit;

    RenderingHandler::RenderingHandler(zfw::IEngine* sys, std::shared_ptr<zfw::MessageQueue> msgQueue) : sys(sys), msgQueue(msgQueue) {
    }

    bool RenderingHandler::RenderingInit() {
        auto imh = sys->GetModuleHandler(true);

        this->rk.reset(RenderingKit::TryCreateRenderingKit(imh));
        zombie_ErrorCheck(rk);

        if (!rk->Init(sys, sys->GetEssentials()->GetErrorBuffer(), nullptr))
            return false;

        auto wm = rk->GetWindowManager();

        if (!wm->LoadDefaultSettings(nullptr) || !wm->ResetVideoOutput())
            return false;

        auto rm = rk->GetRenderingManager();
        sys->SetVideoHandler(std::make_unique<RKVideoHandler>(rm, wm, msgQueue));

        return true;
    }
}
