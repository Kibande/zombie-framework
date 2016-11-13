
#include "ztype.hpp"

#include <littl/File.hpp>
#include <littl/String.hpp>

#include <ctype.h>
#include <superscanf.h>

namespace ztype
{
    using namespace li;

    class FaceListFileImpl : public FaceListFile
    {
        protected:
            Reference<InputStream> file;
            char font_filename[512], cache_filename[512];

            char* Trim(char* str);

        public:
            FaceListFileImpl(InputStream* file);
            virtual ~FaceListFileImpl() {}

            virtual bool ReadEntry(FaceDesc *desc) override;
    };

    FaceListFileImpl::FaceListFileImpl(InputStream* file)
            : file(file)
    {
    }

    bool FaceListFileImpl::ReadEntry(FaceDesc *desc)
    {
        while (file->isReadable())
        {
            String line = file->readLine();

            if (line.isEmpty() || line.beginsWith('#'))
                continue;

            char flags_list[100];

            int num = ssscanf(line, 0, "%S;%n;%S;%n;%n;%S", font_filename, sizeof(font_filename), &desc->size, flags_list, sizeof(flags_list),
                    &desc->range_min, &desc->range_max, cache_filename, sizeof(cache_filename));

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
        Reference<File> file = File::open(file_name);

        if (file == nullptr)
            return nullptr;

        return new FaceListFileImpl(file.detach());
    }

    FaceListFile* FaceListFile::Open(InputStream* stream)
    {
        if (stream == nullptr)
            return nullptr;

        return new FaceListFileImpl(stream);
    }
}
