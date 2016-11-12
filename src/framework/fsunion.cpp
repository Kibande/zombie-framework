
#include <framework/errorbuffer.hpp>
#include <framework/filesystem.hpp>

#include <utility>

// FIXME: SetErrors

namespace zfw
{
    using namespace li;

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    class DirectoryUnion : public IDirectory
    {
        public:
            DirectoryUnion(List<IDirectory*>&& directories);
            ~DirectoryUnion() { iterate2 (i, directories) delete i; }

            virtual const char* ReadDir() override;

        protected:
            List<IDirectory*> directories;
            size_t index;
    };

    class FSUnion : public IFileSystem, public IFSUnion
    {
        public:
            FSUnion(ErrorBuffer_t* eb);

            // IFSUnion
            virtual IFileSystem* GetFileSystem() override { return this; }

            virtual void AddFileSystem(shared_ptr<IFileSystem>&& fs, int priority, const char* mountPoint) override;
            virtual bool RemoveFileSystem(IFileSystem* fs) override;

            // IFSHandler
            virtual IDirectory* OpenDirectory(const char* normalizedPath, int flags) override;
            virtual bool OpenFileStream(const char* normalizedPath, int flags,
                    InputStream** is_out,
                    OutputStream** os_out,
                    IOStream** io_out) override;
            virtual bool Stat(const char* normalizedPath, FSStat_t* stat_out) override;

            virtual int CompareTimestamps(const char* leftPath, const char* rightPath, int64_t* diff_out, int flags) override;
            virtual const char* GetNativeAbsoluteFilename(const char* normalizedPath) override;

        protected:
            struct FileSystem_t
            {
                shared_ptr<IFileSystem> fs;
                int priority;
                String mountPoint;
            };

            ErrorBuffer_t* eb;

            List<FileSystem_t, size_t, Allocator<FileSystem_t>, ArrayOptions::noBoundsChecking> fileSystems;
            List<IDirectory*> dirList;
    };

    // ====================================================================== //
    //  class DirectoryUnion
    // ====================================================================== //

    DirectoryUnion::DirectoryUnion(List<IDirectory*>&& directories)
            : directories(std::move(directories))
    {
        index = 0;
    }

    const char* DirectoryUnion::ReadDir()
    {
        for (; index < directories.getLength(); index++)
        {
            const char* next = directories[index]->ReadDir();

            if (next != nullptr)
                return next;
        }

        return nullptr;
    }

    // ====================================================================== //
    //  class FSUnion
    // ====================================================================== //

    IFSUnion* p_CreateFSUnion(ErrorBuffer_t* eb)
    {
        return new FSUnion(eb);
    }

    FSUnion::FSUnion(ErrorBuffer_t* eb)
    {
        this->eb = eb;
    }

    void FSUnion::AddFileSystem(shared_ptr<IFileSystem>&& fs, int priority, const char* mountPoint)
    {
        size_t i;

        for (i = 0; i < fileSystems.getLength(); i++)
            if (fileSystems[i].priority < priority)
                break;

        auto& entry = fileSystems.insertEmpty(i);
        entry.fs = move(fs);
        entry.priority = priority;
        entry.mountPoint = mountPoint;
    }

    int FSUnion::CompareTimestamps(const char* leftPath, const char* rightPath, int64_t* diff_out, int flags)
    {
        FSStat_t left, right;
        int ret = 0;

        if (!Stat(leftPath, &left))
        {
            ret |= FS_LEFT_NOT_FOUND;

            if (flags & FS_REQUIRE_BOTH)
                return ret;

            left.modificationTime = 0;
        }

        if (!Stat(rightPath, &right))
        {
            ret |= FS_RIGHT_NOT_FOUND;

            if (flags & FS_REQUIRE_BOTH)
                return ret;

            right.modificationTime = 0;
        }

        *diff_out = left.modificationTime - right.modificationTime;
        return ret;
    }

    const char* FSUnion::GetNativeAbsoluteFilename(const char* normalizedPath)
    {
        bool breakOnError = false;

        for (auto& fs : fileSystems)
        {
            if (strncmp(fs.mountPoint, normalizedPath, fs.mountPoint.getNumBytes()) != 0)
                continue;

            const char* naf = fs.fs->GetNativeAbsoluteFilename(normalizedPath + fs.mountPoint.getNumBytes());

            if (naf != nullptr)
                return naf;
            else if (breakOnError && eb->errorCode != EX_NOT_FOUND)
                break;
        }

        return nullptr;
    }

    IDirectory* FSUnion::OpenDirectory(const char* normalizedPath, int flags)
    {
        bool breakOnError = false;

        for (auto& fs : fileSystems)
        {
            if (strncmp(fs.mountPoint, normalizedPath, fs.mountPoint.getNumBytes()) != 0)
                continue;

            IDirectory* dir = fs.fs->OpenDirectory(normalizedPath + fs.mountPoint.getNumBytes(), flags);

            if (dir != nullptr)
                dirList.add(dir);
            else if (breakOnError && eb->errorCode != EX_NOT_FOUND)
                break;
        }

        if (dirList.isEmpty())
            return nullptr;

        return new DirectoryUnion(std::move(dirList));
    }

    bool FSUnion::OpenFileStream(const char* normalizedPath, int flags,
            InputStream** is_out,
            OutputStream** os_out,
            IOStream** io_out)
    {
        bool breakOnError = false;

        for (auto& fs : fileSystems)
        {
            if (strncmp(fs.mountPoint, normalizedPath, fs.mountPoint.getNumBytes()) != 0)
                continue;

            if (fs.fs->OpenFileStream(normalizedPath + fs.mountPoint.getNumBytes(), flags, is_out, os_out, io_out))
                return true;
            else if (breakOnError && eb->errorCode != EX_NOT_FOUND)
                break;
        }

        if (fileSystems.isEmpty() || eb->errorCode == EX_NOT_FOUND)
            return ErrorBuffer::SetError3(eb->errorCode, 3,
                    "desc", sprintf_4095("File not found: '%s'", normalizedPath),
                    "normalizedPath", normalizedPath,
                    "flags", sprintf_15("%d", flags)
                    ), false;

        return false;
    }

    bool FSUnion::RemoveFileSystem(IFileSystem* fs)
    {
        for (size_t i = 0; i < fileSystems.getLength(); i++)
        {
            if (fileSystems[i].fs.get() == fs)
            {
                fileSystems[i].fs.reset();
                fileSystems.remove(i);
                return true;
            }
        }

        return false;
    }

    bool FSUnion::Stat(const char* normalizedPath, FSStat_t* stat_out)
    {
        bool breakOnError = false;

        for (auto& fs : fileSystems)
        {
            if (fs.fs->Stat(normalizedPath, stat_out))
                return true;
            else if (breakOnError && eb->errorCode != EX_NOT_FOUND)
                break;
        }

        return false;
    }
}
