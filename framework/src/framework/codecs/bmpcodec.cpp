
#include <framework/errorbuffer.hpp>
#include <framework/mediacodechandler.hpp>
#include <framework/utility/essentials.hpp>
#include <framework/utility/pixmap.hpp>

#include <framework/system.hpp>

#include <littl/Stream.hpp>

namespace zfw
{
    enum { kBmpHeaderSize = 14 };
    enum { kDibHeaderSize = 40 };
    enum { kPrintResolution = 2835 };

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    class BmpEncoder : public IPixmapEncoder
    {
        public:
            virtual bool GetFileTypes(const char*** fileTypes_out, size_t* numFileTypes_out) override;
            virtual const char* GetName() override { return "zfw::BmpEncoder"; }

            virtual IEncoder::EncodingResult_t EncodePixmap(const Pixmap_t* pm, OutputStream* stream,
                    const char* fileNameOrNull) override;
    };

    // ====================================================================== //
    //  class BmpEncoder
    // ====================================================================== //

    IPixmapEncoder* p_CreateBmpEncoder(ISystem* sys)
    {
        return new BmpEncoder();
    }

    IEncoder::EncodingResult_t BmpEncoder::EncodePixmap(const Pixmap_t* pm, OutputStream* stream,
            const char* fileNameOrNull)
    {
        zombie_assert(pm->info.size.x > 0);
        zombie_assert(pm->info.size.y > 0);

        if (pm->info.format != PixmapFormat_t::BGR8)
        {
            ErrorBuffer::SetError3(errRequestedEncodingNotSupported, 2,
                    "desc", "Unsupported BMP format for encoding. Supported formats are: BGR8",
                    "fileName", fileNameOrNull);
            return kError;
        }

        const size_t lineLength = Pixmap::GetBytesPerLine(pm->info);
        const size_t fileSize = kBmpHeaderSize + kDibHeaderSize + lineLength * pm->info.size.y;

        // BMP header
        stream->write("BM");
        stream->writeLE<uint32_t>(fileSize);
        stream->writeLE<uint16_t>(0);
        stream->writeLE<uint16_t>(0);
        stream->writeLE<uint32_t>(kBmpHeaderSize + kDibHeaderSize);

        // DIB header
        stream->writeLE<uint32_t>(kDibHeaderSize);
        stream->writeLE<uint32_t>(pm->info.size.x);
        stream->writeLE<uint32_t>(pm->info.size.y);
        stream->writeLE<uint16_t>(1);               // 1 plane
        stream->writeLE<uint16_t>(24);              // 24-bit
        stream->writeLE<uint32_t>(0);               // no compression
        stream->writeLE<uint32_t>(lineLength * pm->info.size.y);
        stream->writeLE<uint32_t>(kPrintResolution);
        stream->writeLE<uint32_t>(kPrintResolution);
        stream->writeLE<uint32_t>(0);               // no colors in palette
        stream->writeLE<uint32_t>(0);

#ifdef ZOMBIE_CTR
        uint8_t* data = (uint8_t*) malloc(lineLength * pm->info.size.y);
        //ntile::g_sys->Printf(kLogInfo, "Converting...");
        for (int y = pm->info.size.y - 1; y >= 0; y--)
        {
            const uint8_t* line = Pixmap::GetPixelDataForReading(pm) + y * lineLength;

            memcpy(data + (pm->info.size.y - y - 1) * lineLength, line, lineLength);
        }
        //ntile::g_sys->Printf(kLogInfo, "Writing %u bytes...", lineLength * pm->info.size.y);
        if (stream->write(data, lineLength * pm->info.size.y) != lineLength * pm->info.size.y)
        {
            free(data);
            return ErrorBuffer::SetWriteError(fileNameOrNull, li_functionName),
                    kError;
        }
        
        free(data);
#else
        for (int y = pm->info.size.y - 1; y >= 0; y--)
        {
            const uint8_t* line = Pixmap::GetPixelDataForReading(pm) + y * lineLength;

            if (stream->write(line, lineLength) != lineLength)
                return ErrorBuffer::SetWriteError(fileNameOrNull, li_functionName),
                        kError;
        }
#endif

        return kOK;
    }

    bool BmpEncoder::GetFileTypes(const char*** fileTypes_out, size_t* numFileTypes_out)
    {
        static const char* fileTypes[] = {"bmp"};

        *fileTypes_out = fileTypes;
        *numFileTypes_out = lengthof(fileTypes);

        return true;
    }
}
