#ifndef framework_component_hpp
#define framework_component_hpp

namespace zfw {
    /**
     * A generic component that integrates into the framework using its tools (main loop, message broadcast...)
     *
     * Available ownership models:
     *  - owning - IEngine holds a unique_ptr to the component
     */
    class IComponent {
    public:
        virtual ~IComponent() {}

        virtual void OnFrame() {}
    };
}

#endif
