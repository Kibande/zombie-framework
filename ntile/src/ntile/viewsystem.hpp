#ifndef ntile_viewsystem_hpp
#define ntile_viewsystem_hpp

#include <framework/base.hpp>
#include <framework/component.hpp>

namespace ntile {
    struct World;

    /**
     * View system responsibilities:
     *  - control renderer state
     *  - display graphics
     *  - push input events into engine queue
     */
    class IViewSystem : public zfw::IComponent {
    public:
        static IViewSystem* Create(World& world);

        /**
         * Start up rendering & event input. Register VideoHandler etc.
         *
         * @param sys
         * @param eb
         * @param eventQueue
         * @return
         */
        virtual bool Startup(zfw::ISystem* sys, zfw::ErrorBuffer_t* eb, zfw::MessageQueue* eventQueue) = 0;
    };
}

#endif
