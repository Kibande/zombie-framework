
#include <framework/errorbuffer.hpp>
#include <framework/filesystem.hpp>
#include <framework/system.hpp>

#include <littl/File.hpp>

#include <bleb/repository.hpp>
#include <bleb/byteio_stdio.hpp>

#include <littl+bleb/ByteIOStream.hpp>

#include "../private.hpp"

namespace zfw
{
    using namespace li;

    class FileSystemBleb;

    // ====================================================================== //
    //  class declarations
    // ====================================================================== //

    class FileSystemBleb : public IFileSystem
    {
        public:
            FileSystemBleb(ISystem* sys, const char* path, int access);
            virtual ~FileSystemBleb();

            virtual IDirectory* OpenDirectory(const char* normalizedPath, int flags) override;
            virtual bool OpenFileStream(const char* normalizedPath, int flags,
                    InputStream** is_out,
                    OutputStream** os_out,
                    IOStream** io_out) override;
            virtual bool Stat(const char* normalizedPath, FSStat_t* stat_out) override;

            virtual int CompareTimestamps(const char* leftPath, const char* rightPath, int64_t* diff_out, int flags) override;
            const char* GetNativeAbsoluteFilename(const char* normalizedPath) override;

        protected:
            ISystem* sys;
            String path;

            bleb::StdioFileByteIO bio;
            bleb::Repository repo;

            int access;

            String tempString;
    };

    // ====================================================================== //
    //  class FileSystemBleb
    // ====================================================================== //

    shared_ptr<IFileSystem> p_CreateBlebFileSystem(ISystem* sys, const char* path, int access)
    {
        // Require Stat access at the very least (other methods assume it)
        ZFW_ASSERT(access & kFSAccessStat)

        return std::make_shared<FileSystemBleb>(sys, path, access);
    }

    FileSystemBleb::FileSystemBleb(ISystem* sys, const char* path, int access)
            : sys(sys), path(path), bio(nullptr, true), repo(&bio, false)
    {
        sys->Printf(kLogInfo, "FileSystemBleb: mounting '%s'", path);
        bio.file = fopen(path, "rb");
        zombie_assert(bio.file);

        this->access = access;

        if (!repo.open(false))
        {
            sys->Printf(kLogError, "Bleb error: %s", repo.getErrorDesc());
            zombie_assert(false);
        }
    }

    FileSystemBleb::~FileSystemBleb()
    {
        sys->Printf(kLogInfo, "FileSystemBleb: unmounting '%s'", path.c_str());
    }

    int FileSystemBleb::CompareTimestamps(const char* leftPath, const char* rightPath, int64_t* diff_out, int flags)
    {
        return FS_LEFT_NOT_FOUND | FS_RIGHT_NOT_FOUND;
    }

    const char* FileSystemBleb::GetNativeAbsoluteFilename(const char* normalizedPath)
    {
        return ErrorBuffer::SetError3(EX_NOT_FOUND, 0), nullptr;
    }

    IDirectory* FileSystemBleb::OpenDirectory(const char* normalizedPath, int flags)
    {
        return ErrorBuffer::SetError3(EX_NOT_FOUND, 0), nullptr;
    }

    bool FileSystemBleb::OpenFileStream(const char* normalizedPath, int flags,
            InputStream** is_out,
            OutputStream** os_out,
            IOStream** io_out)
    {
        // Collect the required access permissions
        int reqAccess = kFSAccessStat;

        if (is_out != nullptr || io_out != nullptr)
            reqAccess |= kFSAccessRead;
        
        if (os_out != nullptr || io_out != nullptr || (flags & kFileTruncate))
            reqAccess |= kFSAccessWrite;

        // Test permissions
        if ((reqAccess & ~access) != 0)
            return ErrorBuffer::SetError3(EX_ACCESS_DENIED, 0), false;

        if (((reqAccess | kFSAccessCreateFile) & ~access) != 0)
            flags &= ~kFileMayCreate;

        const char* fullPath = normalizedPath;

        unique_ptr<ByteIOStream> file;

        int blebFlags = 0;

        if (flags & kFileMayCreate)
            blebFlags |= bleb::kStreamCreate;

        if (flags & kFileTruncate)
            blebFlags |= bleb::kStreamTruncate;

        file = ByteIOStream::createFrom(repo.openStream(fullPath, blebFlags).release(), true);

        if (file == nullptr)
            return ErrorBuffer::SetError3(EX_NOT_FOUND, 0), false;

        if (is_out != nullptr)
        {
            zombie_assert(os_out == nullptr && io_out == nullptr);
            *is_out = file.release();
        }
        else if (os_out != nullptr)
        {
            zombie_assert(is_out == nullptr && io_out == nullptr);
            *os_out = file.release();
        }
        else if (io_out != nullptr)
        {
            zombie_assert(is_out == nullptr && os_out == nullptr);
            *io_out = file.release();
        }
        else
        {
            zombie_assert(is_out != nullptr || os_out != nullptr || io_out != nullptr);
        }

        return true;
    }

    bool FileSystemBleb::Stat(const char* normalizedPath, FSStat_t* stat_out)
    {
        return ErrorBuffer::SetError3(EX_NOT_FOUND, 0), false;
    }
};
