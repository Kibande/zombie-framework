#pragma once

#include <framework/base.hpp>

#include <reflection/api.hpp>

#define ZOMBIE_FACTORY(interface_, factory_, moduleName_)\
    static inline interface_* factory_(zfw::IModuleHandler* imh)\
    {\
        return imh->CreateInterface<interface_>(moduleName_);\
    }

#define ZOMBIE_FACTORYLOCAL(interface_, factory_, realfactory_)\
    static inline interface_* factory_(zfw::IModuleHandler* imh)\
    {\
        return realfactory_();\
    }

#define ZOMBIE_IFACTORY(name_, moduleName_)\
    ZOMBIE_FACTORY(I##name_, TryCreate##name_, moduleName_)

#define ZOMBIE_IFACTORYLOCAL(name_)\
    ZOMBIE_FACTORYLOCAL(I##name_, TryCreate##name_, Create##name_)

namespace zfw
{
    class IModuleHandler
    {
        public:
            virtual ~IModuleHandler() {}

            virtual void* CreateInterface(const char* moduleName, const char* interfaceName) = 0;

            template <class C> C* CreateInterface(const char* moduleName)
            {
                return reinterpret_cast<C*>(CreateInterface(moduleName, reflection::versionedNameOfClass<C>()));
            }
    };
}
