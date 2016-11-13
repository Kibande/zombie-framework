#pragma once

#include <littl/BaseIO.hpp>

namespace zshared
{
    enum {ZMF_SECT_COMPRESSIBLE = 1, ZMF_SECT_RELOCATABLE = 2, ZMF_SECT_UNIQUE = 4, ZMF_SECT_XREF = 8, ZMF_SECT_CP_ZLIB = 16};

    struct MediaFileHeader
    {
        uint8_t magic[4];
        uint16_t revision;
        uint16_t flags;
        uint64_t sect_table;
    };

    struct SectionTableHeader
    {
        uint32_t flags;
        uint32_t num_sections;
        uint32_t dn_table_entries;
        uint32_t dn_table_length;
    };

    struct SectionTableEntry
    {
        uint8_t FourCC[4];
        uint32_t dn_table_index;
        uint32_t flags;
        uint32_t reserved;
        uint64_t offset;
        uint64_t length;
    };

    inline int CmpFourCC(const void* FourCC1, const void* FourCC2)
    {
        return *(const uint32_t *) FourCC1 - *(const uint32_t *) FourCC2;
    }

    inline const uint8_t* ToFourCC(const char FourCC[4])
    {
        return (const uint8_t *) FourCC;
    }

    class MediaFileReader
    {
        public:
            struct Section
            {
                SectionTableEntry table_entry;
                li::String domainName;
            };

        protected:
            li::Reference<li::SeekableInputStream> input;

            bool haveHeader;
            MediaFileHeader header;
            SectionTableHeader mst_header;

            MediaFileReader(li::SeekableInputStream* input);
            int ReadHeader();

        public:
            static MediaFileReader* Open(li::SeekableInputStream* input);

            li::InputStream* OpenSection(const Section& s);
            int ReadSectionTable(li::List<Section>& sections);
    };

    class MediaFileWriter
    {
        protected:
            struct Section
            {
                SectionTableEntry table_entry;
                li::String domainName;

                enum { SECT_BUFFER } type;

                li::ArrayIOStream buffer;
            };

            li::Reference<li::OutputStream> output;
            li::List<Section> sections;

            MediaFileHeader header;
            SectionTableHeader mst_header;

            li::ArrayIOStream compressedMst;

        public:
            MediaFileWriter(li::OutputStream* output);

            li::ArrayIOStream& AddSection(const char* dn, int flags);
            li::ArrayIOStream& AddSection(const uint8_t* FourCC, int flags);

            void AddUtf8Section(const char* dn, const char* text, int flags);

            int WriteAll();
    };
}
