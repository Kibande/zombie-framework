#ifndef ntile_systems_motionsystem_hpp
#define ntile_systems_motionsystem_hpp

#include "../nbase.hpp"

#include <framework/system.hpp>

namespace ntile
{
    class IMotionSystem : public ISystem {
    public:
        static unique_ptr<IMotionSystem> Create(IBroadcastHandler* broadcastHandler);

        virtual void Attach(IEntityWorld2* world) = 0;
    };
}

#endif
