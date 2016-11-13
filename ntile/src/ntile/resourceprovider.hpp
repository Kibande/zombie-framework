#pragma once

#include "nbase.hpp"

#include <framework/resourcemanager2.hpp>

namespace ntile
{
    class ResourceProvider : public IResourceProvider2
    {
        public:
            void RegisterResourceProviders(IResourceManager2* resMgr);

            // zfw::IResourceProvider
            virtual IResource2*     CreateResource(IResourceManager2* res, const std::type_index& resourceClass,
                    const char* recipe, int flags) override;
    };
}