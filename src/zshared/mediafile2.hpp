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

#pragma once

#include <littl/Stream.hpp>

#include <memory>

namespace zshared
{
    class IMetadataEntryListener
    {
        public:
            // > 0:     continue
            // <= 0:    return
            virtual int OnMetadataEntry(uint16_t key_crc16, const char* key, const char* value) = 0;
    };

    class IOpenFile
    {
        public:
            virtual std::unique_ptr<li::IOStream> OpenFile(bool readOnly, bool create) = 0;
    };

    class ISectionEntryListener
    {
        public:
            // > 0:     continue
            // <= 0:    return
            virtual int OnSectionEntry(uint16_t name_crc16, const char* name, const char* desc,
                    const uint8_t compression[4], uint64_t data_length, uint64_t compressed_length) = 0;
    };

    class MediaFile
    {
        public:
            static MediaFile* Create();
            virtual ~MediaFile() {}

            virtual bool Open(const char* fileName, bool readOnly, bool canCreate) = 0;
            virtual bool Open(IOpenFile* iof, bool readOnly, bool canCreate) = 0;
            virtual bool Open(std::unique_ptr<li::IOStream>&& ios, bool readOnly) = 0;
            virtual void Close() = 0;

            virtual const char* GetMetadata(const char* key) = 0;
            virtual int IterateMetadata(IMetadataEntryListener* listener) = 0;
            virtual bool SetMetadata(const char* key, const char* value) = 0;

            virtual li::IOStream* CreateSection(const char* name) = 0;
            virtual int IterateSections(ISectionEntryListener* listener) = 0;
            virtual li::IOStream* OpenOrCreateSection(const char* name) = 0;
            virtual li::IOStream* OpenSection(const char* name) = 0;

            virtual const char* GetBitstreamType() = 0;
            virtual const char* GetErrorDesc() = 0;
            virtual uint64_t GetFileSize() = 0;
            virtual uint32_t GetSectorSize() = 0;
            virtual bool SetSectorSize(uint32_t sectorSize) = 0;
    };
}
