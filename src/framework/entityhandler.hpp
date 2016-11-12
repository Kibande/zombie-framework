#pragma once

#include <framework/base.hpp>
#include <framework/errorbuffer.hpp>

namespace zfw
{
    enum
    {
        ENTITY_REQUIRED = 1       // set error if failed to instantiate
    };

    class IEntityClassListener
    {
        public:
            // > 0:     continue
            // <= 0:    return
            virtual int OnEntityClass(const char* className) = 0;
    };

    class IEntityHandler
    {
        public:
            virtual ~IEntityHandler() {}

            virtual bool Register(const char* className, IEntity* (*Instantiate)()) = 0;
            virtual bool RegisterExternal(const char* path, int flags) = 0;

            virtual IEntity* InstantiateEntity(const char* className, int flags) = 0;
            virtual int IterateEntityClasses(IEntityClassListener* listener) = 0;
    };
}
