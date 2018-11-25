
#include <zshared/mediafile2.hpp>

#include <littl/Base.hpp>
#include <littl/File.hpp>

using namespace li;

static const char* app_tag = "name=zmftool,version=1.0,vendor=Xeatheran Minexew";

static int usage()
{
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "\tzmftool add-section <filename> <section-name> <input-file>\n");
    fprintf(stderr, "\tzmftool dump-section <filename> <section-name>\n");
    fprintf(stderr, "\tzmftool get <filename> <key>\n");
    fprintf(stderr, "\tzmftool info <filename>\n");
    fprintf(stderr, "\tzmftool set <filename> <key> <value>\n");
    fprintf(stderr, "\tzmftool touch <filename>\n");
    fprintf(stderr, "\n");

    return -1;
}

static int add_section_main(int argc, char** argv)
{
    if (argc < 4)
        return usage();

    li_Reference2(zshared::MediaFile, &zshared::MediaFile::Release) mediaFile = zshared::MediaFile::Create();

    if (!mediaFile->Open(argv[2], false, true))
    {
        fprintf(stderr, "zmftool: failed to open '%s': %s\n", argv[2], mediaFile->GetErrorDesc());
        return -1;
    }

    Reference<InputStream> input = File::open(argv[4]);

    if (input == nullptr)
    {
        fprintf(stderr, "zmftool: failed to open file '%s'\n", argv[4]);
        return -1;
    }

    Reference<OutputStream> section = mediaFile->CreateSection(argv[3]);

    section->copyFrom(input);
    return 0;
}

static int dump_section_main(int argc, char** argv)
{
    if (argc < 3)
        return usage();

    li_Reference2(zshared::MediaFile, &zshared::MediaFile::Release) mediaFile = zshared::MediaFile::Create();

    if (!mediaFile->Open(argv[2], true, false))
    {
        fprintf(stderr, "zmftool: failed to open '%s': %s\n", argv[2], mediaFile->GetErrorDesc());
        return -1;
    }

    Reference<InputStream> section = mediaFile->OpenSection(argv[3]);

    if (section == nullptr)
    {
        fprintf(stderr, "zmftool: section '%s' does not exist\n", argv[3]);
        return -1;
    }

    for ( ; section->isReadable(); )
    {
        uint8_t buf[1024];

        size_t read = section->read(buf, sizeof(buf));
        fwrite(buf, 1, read, stdout);
    }

    return 0;
}

static int get_main(int argc, char** argv)
{
    if (argc < 3)
        return usage();

    li_Reference2(zshared::MediaFile, &zshared::MediaFile::Release) mediaFile = zshared::MediaFile::Create();

    if (!mediaFile->Open(argv[2], true, false))
    {
        fprintf(stderr, "zmftool: failed to open '%s': %s\n", argv[2], mediaFile->GetErrorDesc());
        return -1;
    }

    const char* value = mediaFile->GetMetadata(argv[3]);

    if (value != nullptr)
        printf("%s=%s\n", argv[3], value);
    else
        fprintf(stderr, "zmftool: '%s' not set\n", argv[3]);

    return 0;
}

static int info_main(int argc, char** argv)
{
    if (argc < 3)
        return usage();

    li_Reference2(zshared::MediaFile, &zshared::MediaFile::Release) mediaFile = zshared::MediaFile::Create();

    if (!mediaFile->Open(argv[2], true, false))
    {
        fprintf(stderr, "zmftool: failed to open '%s': %s\n", argv[2], mediaFile->GetErrorDesc());
        return -1;
    }

    int sectorSize =    (int) mediaFile->GetSectorSize();
    int fileSize =      (int) mediaFile->GetFileSize();

    printf("=== basic info ===\n");
    printf("file:               %s (Zombie media file)\n", argv[2]);
    printf("bitstream type:     %s\n", mediaFile->GetBitstreamType());
    printf("sector size:        %i bytes\n", sectorSize);
    printf("file size:          %i bytes (%i sectors)\n", fileSize, (fileSize + sectorSize - 1) / sectorSize);
    printf("\n");

    printf("=== metadata ===\n");

    class MetadataEntryListener : public zshared::IMetadataEntryListener
    {
        public:
            virtual int OnMetadataEntry(uint16_t key_crc16, const char* key, const char* value) override
            {
                printf("%s=%s\n", key, value);
                return 1;
            }
    };

    MetadataEntryListener mel;
    mediaFile->IterateMetadata(&mel);

    printf("\n");

    printf("=== section list ===\n");

    class SectionListener : public zshared::ISectionEntryListener
    {
        public:
            virtual int OnSectionEntry(uint16_t name_crc16, const char* name, const char* desc,
                    const uint8_t compression[4], uint64_t data_length, uint64_t compressed_length) override
            {
                printf("%s\t(%u bytes", name, (unsigned int) data_length);

                if (memcmp(compression, "\0\0\0\0", 4) != 0)
                    printf(", %u compressed using %c%c%c%c", (unsigned int) compressed_length,
                           compression[0], compression[1], compression[2], compression[3]);

                printf(")\n\t%s\n", desc);
                return 1;
            }
    };

    SectionListener sl;
    mediaFile->IterateSections(&sl);

    printf("\n");

    return 0;
}

static int set_main(int argc, char** argv)
{
    if (argc < 4)
        return usage();

    li_Reference2(zshared::MediaFile, &zshared::MediaFile::Release) mediaFile = zshared::MediaFile::Create();

    if (!mediaFile->Open(argv[2], false, false))
    {
        fprintf(stderr, "zmftool: failed to open '%s': %s\n", argv[2], mediaFile->GetErrorDesc());
        return -1;
    }

    mediaFile->SetMetadata(argv[3], argv[4]);
    return 0;
}

static int touch_main(int argc, char** argv)
{
    if (argc < 3)
        return usage();

    li_Reference2(zshared::MediaFile, &zshared::MediaFile::Release) mediaFile = zshared::MediaFile::Create();

    if (!mediaFile->Open(argv[2], false, true))
    {
        fprintf(stderr, "zmftool: failed to open '%s': %s\n", argv[2], mediaFile->GetErrorDesc());
        return -1;
    }

    mediaFile->SetMetadata("media.original_name", argv[2]);
    mediaFile->SetMetadata("media.authored_using", app_tag);

    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 2)
        return usage();

    if (strcmp(argv[1], "add-section") == 0)
        return add_section_main(argc, argv);
    else if (strcmp(argv[1], "dump-section") == 0)
        return dump_section_main(argc, argv);
    else if (strcmp(argv[1], "get") == 0)
        return get_main(argc, argv);
    else if (strcmp(argv[1], "info") == 0)
        return info_main(argc, argv);
    if (strcmp(argv[1], "set") == 0)
        return set_main(argc, argv);
    else if (strcmp(argv[1], "touch") == 0)
        return touch_main(argc, argv);
    else
        return usage();

    return 0;
}
