#pragma once

#include <framework/base.hpp>

namespace RenderingKit {
    class IRenderingKit;
}

namespace Container {
    class IRenderingHandler {
    public:
        virtual ~IRenderingHandler() {}

        virtual bool RenderingInit() = 0;

        // TODO: this is not nice!
        virtual RenderingKit::IRenderingKit* GetRenderingKit() = 0;
    };

    class IContainerApp {
    public:
    };
}
