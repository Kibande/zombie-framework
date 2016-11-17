
#include "resourceprovider.hpp"

#include "ntile.hpp"
#include "entities/entities.hpp"

namespace ntile
{
    IResource2* ResourceProvider::CreateResource(IResourceManager2* res, const std::type_index& resourceClass,
            const char* recipe, int flags)
    {
        if (resourceClass == typeid(CharacterModel))
        {
            String path;

            const char *key, *value;

            while (Params::Next(recipe, key, value))
            {
                if (strcmp(key, "path") == 0)
                    path = value;
            }

            zombie_assert(!path.isEmpty());

            return new CharacterModel(std::move(path));
        }
        else
        {
            zombie_assert(resourceClass != resourceClass);
            return nullptr;
        }
    }

    void ResourceProvider::RegisterResourceProviders(IResourceManager2* res)
    {
        static const std::type_index resourceClasses[] = {
            typeid(CharacterModel)
        };

        res->RegisterResourceProvider(resourceClasses, li_lengthof(resourceClasses), this);
    }
}
