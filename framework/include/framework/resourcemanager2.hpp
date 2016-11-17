#pragma once

#include <framework/base.hpp>
#include <framework/resource.hpp>

#include <framework/utility/errorbuffer.hpp>
#include <framework/utility/params.hpp>

#include <cstddef>

namespace zfw
{
    struct ResourceSection_t
    {
        const char* name;
        int flags;
        size_t numOwnedResources;
    };

    class IResourceManager2
    {
        public:
            enum GetResourceFlag_t
            {
                kResourceRequired = 1,          // set error if not found & can't create
                kResourceNeverCreate = 2,
                kResourcePrivate = 4,
            };

            virtual ~IResourceManager2() {}

            virtual IResource2::State_t GetTargetState() = 0;
            virtual void SetTargetState(IResource2::State_t state) = 0;

            // see GetResourceFlag_t for possible flags
            virtual IResource2* GetResource(const char* recipe, const TypeID& resourceClass, int flags) = 0;

            virtual bool RegisterResourceProvider(const TypeID* resourceClasses, size_t numResourceClasses,
                    IResourceProvider2* provider) = 0;

            virtual void UnregisterResourceProvider(IResourceProvider2* provider) = 0;

            virtual void GetCurrentResourceSection(ResourceSection_t** sect_out) = 0;
            virtual void EnterResourceSection(ResourceSection_t* sect) = 0;
            virtual void LeaveResourceSection() = 0;
            virtual void ClearResourceSection(ResourceSection_t* sect) = 0;

            virtual bool MakeAllResourcesState(IResource2::State_t state, bool propagateError) = 0;
            virtual bool MakeResourcesInSectionState(ResourceSection_t* sect, IResource2::State_t state, bool propagateError) = 0;

            template <class C> C* GetResource(const char* recipe, int flags)
            {
                return static_cast<C*>(GetResource(recipe, typeID<C>(), flags));
            }

            template <class C>
            C* GetResourceByPath(const char* normpath, int flags)
            {
                char params[4096];

                if (!Params::BuildIntoBuffer(params, sizeof(params), 1, "path", normpath))
                    return ErrorBuffer::SetBufferOverflowError(g_essentials->GetErrorBuffer(), li_functionName),
                            nullptr;

                return GetResource<C>(params, flags);
            }

            template <class C> void Resource(C** res_out, const char* recipe, int flags = kResourceRequired)
            {
                *res_out = GetResource<C>(recipe, flags);
                zombie_assert(*res_out);
            }

            template <class C> void ResourceByPath(C** res_out, const char* path, int flags = kResourceRequired)
            {
                *res_out = GetResourceByPath<C>(path, flags);
                zombie_assert(*res_out);
            }

            bool MakeAllResourcesTargetState(bool propagateError)
            {
                return MakeAllResourcesState(GetTargetState(), propagateError);
            }

            bool MakeResourcesInSectionTargetState(ResourceSection_t* sect, bool propagateError)
            {
                return MakeResourcesInSectionState(sect, GetTargetState(), propagateError);
            }
    };

    class IResourceProvider2
    {
        public:
            virtual ~IResourceProvider2() {}

            virtual IResource2* CreateResource(IResourceManager2* res, const TypeID& resourceClass,
                    const char* recipe, int flags) = 0;

            //virtual const char* TryGetResourceClassName(const TypeID& resourceClass) = 0;
    };

	class ResourceManagerScope {
		public:
			ResourceManagerScope(IResourceManager2* resMgr)
					: backup(GetScopedResourceManager(false))
			{
				SetScopedResourceManager(resMgr);
			}

			~ResourceManagerScope()
			{
				SetScopedResourceManager(backup);
			}

			static IResourceManager2* GetScopedResourceManager(bool required);
			static void SetScopedResourceManager(IResourceManager2* resMgr);

		private:
			IResourceManager2* backup;
	};

	template <typename T>
	class Resource
	{
		public:
			T* operator* () { return res; }
            T* operator-> () { return res; }
            operator T* () { return res; }

            bool ByPath(const char* path, int flags = IResourceManager2::kResourceRequired)
			{
				auto resMgr = ResourceManagerScope::GetScopedResourceManager(true);
				res = resMgr->GetResourceByPath<T>(path, flags);
				return (res != nullptr);
			}

            bool ByRecipe(const char* recipe, int flags = IResourceManager2::kResourceRequired)
            {
                auto resMgr = ResourceManagerScope::GetScopedResourceManager(true);
                res = resMgr->GetResource<T>(recipe, flags);
                return (res != nullptr);
            }

		private:
			T* res;
	};
}
