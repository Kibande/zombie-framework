#pragma once

#include <framework/base.hpp>

#include <framework/utility/errorbuffer.hpp>
#include <framework/utility/params.hpp>

#include <typeindex>

namespace zfw
{
    enum
    {
        PROVIDER_CHECK_PARAMS_ALIASING = 1
    };

    enum
    {
        RESOURCE_REQUIRED = 1,      // set error if not found & can't create
        RESOURCE_NEVER_CREATE = 2
    };

    class IResourceProvider
    {
        public:
            virtual ~IResourceProvider() {}

            virtual shared_ptr<IResource> CreateResource(IResourceManager* res, const TypeID& resourceClass,
                    const char* normparams, int flags) = 0;

            virtual bool        DoParamsAlias(const TypeID& resourceClass, const char* params1,
                    const char* params2) = 0;

            virtual const char* TryGetResourceClassName(const TypeID& resourceClass) = 0;
    };

    class IResourceManager
    {
        public:
            virtual ~IResourceManager() {}

            virtual ErrorBuffer_t* GetErrorBuffer() = 0;

            virtual bool RegisterResourceProvider(const TypeID* resourceClasses, size_t numResourceClasses,
                    IResourceProvider* provider, int providerFlags) = 0;

            virtual shared_ptr<IResource> GetResource(const TypeID& resourceClass, const char* normparams,
                    int flags, int providerFlags) = 0;
            virtual void RegisterResource(const TypeID* resourceClasses, size_t numResourceClasses,
                    const char* normparams, int flags, shared_ptr<IResource> resource) = 0;

            virtual void DumpStatistics() = 0;

            template <class Interface>
            shared_ptr<Interface> GetResource(const char* normparams, int flags, int providerFlags)
            {
                shared_ptr<IResource> rsrc = GetResource(typeID<Interface>(), normparams, flags, providerFlags);

                return (rsrc != nullptr) ? std::static_pointer_cast<Interface>(move(rsrc)) : nullptr;
            }

            template <class Interface>
            shared_ptr<Interface> GetResourceByPath(const char* normpath, int flags, int providerFlags)
            {
                char params[4096];

                if (!Params::BuildIntoBuffer(params, sizeof(params), 1, "path", normpath))
                    return ErrorBuffer::SetBufferOverflowError(this->GetErrorBuffer(), li_functionName),
                            nullptr;

                return GetResource<Interface>(params, flags, providerFlags);
            }
    };
}
