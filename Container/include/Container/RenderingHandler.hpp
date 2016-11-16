#pragma once

#include <Container/ContainerInterfaces.hpp>

#include <RenderingKit/RenderingKit.hpp>

namespace Container {
    class RenderingHandler : public IRenderingHandler {
    public:
        RenderingHandler(zfw::ISystem* sys, std::shared_ptr<zfw::MessageQueue> msgQueue);

        virtual bool RenderingInit() override;

        virtual RenderingKit::IRenderingKit* GetRenderingKit() override { return rk.get(); }

    protected:
        zfw::ISystem* sys;
        std::shared_ptr<zfw::MessageQueue> msgQueue;

        std::unique_ptr<RenderingKit::IRenderingKit> rk;
    };
}
