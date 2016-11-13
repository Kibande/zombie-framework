
#include "mediafile.hpp"

namespace zshared
{
    inline bool IS_FOURCC_CDN(uint8_t FourCC[4])
    {
        return *(uint32_t *)FourCC == *(uint32_t *)"#CDN";
    }

    inline void SET_FOURCC(uint8_t FourCC[4], const void* from)
    {
        *(uint32_t *)FourCC = *(uint32_t *)from;
    }

    inline void SET_FOURCC_CDN(uint8_t FourCC[4])
    {
        *(uint32_t *)FourCC = *(uint32_t *)"#CDN";
    }

    MediaFileReader::MediaFileReader(li::SeekableInputStream* input) : input(input)
    {
        haveHeader = false;
    }

    MediaFileReader* MediaFileReader::Open(li::SeekableInputStream* input)
    {
        return (input != nullptr) ? new MediaFileReader(input) : nullptr;
    }

    li::InputStream* MediaFileReader::OpenSection(const Section& s)
    {
        return new li::SeekableInputStreamSegment(input->reference(), s.table_entry.offset, s.table_entry.length);
    }

    int MediaFileReader::ReadHeader()
    {
        haveHeader = false;

        input->setPos(0);

        if (!input->readItems(&header, 1))
            return -1;

        input->setPos(header.sect_table);

        if (!input->readItems(&mst_header, 1))
            return -1;

        haveHeader = true;
        return 0;
    }

    int MediaFileReader::ReadSectionTable(li::List<Section>& sections)
    {
        int e;

        if (!haveHeader)
        {
            if ((e = ReadHeader()) < 0)
                return e;
        }

        input->setPos(header.sect_table + sizeof(SectionTableHeader));

        li::Array<char> dn_table(mst_header.dn_table_length);

        if (input->read(dn_table.c_array(), mst_header.dn_table_length) != mst_header.dn_table_length)
            return -1;

        sections.resize(sections.getLength() + mst_header.num_sections);

        size_t dn_table_offset = 0;
        size_t dn_table_index = 0;

        for (uint32_t i = 0; i < mst_header.num_sections; i++)
        {
            Section& s = sections.addEmpty();

            if (input->readItems(&s.table_entry, 1) != 1)
                return -1;

            if (IS_FOURCC_CDN(s.table_entry.FourCC))
            {
                if (s.table_entry.dn_table_index != dn_table_index)
                {
                    dn_table_offset = 0;

                    for (dn_table_index = 0; dn_table_index < s.table_entry.dn_table_index; dn_table_index++)
                    {
                        if (dn_table_offset + 2 >= mst_header.dn_table_length)
                            return -1;

                        dn_table_offset += 2 + *(uint16_t *) dn_table.getPtrUnsafe(dn_table_offset);
                    }
                }

                if (dn_table_offset + 2 >= mst_header.dn_table_length)
                    return -1;

                s.domainName.set( dn_table.getPtrUnsafe(dn_table_offset + 2) );
                dn_table_offset += 2 + *(uint16_t *) dn_table.getPtrUnsafe(dn_table_offset);
                dn_table_index++;
            }
        }

        return mst_header.num_sections;
    }

    MediaFileWriter::MediaFileWriter(li::OutputStream* output) : output(output)
    {
        memcpy(header.magic, "ZMF\n", 4);
        header.revision = 10;
        header.flags = 0;
        header.sect_table = 0;

        memset(&mst_header, 0, sizeof(mst_header));
    }

    li::ArrayIOStream& MediaFileWriter::AddSection(const char* dn, int flags)
    {
        auto& s = sections.addEmpty();

        // TODO: compression

        SET_FOURCC_CDN(s.table_entry.FourCC);
        s.table_entry.dn_table_index = 0;
        s.table_entry.flags = flags;
        s.table_entry.reserved = 0;
        s.table_entry.offset = 0;
        s.table_entry.length = 0;

        s.domainName = dn;

        s.type = Section::SECT_BUFFER;
        return s.buffer;
    }

    li::ArrayIOStream& MediaFileWriter::AddSection(const uint8_t* FourCC, int flags)
    {
        auto& s = sections.addEmpty();

        // TODO: compression

        SET_FOURCC(s.table_entry.FourCC, FourCC);
        s.table_entry.dn_table_index = 0;
        s.table_entry.flags = flags;
        s.table_entry.reserved = 0;
        s.table_entry.offset = 0;
        s.table_entry.length = 0;

        s.type = Section::SECT_BUFFER;
        return s.buffer;
    }

    void MediaFileWriter::AddUtf8Section(const char* dn, const char* text, int flags)
    {
        auto& s = sections.addEmpty();

        // TODO: compression

        SET_FOURCC_CDN(s.table_entry.FourCC);
        s.table_entry.dn_table_index = 0;
        s.table_entry.flags = flags;
        s.table_entry.reserved = 0;
        s.table_entry.offset = 0;
        s.table_entry.length = 0;

        s.domainName = dn;

        s.type = Section::SECT_BUFFER;
        s.buffer.writeString(text);
    }

    int MediaFileWriter::WriteAll()
    {
        // Write everything in one pass, no seeking

        // File header
        uint64_t offset = sizeof(header);
        header.sect_table = offset;

        // Main section table header
        // TODO: check `mst_header.flags`
        mst_header.num_sections = sections.getLength();
        mst_header.dn_table_entries = 0;
        mst_header.dn_table_length = 0;
        offset += sizeof(mst_header);

        iterate2 (i, sections)
        {
            auto& s = *i;

            //assert((s.table_entry.flags & SectionTableEntry::SECT_CP_ZLIB) == 0)
            // TODO: check that isset(domainName) == (4cc = '#CDN')
            // TODO: check that (domainName.getLength() < 0x10000)
            if (!s.domainName.isEmpty())
            {
                s.table_entry.dn_table_index = mst_header.dn_table_entries;
                mst_header.dn_table_entries++;
                mst_header.dn_table_length += 2 + s.domainName.getNumBytes() + 1;
            }
            else
                s.table_entry.dn_table_index = 0xffffffff;
        }

        offset += mst_header.dn_table_length;

        // Section Table Entries
        offset += mst_header.num_sections * sizeof(SectionTableEntry);

        iterate2 (i, sections)
        {
            auto& s = *i;

            s.table_entry.offset = offset;
            s.table_entry.length = s.buffer.getSize();
            offset += s.table_entry.length;
        }

        // ok, write everything now

        output->writeItems(&header, 1);
        output->writeItems(&mst_header, 1);

        iterate2 (i, sections)
        {
            auto& s = *i;

            if (!s.domainName.isEmpty())
            {
                output->write<uint16_t>(s.domainName.getNumBytes() + 1);
                output->writeString(s.domainName);
            }
        }

        iterate2 (i, sections)
        {
            auto& s = *i;

            output->writeItems(&s.table_entry, 1);
        }

        iterate2 (i, sections)
        {
            auto& s = *i;

            switch (s.type)
            {
                case Section::SECT_BUFFER:
                    //TODO: assert(s.table_entry.length == s.buffer.getSize());
                    output->write(s.buffer.getPtrUnsafe(), s.buffer.getSize());
            }
        }

        return 1;
    }
}
