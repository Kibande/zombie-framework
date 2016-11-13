#pragma once

#include "framework.hpp"

namespace datafile
{
    using namespace li;

    enum { SDS_COMMON_ID = 0xFF001000 };
    enum { SDS_ID_PURPOSE = 0x00000001 };

    class SDStream
    {
        Reference<SeekableInputStream> file, block;

        bool isFileInitialized, isBlockInitialized, isSectionInitialized;
        uint64_t blockEndOffset, sectionOffset, sectionEndOffset;
        String sectionName, streamPurpose;

        bool InitializeBlock();
        bool InitializeFile();

        public:
            SDStream(SeekableInputStream* file);
            ~SDStream();

            bool FindSection();
            void GetSectionInfo(const char*& name);
            bool GetSectionInfo(uint32_t& numId1, uint32_t& numId2);
            const char* GetStreamPurpose();
            bool Init();
            InputStream* OpenSection();
            void Rewind();
    };

    class SDSWriter
    {
        Reference<OutputStream> output;

        String sectionName;
        ArrayIOStream blockCache, sectionCache;

        bool confFlushed;

        void FlushConf();

        public:
            SDSWriter(OutputStream* output);
            ~SDSWriter();

            void Flush();
            void FlushSection();

            OutputStream* StartSection(uint32_t numId1, uint32_t numId2);
            OutputStream* StartSection(const char* name);

            //void StartSection(uint32_t numId1, uint32_t numId2);
            //void StartSection(const char* name);
    };
}