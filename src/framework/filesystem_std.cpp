
#include <framework/errorbuffer.hpp>
#include <framework/filesystem.hpp>

#include <littl/Directory.hpp>
#include <littl/File.hpp>

#include "private.hpp"

namespace zfw
{
    using namespace li;

    class FileSystemStd;

    // ====================================================================== //
    //  class declarations
    // ====================================================================== //

    class DirectoryStd : public IDirectory
    {
        public:
            DirectoryStd(ErrorBuffer_t* eb, FileSystemStd* fsh, Directory* dir);

            virtual const char* ReadDir() override;

        protected:
            ErrorBuffer_t* eb;
            FileSystemStd* fsh;

            unique_ptr<Directory> dir;

            List<String> fileNames;
            size_t index;
    };

    class FileSystemStd : public IFileSystem
    {
        public:
            FileSystemStd(ErrorBuffer_t* eb, const char* basePath, int access);

            virtual IDirectory* OpenDirectory(const char* normalizedPath, int flags) override;
            virtual bool OpenFileStream(const char* normalizedPath, int flags,
                    InputStream** is_out,
                    OutputStream** os_out,
                    IOStream** io_out) override;
            virtual bool Stat(const char* normalizedPath, FSStat_t* stat_out) override;

            virtual int CompareTimestamps(const char* leftPath, const char* rightPath, int64_t* diff_out, int flags) override;
            const char* GetNativeAbsoluteFilename(const char* normalizedPath) override;

        protected:
            ErrorBuffer_t* eb;
            String basePath;
            int access;

            String tempString;
    };

    // ====================================================================== //
    //  class DirectoryStd
    // ====================================================================== //

    DirectoryStd::DirectoryStd(ErrorBuffer_t* eb, FileSystemStd* fsh, Directory* dir)
            : dir(dir)
    {
        this->eb = eb;
        this->fsh = fsh;

        index = 0;
        dir->list(fileNames);
    }

    const char* DirectoryStd::ReadDir()
    {
        for (; index < fileNames.getLength(); index++)
        {
            if (fileNames[index] == "." || fileNames[index] == "..")
                continue;

            return fileNames[index++];
        }

        return nullptr;
    }

    // ====================================================================== //
    //  class FileSystemStd
    // ====================================================================== //

    shared_ptr<IFileSystem> p_CreateStdFileSystem(ErrorBuffer_t* eb, const char* absolutePathPrefix, int access)
    {
        // Require Stat access at the very least (other methods assume it)
        ZFW_ASSERT(access & kFSAccessStat)

        return std::make_shared<FileSystemStd>(eb, absolutePathPrefix, access);
    }

    FileSystemStd::FileSystemStd(ErrorBuffer_t* eb, const char* basePath, int access)
            : basePath(basePath)
    {
        this->eb = eb;
        this->access = access;
    }

    int FileSystemStd::CompareTimestamps(const char* leftPath, const char* rightPath, int64_t* diff_out, int flags)
    {
        String fullPathLeft = basePath + leftPath;
        String fullPathRight = basePath + rightPath;

        FileStat left, right;
        int ret = 0;

        if (!File::statFileOrDirectory(fullPathLeft, &left))
        {
            ret |= FS_LEFT_NOT_FOUND;

            if (flags & FS_REQUIRE_BOTH)
                return ret;

            left.modificationTime = 0;
        }

        if (!File::statFileOrDirectory(fullPathRight, &right))
        {
            ret |= FS_RIGHT_NOT_FOUND;

            if (flags & FS_REQUIRE_BOTH)
                return ret;

            right.modificationTime = 0;
        }

        *diff_out = left.modificationTime - right.modificationTime;
        return ret;
    }

    const char* FileSystemStd::GetNativeAbsoluteFilename(const char* normalizedPath)
    {
        String fullPath = basePath + normalizedPath;

        FileStat stat;

        if (File::statFileOrDirectory(fullPath, &stat))
        {
            tempString = fullPath;
            return tempString.c_str();
        }
        else
            return ErrorBuffer::SetError(eb, EX_NOT_FOUND, nullptr),
                    nullptr;
    }

    IDirectory* FileSystemStd::OpenDirectory(const char* normalizedPath, int flags)
    {
        if ((kFSAccessCreateFile & ~access) == 0)
            flags &= ~kDirectoryMayCreate;

        String fullPath = basePath + normalizedPath;

        Directory* dir = nullptr;

        dir = Directory::open(fullPath);

        if (dir != nullptr)
            return new DirectoryStd(eb, this, dir);

        if (flags & kDirectoryMayCreate)
        {
            if (Directory::create(fullPath))
                dir = Directory::open(fullPath);

            if (dir != nullptr)
                return new DirectoryStd(eb, this, dir);
        }

        return ErrorBuffer::SetError2(eb, EX_NOT_FOUND, 0), nullptr;
    }

    bool FileSystemStd::OpenFileStream(const char* normalizedPath, int flags,
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
            return ErrorBuffer::SetError2(eb, EX_ACCESS_DENIED, 0), false;

        if (((reqAccess | kFSAccessCreateFile) & ~access) != 0)
            flags &= ~kFileMayCreate;

        String fullPath = basePath + normalizedPath;

        unique_ptr<File> file;

        if (reqAccess & kFSAccessWrite)
        {
            // Open file for writing (and possibly reading)

            if ((flags & (kFileMayCreate | kFileTruncate)) == (kFileMayCreate | kFileTruncate))
                file.reset(File::open(fullPath, (reqAccess & kFSAccessRead) ? "wb+" : "wb"));
            else if (flags & kFileTruncate)
            {
                file.reset(File::open(fullPath, "rb+"));

                if (file != nullptr)
                {
                    file.reset();
                    file.reset(File::open(fullPath, (reqAccess & kFSAccessRead) ? "wb+" : "wb"));
                }
            }
            else
            {
                file.reset(File::open(fullPath, "rb+"));

                if (file == nullptr && (flags & kFileMayCreate))
                    file.reset(File::open(fullPath, (reqAccess & kFSAccessRead) ? "wb+" : "wb"));
            }
        }
        else
        {
            // Open file for reading

            if ((flags & (kFileMayCreate | kFileTruncate)) == (kFileMayCreate | kFileTruncate))
            {
                file.reset(File::open(fullPath, "wb"));
                file.reset();
                file.reset(File::open(fullPath, "rb"));
            }
            else if (flags & kFileTruncate)
            {
                file.reset(File::open(fullPath, "rb+"));

                if (file != nullptr)
                {
                    file.reset();
                    file.reset(File::open(fullPath, "wb"));
                    file.reset();
                    file.reset(File::open(fullPath, "rb"));
                }
            }
            else
            {
                file.reset(File::open(fullPath, "rb"));

                if (file == nullptr && (flags & kFileMayCreate))
                {
                    file.reset(File::open(fullPath, "wb"));
                    file.reset();
                    file.reset(File::open(fullPath, "rb"));
                }
            }
        }

        if (file == nullptr)
            return ErrorBuffer::SetError2(eb, EX_NOT_FOUND, 0), false;

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

    bool FileSystemStd::Stat(const char* normalizedPath, FSStat_t* stat_out)
    {
        String fullPath = basePath + normalizedPath;

        FileStat stat;
        
        if (!File::statFileOrDirectory(fullPath, &stat))
            return ErrorBuffer::SetError(eb, EX_NOT_FOUND, nullptr), false;

        stat_out->sizeInBytes = (stat.flags & FileStat::is_file) ? stat.sizeInBytes : 0;
        stat_out->creationTime = stat.creationTime;
        stat_out->modificationTime = stat.modificationTime;
        stat_out->isDirectory = !!(stat.flags & FileStat::is_directory);

        return true;
    }
};
