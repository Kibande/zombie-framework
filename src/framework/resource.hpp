#pragma once

#include <framework/base.hpp>

namespace zfw
{
    class IResource
    {
        public:
            virtual ~IResource() {}

            virtual const char* GetName() = 0;
    };

    class IResource2
    {
        public:
            enum State_t { CREATED, BOUND, PRELOADED, REALIZED, RELEASED };

            virtual ~IResource2() {}

            virtual void* Cast(const TypeID& resourceClass) = 0;
            //virtual const char* GetRecipe() const = 0;
            virtual State_t GetState() const = 0;
            virtual bool StateTransitionTo(State_t targetState, IResourceManager2* resMgr) = 0;

        protected:
            template <class C>
            void* DefaultCast(C* res, const TypeID& resourceClass)
            {
                if (resourceClass == typeID<C>())
                    return res;
                else if (resourceClass == typeID<IResource2>())
                    return static_cast<IResource2*>(res);
                else
                    return nullptr;
            }

            template <class C>
            bool DefaultStateTransitionTo(C* res, State_t targetState, IResourceManager2* resMgr)
            {
                if (res->GetState() == targetState)
                    return true;

                if (targetState == BOUND) {
                    if (!res->BindDependencies(resMgr))
                        return false;

                    res->Unload();
                    res->Unrealize();
                }
                else if (targetState == PRELOADED) {
                    if (!res->BindDependencies(resMgr) || !res->Preload(resMgr))
                        return false;

                    res->Unrealize();
                }
                else if (targetState == REALIZED) {
                    if (!res->BindDependencies(resMgr) || !res->Preload(resMgr) || !res->Realize(resMgr))
                        return false;
                }
                else if (targetState == RELEASED) {
                    res->Unload();
                    res->Unrealize();
                }

                res->state = targetState;
                return true;
            }
    };
}
