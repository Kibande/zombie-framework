
#include "resourceprovider.hpp"

#include "ntile.hpp"
#include "entities/entities.hpp"

namespace ntile
{
    IResource2* ResourceProvider::CreateResource(IResourceManager2* res, const std::type_index& resourceClass,
            const char* recipe, int flags)
    {
        {
            zombie_assert(resourceClass != resourceClass);
            return nullptr;
        }
    }

    void ResourceProvider::RegisterResourceProviders(IResourceManager2* res)
    {
        static const std::type_index resourceClasses[] = {
        };

        //res->RegisterResourceProvider(resourceClasses, li_lengthof(resourceClasses), this);
    }
}
