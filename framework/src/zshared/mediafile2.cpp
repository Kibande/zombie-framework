/*
    Copyright (c) 2013 Xeatheran Minexew

    This software is provided 'as-is', without any express or implied
    warranty. In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

// Project:             Zombie Framework (Minexew Games 2013)
// Compliant:           Zombie Media File Format Version 2.1

// Safety:              64-bit          SAFE
// Safety:              endianness      UNTESTED SAFE
// Safety:              multithread     SINGLETHREAD

// Copyright:           Original code by Xeatheran Minexew, 2013

#include <zshared/mediafile2.hpp>

#include <zshared/crc16_ccitt.hpp>

#include <littl/Array.hpp>
#include <littl/File.hpp>

#include <algorithm>
#include <memory>

// TODO: optimize seek forward
// TODO: mediafile corruption handling
// TODO: prevent corruption from opening section multiple times

// "assignment operator could not be generated"
#pragma warning(disable : 4512)

namespace
{
    using namespace li;
    using std::unique_ptr;
    using zshared::MediaFile;
    using zshared::IMetadataEntryListener;
    using zshared::ISectionEntryListener;

    class MediaFileImpl;

    // specification-defined
    static const uint16_t BITSTREAM_BINARY =            0x0A89;
    static const uint16_t BITSTREAM_VER =               0x0101;

    static const uint32_t SECT_MIN_SIZE =               256;
    static const uint32_t SECT_MAX_SIZE =               (1 << 30) - 1;

    static const uint32_t SECTION_DESC_MAX_LENGTH =     32767 - 31;
    static const uint32_t METADATA_MAX_ENTRY_LENGTH =   32767 - 31;

    static const uint32_t OFFSET_SECTION_MAP =          16;
    static const char* METADATA_SECTION_NAME =          "media.Metadata";

    const static uint8_t NO_COMPRESSION[4] =            {0x00, 0x00, 0x00, 0x00};
    const static uint8_t ZLIB_COMPRESSION[4] =          {'Z', 'L', 'I', 'B'};

    // implementation-defined
    static const uint32_t SECT_DEFAULT_SIZE =           1024;

    static const char zeros[32] = { 0 };

    struct zmfSpan_t {
        uint32_t    sect_first;
        uint32_t    sect_count;
    };

    struct zmfBlock_t {
        uint64_t    length;
        zmfSpan_t   first_span;
    };

    static_assert(sizeof(zmfSpan_t) == 8,       "Size of zmfSpan_t");
    static_assert(sizeof(zmfBlock_t) == 16,     "Size of zmfBlock_t");

    ////////////////////////////////////////////////////////////////////////

    static inline bool IsValid(const zmfSpan_t& span)
    {
        return span.sect_first != 0;
    }

    static inline bool CanJumpTo(const zmfSpan_t& from, const zmfSpan_t& to)
    {
        return to.sect_first != 0 && to.sect_count != 0 && to.sect_first != from.sect_first;
    }

    class BlockStream : public IOStream
    {
        protected:
            MediaFileImpl* mf;
            bool isReadOnly;
            OutputStream* descWriteback;
            uint64_t descOffset, data_length_offset;

            // block desc
            uint64_t    length;
            zmfSpan_t   first_span;

            bool        updateDesc;

            // stream control
            uint64_t    pos;

            zmfSpan_t   curr_span;
            uint64_t    curr_span_in_stream, curr_span_pos;

            bool AllocateSpan(zmfSpan_t* span, uint64_t sizeNeeded);
            bool JumpToPos();
            void SetCurrent(const zmfSpan_t* span);

        public:
            BlockStream(MediaFileImpl* mf, bool readOnly, OutputStream* descWriteback, uint64_t descOffset,
                    uint64_t data_length_offset = 0);
            virtual ~BlockStream();

            void ReadDesc(const zmfBlock_t* desc);
            bool ReadDesc(InputStream* file);
            bool WriteDesc(OutputStream* file);

            virtual bool finite() override { return true; }
            virtual bool seekable() override { return true; }

            virtual const char* getErrorDesc() { return nullptr; }

            virtual bool eof() override { return pos >= length; }
            virtual void flush();

            virtual uint64_t getPos() override { return pos; }
            virtual uint64_t getSize() override { return length; }
            virtual bool setPos(uint64_t pos) override;

            virtual size_t read(void* out, size_t length) override;
            virtual size_t write(const void* in, size_t length) override;
    };

    class MediaFileImpl : public MediaFile
    {
        protected:
            friend class BlockStream;

            unique_ptr<IOStream> file;
            bool isReadOnly;
            String errorDesc, tmpString;
            Array<char> data;

            // media file properties
            uint32_t bitstream;
            uint32_t sectorSize;

            // control blocks
            unique_ptr<BlockStream> section_map;
            unique_ptr<IOStream> metadata, reclaimed_sects;

            bool InitMediaFile(size_t sectorSize);
            bool OpenMediaFile();

            void ErrCorrupted() { errorDesc = "The media file is corrupted."; }
            void ErrLimit() { errorDesc = "A format limit was exceeded."; }
            void ErrReadOnly() { errorDesc = "The file is read-only."; }
            void ErrWrite() { errorDesc = "Failed to write data. (disk full?)"; }

            // metadata
            int OpenMetadataSection(bool readOnly);
            int ReadMetadataEntryHeader(uint16_t& data_len, uint16_t& data_len_padded, uint16_t& key_crc16);
            bool ReadMetadataEntryData(uint16_t data_len, uint16_t data_len_padded,
                    char*& key, char*& value);

            // section map
            int ReadSectionEntryHeader(uint16_t& desc_len, uint16_t& desc_len_padded, uint16_t& name_crc16,
                    uint8_t compression[4], uint64_t& data_length, zmfBlock_t& block);
            bool ReadSectionEntryData(uint16_t desc_len, uint16_t desc_len_padded,
                char*& name, char*& desc);

            IOStream* CreateSectionPriv(const char* name);
            IOStream* OpenSectionPriv(const char* name);

        public:
            MediaFileImpl();
            virtual ~MediaFileImpl();

            virtual bool Open(const char* fileName, bool readOnly, bool canCreate) override;
            virtual bool Open(zshared::IOpenFile* iof, bool readOnly, bool canCreate) override;
            virtual bool Open(unique_ptr<li::IOStream>&& ios, bool readOnly) override;
            virtual void Close() override;

            virtual const char* GetMetadata(const char* key) override;
            virtual int IterateMetadata(IMetadataEntryListener* listener) override;
            virtual bool SetMetadata(const char* key, const char* value) override;

            virtual IOStream* CreateSection(const char* name) override { return CreateSectionPriv(name); }
            virtual int IterateSections(ISectionEntryListener* listener) override;
            virtual IOStream* OpenOrCreateSection(const char* name) override;
            virtual IOStream* OpenSection(const char* name) override { return OpenSectionPriv(name); }

            virtual const char* GetBitstreamType() override;
            virtual const char* GetErrorDesc() override { return errorDesc.c_str(); }
            virtual uint64_t GetFileSize() override { return file->getSize(); }
            virtual uint32_t GetSectorSize() override { return sectorSize; }
            virtual bool SetSectorSize(uint32_t sectorSize) override;
    };

    BlockStream::BlockStream(MediaFileImpl* mf, bool readOnly,
            OutputStream* descWriteback, uint64_t descOffset, uint64_t data_length_offset)
            : mf(mf), isReadOnly(readOnly),
            descWriteback(descWriteback), descOffset(descOffset), data_length_offset(data_length_offset)
    {
        length = 0;
        first_span.sect_first = 0;
        first_span.sect_count = 0;

        updateDesc = false;

        setPos(0);
    }

    BlockStream::~BlockStream()
    {
        flush();
    }

    bool BlockStream::AllocateSpan(zmfSpan_t* span, uint64_t sizeNeeded)
    {
        const uint64_t fileSize = mf->file->getSize();

        // always round up to next sector
        const uint64_t sect_first = (fileSize + mf->sectorSize - 1) / mf->sectorSize;
        const uint64_t sect_count = (sizeNeeded + mf->sectorSize - 1) / mf->sectorSize;

        if (sect_first > UINT32_MAX || sect_count > UINT32_MAX)
            return false;

        span->sect_first = (uint32_t) sect_first;
        span->sect_count = (uint32_t) sect_count;

        mf->file->setPos(span->sect_first * mf->sectorSize + mf->sectorSize - 1);
        mf->file->writeLE<uint8_t>(0);

        return true;
    }

    void BlockStream::flush()
    {
        // if block length has changed, we need to update its description
        if (updateDesc)
        {
            // section blocks have their length stored in two places
            if (data_length_offset != 0)
                descWriteback->setPos(data_length_offset) && descWriteback->writeLE(length);

            descWriteback->setPos(descOffset) && WriteDesc(descWriteback);
        }
    }

    bool BlockStream::JumpToPos()
    {
        // this is highly unoptimal right now (and possibly broken)
        // we're always starting from the block beginning and going span-by-span

        SetCurrent(&first_span);
        curr_span_in_stream = 0;

        if (pos > length)
            return false;

        while (pos != curr_span_in_stream)
        {
            // this should never happen
            if (curr_span_in_stream > length)
                return false;

            const uint64_t maxPossibleBytesInSpan = curr_span.sect_count * mf->sectorSize;

            // does this span have an 8-byte tag at its end?
            // we can find out by checking whether it contains the rest of the block
            const uint64_t bytesInSpan = maxPossibleBytesInSpan
                - (curr_span_in_stream + maxPossibleBytesInSpan < length ? 8 : 0);

            // is the 'pos' we're looking for within this span?
            if (pos <= curr_span_in_stream + bytesInSpan)
            {
                curr_span_pos = pos - curr_span_in_stream;
                break;
            }

            zmfSpan_t next_span;

            // read the tag for next span
            if (!mf->file->setPos(curr_span.sect_first * mf->sectorSize + bytesInSpan)
                || !mf->file->readLE<uint32_t>(&next_span.sect_first)
                || !mf->file->readLE<uint32_t>(&next_span.sect_count))
                return false;

            SetCurrent(&next_span);
            curr_span_in_stream += bytesInSpan;
        }

        return true;
    }

    size_t BlockStream::read(void* out, size_t count)
    {
        // clamp read length
        if (count > pos + length)
            count = (size_t)(length - pos);

        if (count == 0)
            return 0;

        uint8_t* buffer_out = reinterpret_cast<uint8_t*>(out);

        size_t read_total = 0;

        for (;;)
        {
            // start by checking whether we currently are within any span
            // and seek if not
            if (!IsValid(curr_span) && !JumpToPos())
                return read_total;

            // how many bytes from current pos till the end of this span?
            const uint64_t maxRemainingBytesInSpan = curr_span.sect_count * mf->sectorSize - curr_span_pos;

            // are there any more spans for this block? if so, last 8 bytes are used for chaining
            const int64_t remainingBytesInSpan = maxRemainingBytesInSpan
                - (curr_span_in_stream + curr_span.sect_count * mf->sectorSize < length ? 8 : 0);

            if (remainingBytesInSpan > 0 && (unsigned int) remainingBytesInSpan >= count)
            {
                // all the remaining data to be read is within the current span

                mf->file->setPos(curr_span.sect_first * mf->sectorSize + curr_span_pos);
                auto read = mf->file->read(buffer_out, count);

                curr_span_pos += read;
                pos += read;
                read_total += read;

                return read_total;
            }
            else
            {
                // there are more data to be read beyond this span
                // start by reading the rest of this one, then jump for more

                // are there ANY more data in this span? if so, read it
                if (remainingBytesInSpan > 0)
                {
                    mf->file->setPos(curr_span.sect_first * mf->sectorSize + curr_span_pos);
                    auto read = mf->file->read(buffer_out, (size_t) remainingBytesInSpan);

                    curr_span_pos += read;
                    pos += read;
                    read_total += read;

                    if (read < (unsigned int) remainingBytesInSpan)
                        return read_total;

                    buffer_out += read;
                    count -= read;
                }
                else
                    mf->file->setPos(curr_span.sect_first * mf->sectorSize + curr_span_pos + remainingBytesInSpan);

                zmfSpan_t next_span;

                // read the tag for next span
                if (!mf->file->readLE<uint32_t>(&next_span.sect_first)
                    || !mf->file->readLE<uint32_t>(&next_span.sect_count))
                    return read_total;

                if (!CanJumpTo(curr_span, next_span))
                    return read_total;

                // loop for next span
                curr_span_in_stream += curr_span.sect_count * mf->sectorSize - 8;
                SetCurrent(&next_span);
            }
        }
    }

    void BlockStream::ReadDesc(const zmfBlock_t* desc)
    {
        length = desc->length;
        first_span = desc->first_span;

        pos = 0;
        curr_span_in_stream = 0;
    }

    bool BlockStream::ReadDesc(InputStream* file)
    {
        if (!file->readLE<uint64_t>(&length)
            || !file->readLE<uint32_t>(&first_span.sect_first)
            || !file->readLE<uint32_t>(&first_span.sect_count))
            return false;

        pos = 0;
        curr_span_in_stream = 0;

        return true;
    }

    void BlockStream::SetCurrent(const zmfSpan_t* span)
    {
        curr_span.sect_first = span->sect_first;
        curr_span.sect_count = span->sect_count;
        curr_span_pos = 0;
    }

    bool BlockStream::setPos(uint64_t pos)
    {
        if (pos > length)
            return false;

        // don't do any actual seeking; just change the internal pointer
        // and invalidate current span
        this->pos = pos;
        curr_span.sect_first = 0;

        return true;
    }

    size_t BlockStream::write(const void* in, size_t count)
    {
        if (isReadOnly || count == 0)
            return 0;

        const uint8_t* buffer_in = reinterpret_cast<const uint8_t*>(in);

        size_t written_total = 0;

        for (;;)
        {
            // start by checking whether we currently are within any span

            if (!IsValid(curr_span))
            {
                if (length == 0)
                {
                    // the block is empty; allocate initial span

                    if (!AllocateSpan(&first_span, count))
                        return written_total;

                    SetCurrent(&first_span);
                    curr_span_in_stream = 0;

                    // we'll have to update first_span in block desc
                    updateDesc = true;
                }
                else
                    // seek; will fail if pos > length
                    if (!JumpToPos())
                        return written_total;
            }

            // we now have a valid span to write into
            // if the span has a chaining tag on its end, the usable capacity will be 8 bytes less
            // in that case we might have to fetch or allocate more spans

            // how many bytes from current pos till the end of this span?
            const uint64_t maxRemainingBytesInSpan = curr_span.sect_count * mf->sectorSize - curr_span_pos;

            const uint64_t newLength = std::max<uint64_t>(length, pos + count);

            // are there/will there be any more spans beyond this one? if so, last 8 bytes are used for chaining
            const int64_t remainingBytesInSpan = maxRemainingBytesInSpan
                - (curr_span_in_stream + curr_span.sect_count * mf->sectorSize < newLength ? 8 : 0);

            if (remainingBytesInSpan > 0 && (unsigned int) remainingBytesInSpan >= count)
            {
                // all the remaining data to be written will within the current span

                mf->file->setPos(curr_span.sect_first * mf->sectorSize + curr_span_pos);
                auto written = mf->file->write(buffer_in, count);

                curr_span_pos += written;
                pos += written;
                written_total += written;

                if (pos > length)
                {
                    length = pos;
                    updateDesc = true;
                }

                return written_total;
            }
            else
            {
                // we'll have to go beyond this span
                // if there is a chaining tag already, we'll take the jump,
                // otherwise we have to allocate a new span

                const int64_t affectedBytesInSpan = maxRemainingBytesInSpan - 8;

                // will be > 8 if the block already continues beyond this span;
                // in that case the last 8 bytes contain a chaining tag which will be used

                // if 1..8, the current span contains data within its last 8 bytes
                // which will have to be moved
                const int64_t tailLength = length - (curr_span_in_stream + curr_span.sect_count * mf->sectorSize) + 8;

                // will ANY more data fit in this span?
                if (affectedBytesInSpan > 0)
                {
                    mf->file->setPos(curr_span.sect_first * mf->sectorSize + curr_span_pos);
                    auto written = mf->file->write(buffer_in, (size_t) affectedBytesInSpan);

                    curr_span_pos += written;
                    pos += written;
                    written_total += written;

                    if (pos > length)
                    {
                        length = pos;
                        updateDesc = true;
                    }

                    if (written < (unsigned int) affectedBytesInSpan)
                        return written_total;

                    buffer_in += written;
                    count -= written;
                }
                else
                    mf->file->setPos(curr_span.sect_first * mf->sectorSize + curr_span_pos + affectedBytesInSpan);

                if (tailLength > 8)
                {
                    zmfSpan_t next_span;

                    // read the tag for next span
                    if (!mf->file->readLE<uint32_t>(&next_span.sect_first)
                        || !mf->file->readLE<uint32_t>(&next_span.sect_count))
                        return written_total;

                    if (!CanJumpTo(curr_span, next_span))
                        return false;

                    // loop for next span
                    curr_span_in_stream += curr_span.sect_count * mf->sectorSize - 8;
                    SetCurrent(&next_span);
                }
                else
                {
                    // current data ends within this span; we might have to copy a couple of byte
                    // if they are to be rewritten with the chaining tag

                    uint8_t tail[8];

                    if (tailLength > 0)
                        if (mf->file->read(tail, (size_t) tailLength) != (size_t) tailLength)
                            return written_total;

                    zmfSpan_t new_span;

                    // allocate a new span
                    if (!AllocateSpan(&new_span, count))
                        return written_total;

                    // write chaining tag
                    if (!mf->file->setPos((curr_span.sect_first + curr_span.sect_count) * mf->sectorSize - 8)
                        || !mf->file->writeLE<uint32_t>(new_span.sect_first)
                        || !mf->file->writeLE<uint32_t>(new_span.sect_count))
                        return written_total;

                    // jump to new span
                    curr_span_in_stream += curr_span.sect_count * mf->sectorSize - 8;
                    SetCurrent(&new_span);

                    // flush saved data (if any) and loop for the newly allocated span
                    if (tailLength > 0)
                    {
                        if (!mf->file->setPos(curr_span.sect_first * mf->sectorSize)
                            || mf->file->write(tail, (size_t) tailLength) != (size_t) tailLength)
                            return written_total;

                        curr_span_pos = tailLength;
                    }
                }
            }
        }
    }

    bool BlockStream::WriteDesc(OutputStream* file)
    {
        if (!file->writeLE<uint64_t>(length)
            || !file->writeLE<uint32_t>(first_span.sect_first)
            || !file->writeLE<uint32_t>(first_span.sect_count))
            return mf->ErrWrite(), false;

        return true;
    }

    MediaFileImpl::MediaFileImpl()
    {
        sectorSize = 1024;
    }

    MediaFileImpl::~MediaFileImpl()
    {
        Close();
    }

    void MediaFileImpl::Close()
    {
        metadata.reset();
        reclaimed_sects.reset();

        section_map.reset();

        file.reset();
    }

    IOStream* MediaFileImpl::CreateSectionPriv(const char* name)
    {
        // TODO: What if we detect corruption?

        if (isReadOnly)
            return ErrReadOnly(), nullptr;

        String new_desc = (String) "name=" + name;

        const uint16_t new_name_crc16 = zshared::crc16_ccitt((const uint8_t*) name, strlen(name));
        const size_t new_desc_len = new_desc.getNumBytes();
        const size_t new_desc_len_padded = ((new_desc_len + 31) & ~31);

        if (new_desc_len > SECTION_DESC_MAX_LENGTH)
            return ErrLimit(), nullptr;

        for (section_map->rewind(); !section_map->eof(); )
        {
            uint16_t desc_len, desc_len_padded, name_crc16;
            uint8_t compression[4];
            uint64_t data_length;
            zmfBlock_t block;

            auto startPos = section_map->getPos();
            int valid = ReadSectionEntryHeader(desc_len, desc_len_padded, name_crc16, compression, data_length, block);

            if (valid < 0)
                return ErrCorrupted(), nullptr;

            if (!valid)
            {
                if (desc_len_padded == new_desc_len_padded)
                {
                    section_map->setPos(startPos);
                    break;
                }

                if (!section_map->seek(desc_len_padded))
                    return ErrCorrupted(), nullptr;

                continue;
            }

            char *rd_key, *rd_value;

            if (!ReadSectionEntryData(desc_len, desc_len_padded, rd_key, rd_value))
                return ErrCorrupted(), nullptr;
        }

        if (!section_map->writeLE<uint16_t>((uint16_t) new_desc_len)
                || !section_map->writeLE<uint16_t>(new_name_crc16)
                || section_map->write(NO_COMPRESSION, 4) != 4
                || !section_map->writeLE<uint64_t>(0))
            return ErrWrite(), nullptr;

        uint64_t pos = section_map->getPos();
        unique_ptr<BlockStream> stream(new BlockStream(this, false, section_map.get(), pos, pos - 8));

        if (!stream->WriteDesc(section_map.get()))
            return nullptr;

        section_map->write(new_desc, new_desc.getNumBytes());
        section_map->write(zeros, new_desc_len_padded - new_desc_len);

        return stream.release();
    }

    const char* MediaFileImpl::GetBitstreamType()
    {
        if ((bitstream & 0xFFFF) == BITSTREAM_BINARY)
            tmpString = (String) "BINARY-" + String::formatInt(bitstream >> 16, -1, String::hexadecimal) + "h";
        else
            tmpString = "UNKNOWN-???";

        return tmpString;
    }

    const char* MediaFileImpl::GetMetadata(const char* key)
    {
        class Iterator : public IMetadataEntryListener
        {
            const char* key;
            String& value;

            public:
                Iterator(const char* key, String& value) : key(key), value(value)
                {
                }

                virtual int OnMetadataEntry(uint16_t key_crc16, const char* key, const char* value) override
                {
                    if (strcmp(key, this->key) == 0)
                    {
                        this->value = value;
                        return -1;
                    }
                    else
                        return 1;
                }
        };

        tmpString.clear();

        Iterator iterator(key, tmpString);
        IterateMetadata(&iterator);

        return tmpString;
    }

    bool MediaFileImpl::InitMediaFile(size_t sectorSize)
    {
        bitstream = (BITSTREAM_VER << 16) | BITSTREAM_BINARY;

        if (!file->rewind()
            // signature
            || !file->write("ZMF2", 4)
            // bitstream
            || !file->writeLE<uint32_t>(bitstream)
            // sect_size
            || !file->writeLE<uint32_t>((uint32_t) sectorSize)
            // ctlsect
            || !file->writeLE<uint32_t>(0)

            || !section_map->WriteDesc(file.get()))
        {
            errorDesc = "Failed to write file header.";
            return false;
        }

        return true;
    }

    int MediaFileImpl::IterateMetadata(IMetadataEntryListener* listener)
    {
        char *rd_key, *rd_value;

        int rc = OpenMetadataSection(true);

        if (rc <= 0)
            return rc;

        for (metadata->rewind(); !metadata->eof(); )
        {
            uint16_t data_len, data_len_padded, key_crc16;

            int valid = ReadMetadataEntryHeader(data_len, data_len_padded, key_crc16);

            if (valid < 0)
                return -1;

            if (!valid)
            {
                if (!metadata->seek(data_len_padded))
                    return -1;

                continue;
            }

            if (!ReadMetadataEntryData(data_len, data_len_padded, rd_key, rd_value))
                return -1;

            int ret = listener->OnMetadataEntry(key_crc16, rd_key, rd_value);

            if (ret <= 0)
                return ret;
        }

        return 0;
    }

    int MediaFileImpl::IterateSections(ISectionEntryListener* listener)
    {
        char *rd_name, *rd_desc;

        for (section_map->rewind(); !section_map->eof(); )
        {
            uint16_t desc_len, desc_len_padded, name_crc16;
            uint8_t compression[4];
            uint64_t data_length;
            zmfBlock_t block;

            int valid = ReadSectionEntryHeader(desc_len, desc_len_padded, name_crc16, compression, data_length, block);

            if (valid < 0)
                return false;

            if (!valid)
            {
                if (!section_map->seek(desc_len_padded))
                    return false;

                continue;
            }

            if (!ReadSectionEntryData(desc_len, desc_len_padded, rd_name, rd_desc))
                return false;

            int ret = listener->OnSectionEntry(name_crc16, rd_name, rd_desc, compression, data_length, block.length);

            if (ret <= 0)
                return ret;
        }

        return 0;
    }

    bool MediaFileImpl::Open(const char* fileName, bool readOnly, bool canCreate)
    {
        class OpenFilePriv : public zshared::IOpenFile
        {
            const char* fileName;

            public:
                OpenFilePriv(const char* fileName) : fileName(fileName)
                {
                }

                virtual unique_ptr<li::IOStream> OpenFile(bool readOnly, bool create) override
                {
                    if (create)
                        return unique_ptr<li::IOStream>(File::open(fileName, "wb+"));
                    else if (!readOnly)
                        return unique_ptr<li::IOStream>(File::open(fileName, "rb+"));
                    else
                        return unique_ptr<li::IOStream>(File::open(fileName, "rb"));
                }
        };

        OpenFilePriv of(fileName);
        return Open(&of, readOnly, canCreate);
    }

    bool MediaFileImpl::Open(zshared::IOpenFile* iof, bool readOnly, bool canCreate)
    {
        Close();
        isReadOnly = readOnly;

        file = iof->OpenFile(readOnly, false);

        if (file == nullptr)
        {
            if (!readOnly && canCreate)
            {
                file = iof->OpenFile(readOnly, true);

                if (file == nullptr)
                {
                    errorDesc = "Failed to open or create file.";
                    return false;
                }

                section_map.reset(new BlockStream(this, readOnly, file.get(), OFFSET_SECTION_MAP));
                metadata =          nullptr;
                reclaimed_sects =   nullptr;

                if (!InitMediaFile(sectorSize))
                    return false;
            }
            else
            {
                errorDesc = "Failed to open file.";
                return false;
            }
        }
        else
        {
            if (!OpenMediaFile())
                return false;
        }

        return true;
    }

    bool MediaFileImpl::Open(unique_ptr<IOStream>&& ios, bool readOnly)
    {
        Close();
        isReadOnly = readOnly;

        file = std::move(ios);

        if (file->getSize() < 16)
        {
            if (!readOnly)
            {
                section_map.reset(new BlockStream(this, readOnly, file.get(), OFFSET_SECTION_MAP));
                metadata =          nullptr;
                reclaimed_sects =   nullptr;

                if (!InitMediaFile(sectorSize))
                    return false;
            }
            else
            {
                errorDesc = "Failed to open file.";
                return false;
            }
        }
        else
        {
            if (!OpenMediaFile())
                return false;
        }

        return true;
    }

    bool MediaFileImpl::OpenMediaFile()
    {
        char magic[4];
        uint32_t sect_size;
        uint32_t ctlsect;

        if (!file->read(magic, 4) || memcmp(magic, "ZMF2", 4) != 0
                || !file->readLE<uint32_t>(&bitstream))
        {
            errorDesc = "Not a Zombie media file.";
            return false;
        }

        if (bitstream != ((BITSTREAM_VER << 16) | BITSTREAM_BINARY))
        {
            errorDesc = "Unrecognized bitstream type/version.";
            return false;
        }

        section_map.reset(new BlockStream(this, isReadOnly, file.get(), OFFSET_SECTION_MAP));
        metadata =          nullptr;
        reclaimed_sects =   nullptr;

        if (!file->readLE<uint32_t>(&sect_size)
            || !file->readLE<uint32_t>(&ctlsect)
            || !section_map->ReadDesc(file.get()))
        {
            errorDesc = "Failed to read file header.";
            return false;
        }

        // TODO: Verify sect_size, ctlsect
        sectorSize = sect_size;

        return true;
    }

    int MediaFileImpl::OpenMetadataSection(bool readOnly)
    {
        if (metadata != nullptr)
            return 1;

        metadata.reset(OpenSectionPriv(METADATA_SECTION_NAME));

        if (metadata != nullptr)
            return 1;

        if (readOnly)
            return 0;

        metadata.reset(CreateSectionPriv(METADATA_SECTION_NAME));

        return (metadata != nullptr) ? 1 : -1;
    }

    IOStream* MediaFileImpl::OpenOrCreateSection(const char* name)
    {
        IOStream* section = OpenSectionPriv(name);

        if (section == nullptr)
            section = CreateSectionPriv(name);

        return section;
    }

    IOStream* MediaFileImpl::OpenSectionPriv(const char* name)
    {
        char *rd_name, *rd_desc;

        const uint16_t new_name_crc16 = zshared::crc16_ccitt((const uint8_t*) name, strlen(name));

        for (section_map->rewind(); !section_map->eof(); )
        {
            uint16_t desc_len, desc_len_padded, name_crc16;
            uint8_t compression[4];
            uint64_t data_length;
            zmfBlock_t block;

            int valid = ReadSectionEntryHeader(desc_len, desc_len_padded, name_crc16, compression, data_length, block);

            if (valid < 0)
                return nullptr;

            if (!valid)
            {
                if (!section_map->seek(desc_len_padded))
                    return nullptr;

                continue;
            }

            if (!ReadSectionEntryData(desc_len, desc_len_padded, rd_name, rd_desc))
                return nullptr;

            if (name_crc16 == new_name_crc16 && strcmp(rd_name, name) == 0)
            {
                auto pos = section_map->getPos() - desc_len_padded - 32;

                unique_ptr<BlockStream> stream(new BlockStream(this, isReadOnly, section_map.get(), pos + 16, pos + 8));
                stream->ReadDesc(&block);
                return stream.release();
            }
        }

        return nullptr;
    }

    bool MediaFileImpl::ReadMetadataEntryData(uint16_t data_len, uint16_t data_len_padded,
            char*& key, char*& value)
    {
        data.resize(data_len + 1, true);

        if (metadata->read(data.c_array(), data_len) != data_len)
            return false;

        if (!metadata->seek(data_len_padded - data_len))
            return false;

        data[data_len] = 0;

        char* delim = strchr(data.c_array(), '=');

        if (delim == nullptr)
            return false;

        *delim = 0;

        key = data.c_array();
        value = delim + 1;
        return true;
    }

    int MediaFileImpl::ReadMetadataEntryHeader(uint16_t& data_len, uint16_t& data_len_padded, uint16_t& key_crc16)
    {
        if (!metadata->readLE<uint16_t>(&data_len)
            || !metadata->readLE<uint16_t>(&key_crc16))
            return -1;

        // clear highest bit, round up to 32 bytes
        data_len_padded = ((data_len + 31) & ~0x801F);

        if (data_len & 0x8000)
            return 0;
        else
            return 1;
    }

    bool MediaFileImpl::ReadSectionEntryData(uint16_t desc_len, uint16_t desc_len_padded,
            char*& name, char*& desc)
    {
        // make sure we won't need to reallocate data after doing strchr
        data.resize(desc_len * 2 + 1, true);

        if (section_map->read(data.c_array(), desc_len) != desc_len)
            return false;

        if (!section_map->seek(desc_len_padded - desc_len))
            return false;

        data[desc_len] = 0;

        char* delim = strchr(data.c_array(), '=');

        if (delim == nullptr)
            return false;

        delim++;
        char* delim2 = strchr(delim, ',');

        if (delim2 == nullptr)
            delim2 = delim + strlen(delim);

        data.resize(desc_len + 1 + delim2 - delim + 1, true);

        memcpy(&data[desc_len + 1], delim, delim2 - delim);
        data[desc_len + 1 + delim2 - delim] = 0;

        name = &data[desc_len + 1];
        desc = data.c_array();
        return true;
    }

    int MediaFileImpl::ReadSectionEntryHeader(uint16_t& desc_len, uint16_t& desc_len_padded, uint16_t& name_crc16,
            uint8_t compression[4], uint64_t& data_length, zmfBlock_t& block)
    {
        if (!section_map->readLE<uint16_t>(&desc_len)
            || !section_map->readLE<uint16_t>(&name_crc16)
            || section_map->read(compression, 4) != 4
            || !section_map->readLE<uint64_t>(&data_length)
            || !section_map->readLE<uint64_t>(&block.length)
            || !section_map->readLE<uint32_t>(&block.first_span.sect_first)
            || !section_map->readLE<uint32_t>(&block.first_span.sect_count))
            return -1;

        // clear highest bit, round up to 32 bytes
        desc_len_padded = ((desc_len + 31) & ~0x801F);

        if (desc_len & 0x8000)
            return 0;
        else
            return 1;
    }

    bool MediaFileImpl::SetMetadata(const char* key, const char* value)
    {
        // TODO: What if we detect corruption?

        if (isReadOnly)
            return false;

        const uint16_t new_key_crc16 = zshared::crc16_ccitt((const uint8_t*) key, strlen(key));
        const size_t new_data_len = strlen(key) + 1 + strlen(value);
        const size_t new_data_len_padded = ((new_data_len + 31) & ~31);

        if (new_data_len > METADATA_MAX_ENTRY_LENGTH)
            return false;

        int rc = OpenMetadataSection(false);

        if (rc <= 0)
            return false;

        for (metadata->rewind(); !metadata->eof(); )
        {
            uint16_t data_len, data_len_padded, key_crc16;
            auto startPos = metadata->getPos();

            int valid = ReadMetadataEntryHeader(data_len, data_len_padded, key_crc16);

            if (valid < 0)
                return false;

            if (!valid)
            {
                if (!metadata->seek(data_len_padded))
                    return false;

                continue;
            }

            char *rd_key, *rd_value;

            if (!ReadMetadataEntryData(data_len, data_len_padded, rd_key, rd_value))
                return false;

            if (key_crc16 == new_key_crc16 && strcmp(key, rd_key) == 0)
            {
                metadata->setPos(startPos);

                if (new_data_len_padded == data_len_padded)
                    break;
                else
                {
                    metadata->writeLE<uint16_t>(0x8000 | data_len);
                    metadata->seek(2 + data_len_padded);

                    while (!metadata->eof())
                    {
                        if (!ReadMetadataEntryHeader(data_len, data_len_padded, key_crc16))
                            return false;

                        if (!metadata->seek(data_len_padded))
                            return false;
                    }
                }
            }
        }

        metadata->writeLE<uint16_t>((uint16_t) new_data_len);
        metadata->writeLE<uint16_t>(new_key_crc16);

        metadata->write(key, strlen(key));
        metadata->write(char('='));
        metadata->write(value, strlen(value));

        metadata->write(zeros, new_data_len_padded - new_data_len);

        return true;
    }

    bool MediaFileImpl::SetSectorSize(uint32_t sectorSize)
    {
        if (sectorSize < SECT_MIN_SIZE || sectorSize > SECT_MAX_SIZE)
        {
            errorDesc = (String) "Sector size must be between " + SECT_MIN_SIZE + " and " + SECT_MAX_SIZE + " bytes, inclusive.";
            return false;
        }

        if ((sectorSize & (sectorSize - 1)) != 0)
        {
            errorDesc = (String) "Sector size must be a power of 2.";
            return false;
        }

        this->sectorSize = sectorSize;
        return true;
    }
}

namespace zshared
{
    MediaFile* MediaFile::Create()
    {
        return new MediaFileImpl();
    }
}
