
#include "ztype.hpp"

#include <littl/File.hpp>
#include <littl/String.hpp>

#include <ctype.h>

namespace ztype
{
    using namespace li;

    class FaceListFileImpl : public FaceListFile
    {
        protected:
            std::unique_ptr<InputStream> file;
            char font_filename[512], cache_filename[512];

            char* Trim(char* str);

        public:
            FaceListFileImpl(std::unique_ptr<InputStream>&& file);
            virtual ~FaceListFileImpl() {}

            virtual bool ReadEntry(FaceDesc *desc) override;
    };

    FaceListFileImpl::FaceListFileImpl(std::unique_ptr<InputStream>&& file)
            : file(std::move(file))
    {
    }

    bool FaceListFileImpl::ReadEntry(FaceDesc *desc)
    {
        while (!file->eof())
        {
            String line = file->readLine();

            if (line.isEmpty() || line.beginsWith('#'))
                continue;

            char flags_list[100];

            //int num = ssscanf(line, 0, "%S;%n;%S;%n;%n;%S", font_filename, sizeof(font_filename), &desc->size, flags_list, sizeof(flags_list),
            //        &desc->range_min, &desc->range_max, cache_filename, sizeof(cache_filename));
            int num = (abort(), -1);

            if (num < 5)
                return false;

            desc->filename = Trim(font_filename);
            desc->flags = 0;

            if (strstr(flags_list, "BOLD") != nullptr)
                desc->flags |= FACE_BOLD;

            if (strstr(flags_list, "ITALIC") != nullptr)
                desc->flags |= FACE_ITALIC;

            if (num >= 6)
                desc->cache_filename = Trim(cache_filename);
            else
                desc->cache_filename = nullptr;

            return true;
        }

        return false;
    }

    char* FaceListFileImpl::Trim(char* str)
    {
        while (isspace(*str))
            str++;

        size_t length = strlen(str);

        while (length > 0 && isspace(str[length-1]))
        {
            length--;
            str[length] = 0;
        }

        return str;
    }

    FaceListFile* FaceListFile::Open(const char *file_name)
    {
        std::unique_ptr<File> file(File::open(file_name));

        if (file == nullptr)
            return nullptr;

        return new FaceListFileImpl(std::move(file));
    }

    FaceListFile* FaceListFile::Open(std::unique_ptr<InputStream>&& stream)
    {
        if (stream == nullptr)
            return nullptr;

        return new FaceListFileImpl(std::move(stream));
    }
}
