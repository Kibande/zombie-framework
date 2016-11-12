
#include "ztype.hpp"

#include <littl/List.hpp>
#include <littl/String.hpp>

#include <parse_args.h>

#if defined(_WIN32) || defined(_WIN64)
#include <direct.h>
#else
#include <sys/stat.h>
#endif

using namespace li;
using namespace ztype;

static String input_filename, output_filename;
static List<String> inputs;
static String cache_dir = "fontcache";
static bool force = 0, lists = 0, verbose = 0;
static int size = -1, min = -1, max = -1;

static int on_arg(int type, const char* arg, const char* ext)
{
    if (type == ARG_DEFAULT)
    {
        if (lists)
            inputs.add(arg);
        else
            input_filename = arg;
    }
    else if (type == ARG_SINGLE_CHAR && arg[1] == 'f')
        force = 1;
    else if (type == ARG_SINGLE_CHAR && arg[1] == 'l')
    {
        if (!input_filename.isEmpty())
            return -1;

        lists = 1;
    }
    else if (type == ARG_SINGLE_CHAR && arg[1] == 'v')
        verbose = 1;
    else if (type == ARG_SINGLE_CHAR_EXT && arg[1] == 'd')
        cache_dir = ext;
    else if (type == ARG_SINGLE_CHAR_EXT && arg[1] == 'm')
        min = strtol(ext, nullptr, 0);
    else if (type == ARG_SINGLE_CHAR_EXT && arg[1] == 'M')
        max = strtol(ext, nullptr, 0);
    else if (type == ARG_SINGLE_CHAR_EXT && arg[1] == 's')
        size = strtol(ext, nullptr, 0);
    else if (type == ARG_SINGLE_CHAR_EXT && arg[1] == 'o')
        output_filename = ext;

    return 0;
}

static void on_err(const char* arg)
{
    printf("buildfontcache: unrecognized argument `%s`\n", arg);
}

static const parse_args_t buildfontcache_args = {
    "flv", "dmosM",
    NULL, NULL,

    on_arg,
    on_err
};

int main(int argc, char **argv)
{
    if (parse_args(argc - 1, argv + 1, &buildfontcache_args) < 0
            || (lists && inputs.isEmpty())
            || (!lists && (input_filename.isEmpty() || output_filename.isEmpty() || size <= 0 || min < 0 || max < 0))
        )
    {
        fprintf(stderr, "usage: buildfontcache [-f] [-v] <filename> -s <size> -o <output> -m <min> -M <max>\n");
        fprintf(stderr, "usage: buildfontcache [-f] [-v] [-d <cachedir>] -l <facelist>+\n");
        return -1;
    }

    std::unique_ptr<ztype::FontCache> cache(ztype::FontCache::Create());

    int build_count = 0;

    if (lists)
    {
#if defined(_WIN32) || defined(_WIN64)
        _mkdir(cache_dir);
#else
        // FIXME: access rights
        mkdir(cache_dir, 0777);
#endif

        iterate2 (i, inputs)
        {
            std::unique_ptr<FaceListFile> face_list(FaceListFile::Open((String&) i));

            if (face_list == nullptr)
            {
                fprintf(stderr, "buildfontcache: failed to open `%s`\n", ((String&) i).c_str());
                continue;
            }

            int n;

            if ((n = cache->BuildUpdateList(face_list.get(), cache_dir, force, verbose)) < 0)
            {
                fprintf(stderr, "buildfontcache: failed to process `%s`\n", ((String&) i).c_str());
                continue;
            }

            build_count += n;
        }
    }
    else
    {
        FaceDesc desc = {input_filename, nullptr, size, 0, min, max};

        int n;

        if ((n = cache->AddToUpdateList(&desc, output_filename, force, verbose)) < 0)
        {
            fprintf(stderr, "buildfontcache: failed to add `%s` to update list\n", input_filename.c_str());
            return -1;
        }

        build_count += n;
    }

    if (verbose)
        printf("\n====================\nbuildfontcache: Will build cache for %i faces\n====================\n\n", build_count);

    cache->UpdateCache(verbose);

    return 0;
}
