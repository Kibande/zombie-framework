#pragma once

#include <framework/base.hpp>

#include <ctime>

namespace zfw
{
    enum
    {
        kFSAccessStat           = 1,
        kFSAccessRead           = 2,
        kFSAccessWrite          = 4,
        kFSAccessCreateFile     = 8,
        kFSAccessCreateDir      = 16,
        kFSAccessAll            = 31
    };

    enum
    {
        kDirectoryMayCreate     = 1,
    };

    enum
    {
        kFileMayCreate  = 1,
        kFileTruncate   = 2,
    };

    enum
    {
        FS_REQUIRE_BOTH = 1
    };

    enum
    {
        FS_LEFT_NOT_FOUND =     1,
        FS_RIGHT_NOT_FOUND =    2,
        FS_NEITHER_FOUND =      3
    };

    struct FSStat_t
    {
        uint64_t sizeInBytes;
        time_t creationTime, modificationTime;
        bool isDirectory;
    };

    class IDirectory
    {
        public:
            virtual ~IDirectory() {}

            virtual const char* ReadDir() = 0;
    };

    class IFileSystem
    {
        public:
            virtual ~IFileSystem() {}

            virtual IDirectory* OpenDirectory(const char* normalizedPath, int flags) = 0;
            virtual bool OpenFileStream(const char* normalizedPath, int flags,
                    InputStream** is_out,
                    OutputStream** os_out,
                    IOStream** io_out) = 0;
            virtual bool Stat(const char* normalizedPath, FSStat_t* stat_out) = 0;

            virtual int CompareTimestamps(const char* leftPath, const char* rightPath, int64_t* diff_out, int flags) = 0;
            virtual const char* GetNativeAbsoluteFilename(const char* normalizedPath) = 0;
    };

    class IFSUnion
    {
        public:
            virtual ~IFSUnion() {}

            virtual IFileSystem* GetFileSystem() = 0;

            virtual void AddFileSystem(shared_ptr<IFileSystem>&& fs, int priority, const char* mountPoint) = 0;
            virtual bool RemoveFileSystem(IFileSystem* fs) = 0;

            void AddFileSystem(shared_ptr<IFileSystem>&& fs, int priority)
            {
                AddFileSystem(std::forward<shared_ptr<IFileSystem>>(fs), priority, nullptr);
            }
    };
}
