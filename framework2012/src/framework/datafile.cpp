
#include "datafile.hpp"

namespace datafile
{
    SDStream::SDStream(SeekableInputStream* file) : file(file), isFileInitialized(false), isBlockInitialized(false), isSectionInitialized(false)
    {
    }

    SDStream::~SDStream()
    {
    }

    bool SDStream::FindSection()
    {
        if ( isSectionInitialized )
        {
            block->setPos( sectionEndOffset );
            isSectionInitialized = false;
        }

        if ( block->getPos() >= blockEndOffset )
            return false;

        sectionName = block->readString();

        printf( "[Section: '%s']\n", sectionName.c_str() );

        if ( sectionName.isEmpty() )
            return false;

        uint32_t sectionLength;

        if (!block->readItems(&sectionLength, 1))
            return false;

        sectionOffset = file->getPos();
        sectionEndOffset = sectionOffset + sectionLength;

        printf( "Section Ends At: %u\n", (unsigned) sectionEndOffset );

        isSectionInitialized = true;

        uint32_t numId1, numId2;

        if (GetSectionInfo(numId1, numId2))
        {
            if (numId1 == SDS_COMMON_ID || numId2 == SDS_ID_PURPOSE)
            {
                Reference<InputStream> section = OpenSection();
                streamPurpose = section->readString();
            }
        }

        return true;
    }

    void SDStream::GetSectionInfo(const char*& name)
    {
        name = sectionName;
    }

    bool SDStream::GetSectionInfo(uint32_t& numId1, uint32_t& numId2)
    {
        if (sectionName[0] != '#' || sectionName[9] != '#')
            return false;

        numId1 = (uint32_t)sectionName.subString(1, 8).toUnsigned(String::hexadecimal); 
        numId2 = (uint32_t)sectionName.subString(10, 8).toUnsigned(String::hexadecimal); 
        return true;
    }

    const char* SDStream::GetStreamPurpose()
    {
        while (streamPurpose.isEmpty())
        {
            if (!FindSection())
                return nullptr;
        }

        return streamPurpose;
    }

    bool SDStream::Init()
    {
        return InitializeBlock();
    }

    bool SDStream::InitializeBlock()
    {
        if ( isBlockInitialized )
            return true;

        if ( !isFileInitialized && !InitializeFile() )
            return false;

        char header[4];

        if (file->read(header, 4) != 4 )
            return false;

        printf( "Block Header: %c%c%c%c\n", header[0], header[1], header[2], header[3] );

        if (memcmp(header, "Blk-", 4) != 0)
            return false;

        uint32_t blockLength;

        if (!file->readItems(&blockLength, 1))
            return false;

        blockEndOffset = file->getPos() + blockLength;

        printf( "Block Ends At: %u\n", (unsigned) blockEndOffset );

        block = file->reference();
        isBlockInitialized = true;
        return true;
    }

    bool SDStream::InitializeFile()
    {
        if ( isFileInitialized )
            return true;

        char magic[4];

        if (file->read(magic, 4) != 4 || memcmp(magic, "DSf0", 4) != 0)
            return false;

        return true;
    }

    InputStream* SDStream::OpenSection()
    {
        return new SeekableInputStreamSegment( block->reference(), sectionOffset, sectionEndOffset - sectionOffset );
    }

    void SDStream::Rewind()
    {
        file->setPos(0);

        isFileInitialized = false;
        isBlockInitialized = false;
        isSectionInitialized = false;
    }

    SDSWriter::SDSWriter(OutputStream* output) : output(output), confFlushed(false)
    {
        output->write("DSf0", 4);
    }

    SDSWriter::~SDSWriter()
    {
        Flush();
    }

    void SDSWriter::FlushConf()
    {
        if (!confFlushed)
        {
            //output->write("Feat", 4);
            //WriteStr16("sds_v1_0");

            //output->write("Conf", 4);
        }
    }

    void SDSWriter::FlushSection()
    {
        if (sectionName.isEmpty())
            return;

        ZFW_ASSERT(sectionCache.getSize() < 0x7FFFFFFF)

        sectionCache.setPos(0);
        blockCache.writeString(sectionName);
        blockCache.write<uint32_t>((uint32_t) sectionCache.getSize());
        blockCache.copyFrom(&sectionCache);

        sectionCache.clear();
        sectionName.clear();
    }

    void SDSWriter::Flush()
    {
        FlushSection();

        if (blockCache.getSize() > 0)
        {
            ZFW_ASSERT(blockCache.getSize() < 0x7FFFFFFF)

            FlushConf();

            blockCache.setPos(0);

            output->write("Blk-", 4);
            output->write<uint32_t>((uint32_t) blockCache.getSize());
            output->copyFrom(&blockCache);

            blockCache.clear();
        }
    }

    OutputStream* SDSWriter::StartSection(uint32_t numId1, uint32_t numId2)
    {
        char tmp[35];
        snprintf(tmp, sizeof(tmp), "#%08X#%08X", numId1, numId2);

        return StartSection(tmp);
    }

    OutputStream* SDSWriter::StartSection(const char* name)
    {
        FlushSection();

        sectionName = name;

        return &sectionCache;
    }
}