
#include <framework/errorbuffer.hpp>
#include <framework/resourcemanager.hpp>
#include <framework/engine.hpp>

#include <littl/HashMap.hpp>
#include <littl/String.hpp>

#include <utility>

#include "private.hpp"

namespace zfw
{
    using namespace li;

    namespace
    {
        // musn't be static (is used as a template argument), but we don't want to pollute the zfw namespace
        size_t getHash(const TypeID& t)
        {
            return t.hash_code();
        }
    }

    class ResourceManager : public IResourceManager
    {
        public:
            ResourceManager(ErrorBuffer_t* eb, IEngine* system, const char* name);
            ~ResourceManager();

            virtual ErrorBuffer_t* GetErrorBuffer() { return eb; }

            virtual bool RegisterResourceProvider(const TypeID* resourceClasses, size_t numResourceClasses,
                    IResourceProvider* provider, int providerFlags) override;

            virtual shared_ptr<IResource> GetResource(const TypeID& resourceClass, const char* normparams,
                    int flags, int providerFlags) override;
            virtual void RegisterResource(const TypeID* resourceClasses, size_t numResourceClasses,
                    const char* normparams, int flags, shared_ptr<IResource> resource) override;

            virtual void DumpStatistics() override;

        protected:
            struct ResourceClassBucket_t
            {
                TypeID resourceClass;
                int providerFlags;
                IResourceProvider* provider;

                HashMap<String, shared_ptr<IResource>> resourcesByNormparams;

                ResourceClassBucket_t(TypeID resourceClass, int providerFlags,
                        IResourceProvider* provider = nullptr)
                        : resourceClass(resourceClass), providerFlags(providerFlags), provider(provider)
                {
                }

                ResourceClassBucket_t(ResourceClassBucket_t&& other)
                        : resourceClass(move(other.resourceClass)),
                        resourcesByNormparams(move(other.resourcesByNormparams))
                {
                    providerFlags = other.providerFlags;
                    provider = other.provider;
                }

                ResourceClassBucket_t& operator =(ResourceClassBucket_t&& other)
                {
                    resourceClass = move(other.resourceClass);
                    providerFlags = other.providerFlags;
                    provider = other.provider;
                    resourcesByNormparams = move(other.resourcesByNormparams);

                    return *this;
                }

            private:
                ResourceClassBucket_t(const ResourceClassBucket_t&);
                ResourceClassBucket_t& operator =(const ResourceClassBucket_t&);
            };

            ErrorBuffer_t* eb;
            IEngine* sys;
            String name;

            HashMap<TypeID, ResourceClassBucket_t, size_t, &getHash> buckets;
    };

    IResourceManager* p_CreateResourceManager(ErrorBuffer_t* eb, IEngine* sys, const char* name)
    {
        return new ResourceManager(eb, sys, name);
    }

    ResourceManager::ResourceManager(ErrorBuffer_t* eb, IEngine* sys, const char* name)
    {
        this->eb = eb;
        this->sys = sys;
        this->name = name;
    }

    ResourceManager::~ResourceManager()
    {
        //DumpStatistics();
        //printf("Killing %s.\n", name.c_str());
    }

    void ResourceManager::DumpStatistics()
    {
        sys->Printf(kLogNever, "Dumping statistics for zfw::ResourceManager");
        sys->Printf(kLogNever, "resourceClass hashmap:");
        buckets.printStatistics();

        iterate2 (i, buckets)
        {
            ResourceClassBucket_t& bucket = (*i).value;
            sys->Printf(kLogNever, "resourceClass %s:", bucket.provider->TryGetResourceClassName(bucket.resourceClass));

            //bucket.resourcesByNormparams.printStatistics();

            /*iterate2 (j, bucket.resourcesByNormparams)
            {
                Sys::Printf(kLogNever, "    `%s`", (*j).value->GetName());
            }*/
        }
    }

    shared_ptr<IResource> ResourceManager::GetResource(const TypeID& resourceClass, const char* normparams,
            int flags, int providerFlags)
    {
        ResourceClassBucket_t* bucket = buckets.find(resourceClass);

        if (bucket != nullptr)
        {
            // Try to find the resource in the bucket
            // FIXME: Handle PROVIDER_CHECK_PARAMS_ALIASING
            shared_ptr<IResource> resource = bucket->resourcesByNormparams.get(String(normparams));

            // If not found, create it if possible
            if (resource == nullptr
                    && !(flags & RESOURCE_NEVER_CREATE)
                    && bucket->provider != nullptr)
            {
                resource = bucket->provider->CreateResource(this, resourceClass, normparams, providerFlags);

                if (resource == nullptr)
                    return nullptr;

                bucket = nullptr;       // invalid now (think composite resources making our HashMap reallocate)

                RegisterResource(&resourceClass, 1, normparams, flags, resource);

                // FIXME: We should log CreateResource errors if we know the caller won't care
            }

            return resource;
        }

        if (flags & RESOURCE_REQUIRED)
            ErrorBuffer::SetError(eb, EX_OBJECT_UNDEFINED,
                    "desc", "Failed to retrieve a necessary resource.",
                    "resourceClass", resourceClass.name(),
                    "normparams", normparams,
                    "function", li_functionName,
                    nullptr);

        return nullptr;
    }

    void ResourceManager::RegisterResource(const TypeID* resourceClasses, size_t numResourceClasses,
            const char* normparams, int flags, shared_ptr<IResource> resource)
    {
        for (size_t i = 0; i < numResourceClasses; i++)
        {
            ResourceClassBucket_t* bucket = buckets.find(resourceClasses[i]);

            if (bucket == nullptr)
            {
                auto resCls = resourceClasses[i];

                ResourceClassBucket_t newBucket(resourceClasses[i], 0);
                bucket = &buckets.set(move(resCls), move(newBucket));
            }

            bucket->resourcesByNormparams.set(String(normparams), move(resource));
        }
    }

    bool ResourceManager::RegisterResourceProvider(const TypeID* resourceClasses, size_t numResourceClasses,
            IResourceProvider* provider, int providerFlags)
    {
        for (size_t i = 0; i < numResourceClasses; i++)
        {
            ResourceClassBucket_t* bucket = buckets.find(resourceClasses[i]);

            if (bucket != nullptr)
            {
                if (bucket->provider != nullptr)
                    return ErrorBuffer::SetError(eb, EX_INVALID_ARGUMENT,
                            "desc", (const char*) sprintf_t<63>("Resource class provider collision (resource class 0x%X)", resourceClasses[i]),
                            "function", li_functionName,
                            nullptr),
                            false;

                bucket->providerFlags = providerFlags;
                bucket->provider = provider;
            }
            else
            {
                auto resCls = resourceClasses[i];

                ResourceClassBucket_t newBucket(resourceClasses[i], providerFlags, provider);
                buckets.set(move(resCls), move(newBucket));
            }
        }

        return true;
    }
}
