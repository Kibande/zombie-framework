#ifndef ntile_viewsystem_hpp
#define ntile_viewsystem_hpp

#include <framework/base.hpp>
#include <framework/system.hpp>

namespace ntile {
    struct World;

    /**
     * View system responsibilities:
     *  - control renderer state
     *  - display graphics
     *  - push input events into engine queue
     */
    class IViewSystem : public zfw::ISystem {
    public:
        static IViewSystem* Create(zfw::IResourceManager2& resourceManager, World& world);

        /**
         * Start up rendering & event input. Register VideoHandler etc.
         *
         * @param sys
         * @param eb
         * @param eventQueue
         * @return
         */
        virtual bool Startup(zfw::IEngine* sys, zfw::ErrorBuffer_t* eb, zfw::MessageQueue* eventQueue) = 0;
    };
}

#endif
