#pragma once

#include <framework/pixmap.hpp>
#include <framework/mediacodechandler.hpp>
#include <framework/engine.hpp>
#include <framework/utility/essentials.hpp>

#include <littl/Algorithm.hpp>

// FIXME: This shouldn't be here and LoadFromFile shouldn't be inline
#include <littl/Stream.hpp>

namespace zfw
{
    class Pixmap
    {
        public:
            template <class Pixmap_t>
            static void Initialize(Pixmap_t* pm, Int2 size, PixmapFormat_t format)
            {
                zombie_assert(size.x > 0);
                zombie_assert(size.y > 0);

                pm->info.size = size;
                pm->info.format = format;
            }

            template <class Pixmap_t>
            static void DropContents(Pixmap_t* pm)
            {
                pm->pixelData.clear();
                pm->pixelData.shrink_to_fit();
            }

            static size_t GetBytesPerLine(const PixmapInfo_t& info)
            {
                return li::align<4>(GetBytesPerPixel(info.format) * info.size.x);
            }

            static size_t GetBytesPerPixel(PixmapFormat_t format)
            {
                static const size_t bytesPerPixel[] = { 3, 3, 4 };

                return bytesPerPixel[static_cast<size_t>(format)];
            }

            static size_t GetBytesTotal(const PixmapInfo_t& info)
            {
                return GetBytesPerLine(info) * info.size.y;
            }

            template <class Pixmap_t>
            static const uint8_t* GetPixelDataForReading(const Pixmap_t* pm)
            {
                zombie_assert(pm->pixelData.capacity() >= pm->info.size.y * GetBytesPerLine(pm->info));

                return &pm->pixelData[0];
            }

            template <class Pixmap_t>
            static uint8_t* GetPixelDataForWriting(Pixmap_t* pm)
            {
                pm->pixelData.resize(pm->info.size.y * GetBytesPerLine(pm->info));

                return &pm->pixelData[0];
            }

            template <class Pixmap_t>
            static bool LoadFromFile(IEngine* sys, Pixmap_t* pm, const char* fileName)
            {
                // FIXME: Error description on error

                auto imch = sys->GetMediaCodecHandler(true);

                unique_ptr<InputStream> stream(sys->OpenInput(fileName));

                if (!stream)
                    return false;

                uint8_t signature[8];

                if (!stream->read(signature, li_lengthof(signature)) || !stream->setPos(0))
                    return false;

                auto decoder = imch->GetDecoderByFileSignature<IPixmapDecoder>(signature, li_lengthof(signature),
                    fileName, kCodecRequired);

                if (!decoder)
                    return false;

                return decoder->DecodePixmap(pm, stream.get(), fileName) == IDecoder::kOK;
            }
    };
}
