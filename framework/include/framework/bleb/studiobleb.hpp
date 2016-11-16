#pragma once

#include <framework/filesystem.hpp>

#include <littl/String.hpp>

#include <vector>

namespace zfw
{
    class StudioBlebManager
    {
        public:
            bool Init(ISystem* sys);

            void MountAllInDirectory(const char* nativePath);
            bool MountBleb(const char* nativePath);
            bool Refresh();

        private:
            struct MountedBleb_t
            {
                li::String nativePath, mountPoint, sourceFile, recipe, tool;
                std::shared_ptr<zfw::IFileSystem> fs;
                time_t timestamp;
            };

            bool RefreshResource(MountedBleb_t& res);

            ISystem* sys;

            std::vector<MountedBleb_t> mountedBlebs;
    };
}
