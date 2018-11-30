#ifndef framework_system_hpp
#define framework_system_hpp

namespace zfw {
    /**
     * A generic component system that integrates into the framework using its tools (main loop, message broadcast...)
     *
     * Available ownership models:
     *  - owning - IEngine holds a unique_ptr to the system
     */
    class ISystem {
    public:
        virtual ~ISystem() {}

        virtual void OnFrame(float dt) {}
        virtual void OnTicks(int ticks) {}
    };
}

#endif
