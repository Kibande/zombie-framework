
#include <framework/resourcemanager2.hpp>
#include <framework/system.hpp>

#include <littl/List.hpp>

#include <unordered_map>

namespace zfw
{
    using namespace li;

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    struct Hasher
    {
        // musn't be static (is used as a template argument), but we don't want to pollute the zfw namespace
        size_t operator()(const std::pair<TypeID, String>& value) const
        {
            return value.first.hash_code() ^ value.second.getHash();
        }
    };

    class ResourceManager2 : public IResourceManager2
    {
        public:
            ResourceManager2(ISystem* sys);
            virtual ~ResourceManager2();

            virtual IResource2::State_t GetTargetState() final override { return targetState; }
            virtual void SetTargetState(IResource2::State_t state) final override { targetState = state; }

            // see GetResourceFlag_t for possible flags
            virtual IResource2* GetResource(const char* recipe, const TypeID& resourceClass, int flags) final override;

            virtual bool RegisterResourceProvider(const TypeID* resourceClasses, size_t numResourceClasses,
                    IResourceProvider2* provider) final override;

            virtual void UnregisterResourceProvider(IResourceProvider2* provider) final override;

            virtual void GetCurrentResourceSection(ResourceSection_t** sect_out) final override { *sect_out = currentSection; }
            virtual void EnterResourceSection(ResourceSection_t* sect) final override;
            virtual void LeaveResourceSection() final override;
            virtual void ClearResourceSection(ResourceSection_t* sect) final override;

            virtual bool MakeAllResourcesState(IResource2::State_t state, bool propagateError) final override;
            virtual bool MakeResourcesInSectionState(ResourceSection_t* sect, IResource2::State_t state, bool propagateError) final override;

        private:
            struct ResourceSectionStorage_t
            {
                ResourceSection_t* key;

                std::unordered_map<
                        std::pair<TypeID, String>,
                        IResource2*,
                        Hasher
                        > resources;

                List<IResource2*> privateResources;
            };

            void p_ClearStorage(size_t storage);
            IResource2* p_CreateResource(const char* recipe, const TypeID& resourceClass, int flags);
            size_t p_GetStorageForSection(ResourceSection_t* key);
            bool p_MakeResourceCorrectState(IResource2* res);
            bool p_MakeResourcesInStorageState(size_t storage, IResource2::State_t state, bool propagateError);

            ISystem* sys;

            IResource2::State_t targetState;

            ResourceSection_t* currentSection;

            List<ResourceSectionStorage_t, size_t, Allocator<ResourceSectionStorage_t>, ArrayOptions::noBoundsChecking> storages;
            std::unordered_map<TypeID, IResourceProvider2*> providers;
    };

    // ====================================================================== //
    //  class ResourceManager2
    // ====================================================================== //

    IResourceManager2* p_CreateResourceManager2(ISystem* sys)
    {
        return new ResourceManager2(sys);
    }

    ResourceManager2::ResourceManager2(ISystem* sys)
            : sys(sys)
    {
        targetState = IResource2::CREATED;

        storages.addEmpty().key = nullptr;

        currentSection = nullptr;
    }

    ResourceManager2::~ResourceManager2()
    {
        for (size_t i = 0; i < storages.getLength(); i++)
            p_ClearStorage(i);
    }

    void ResourceManager2::ClearResourceSection(ResourceSection_t* sect)
    {
        p_ClearStorage(p_GetStorageForSection(sect));
    }

    void ResourceManager2::EnterResourceSection(ResourceSection_t* sect)
    {
        currentSection = sect;
    }

    IResource2* ResourceManager2::GetResource(const char* recipe, const TypeID& resourceClass, int flags)
    {
        const auto key = std::pair<TypeID, String>(resourceClass, recipe);

        auto storage = p_GetStorageForSection(currentSection);
        auto resourceEntry = storages[storage].resources.find(key);
        
        if (resourceEntry != storages[storage].resources.end())
            return resourceEntry->second;

        zombie_assert((flags & kResourceNeverCreate) == 0);
        
        unique_ptr<IResource2> res(p_CreateResource(recipe, resourceClass, flags));
        
        if (!res/* || !p_MakeResourceCorrectState(res.get())*/)
            return nullptr;

        if (!p_MakeResourceCorrectState(res.get()))
            sys->PrintError(g_essentials->GetErrorBuffer(), kLogError);

        if (!(flags & kResourcePrivate))
            return (storages[storage].resources[key] = res.release());
        else
            return (storages[storage].privateResources.addEmpty() = res.release());
    }

    void ResourceManager2::LeaveResourceSection()
    {
        currentSection = nullptr;
    }

    bool ResourceManager2::MakeAllResourcesState(IResource2::State_t state, bool propagateError)
    {
        zombie_assert(state != IResource2::CREATED);

        for (size_t i = 0; i < storages.getLength(); i++)
            if (!p_MakeResourcesInStorageState(i, state, propagateError))
                return false;

        return true;
    }

    bool ResourceManager2::MakeResourcesInSectionState(ResourceSection_t* sect, IResource2::State_t state, bool propagateError)
    {
        bool res = p_MakeResourcesInStorageState(p_GetStorageForSection(sect), state, propagateError);

        return res;
    }

    void ResourceManager2::p_ClearStorage(size_t storage)
    {
        for (auto& resource : storages[storage].resources)
        {
            delete resource.second;
            resource.second = nullptr;
        }

        for (auto res : storages[storage].privateResources)
            delete res;
        
        storages[storage].resources.clear();
        storages[storage].privateResources.clear();
    }

    IResource2* ResourceManager2::p_CreateResource(const char* recipe, const TypeID& resourceClass, int flags)
    {
        auto provider = providers.find(resourceClass);
        
        if (provider == providers.end())
        {
            if (flags & kResourceRequired)
                ErrorBuffer::SetError3(EX_OBJECT_UNDEFINED, 3,
                        "desc", "Failed to retrieve a necessary resource.",
                        "resourceClass", resourceClass.name(),
                        "recipe", recipe,
                        "function", li_functionName
                        );

            return nullptr;
        }

        return provider->second->CreateResource(this, resourceClass, recipe, flags);
    }

    size_t ResourceManager2::p_GetStorageForSection(ResourceSection_t* key)
    {
        for (size_t i = 0; i < storages.getLength(); i++)
            if (storages[i].key == key)
                return i;

        auto& new_ = storages.addEmpty();
        new_.key = key;
        return storages.getLength() - 1;
    }

    bool ResourceManager2::p_MakeResourceCorrectState(IResource2* res)
    {
        /*if (res->GetState() != targetState) {
            printf("%-39s ", res->GetRecipe());

            switch (targetState) {
                case IResource2::BOUND:     printf("bindDependencies (single thread)"); break;
                case IResource2::PRELOADED: printf("preload (multi-threaded)"); break;
                case IResource2::REALIZED:  printf("realize (rendering thread)"); break;
                case IResource2::RELEASED:  printf("release"); break;
            }

            printf("\n");
        }*/

        return res->StateTransitionTo(targetState, this);
    }

    bool ResourceManager2::p_MakeResourcesInStorageState(size_t storage, IResource2::State_t state, bool propagateError)
    {
        for (auto& resource : storages[storage].resources)
            if (!resource.second->StateTransitionTo(state, this))
            {
                if (propagateError)
                    return false;
                else
                    sys->PrintError(g_essentials->GetErrorBuffer(), kLogError);
            }

        for (auto res : storages[storage].privateResources)
            if (!res->StateTransitionTo(state, this))
            {
                if (propagateError)
                    return false;
                else
                    sys->PrintError(g_essentials->GetErrorBuffer(), kLogError);
            }

        return true;
    }

    bool ResourceManager2::RegisterResourceProvider(const TypeID* resourceClasses, size_t numResourceClasses,
            IResourceProvider2* provider)
    {
        for (size_t i = 0; i < numResourceClasses; i++)
        {
            if (providers.count(resourceClasses[i]) > 0)
                return ErrorBuffer::SetError3(EX_INVALID_ARGUMENT, 2,
                        "desc", (const char*) sprintf_t<63>("Resource class provider collision (resource class 0x%X)", resourceClasses[i]),
                        "function", li_functionName
                        ), false;

            providers[resourceClasses[i]] = provider;
        }

        return true;
    }

    void ResourceManager2::UnregisterResourceProvider(IResourceProvider2* provider)
    {
        for (auto it = providers.begin(); it != providers.end();)
        {
            if (it->second == provider)
                it = providers.erase(it);
            else
                it++;
        }
    }
}
