#pragma once

#include <framework/base.hpp>

namespace zfw
{
namespace studio
{
    class RecentFiles
    {
        public:
            static RecentFiles* Create(ISystem* sys);
            virtual ~RecentFiles() {}

            virtual bool Load(const char* filename) = 0;
            virtual bool Save(const char* filename) = 0;

            virtual void BumpItem(const char* item) = 0;
            virtual bool IsDirty() = 0;
            virtual size_t GetNumItems() = 0;
            virtual const char* GetItemAt(size_t index) = 0;
            virtual void SetMaxNumItems(size_t maxNumItems) = 0;
    };
}
}
