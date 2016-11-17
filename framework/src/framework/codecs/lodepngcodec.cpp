
#include <framework/errorbuffer.hpp>
#include <framework/mediacodechandler.hpp>
#include <framework/utility/essentials.hpp>
#include <framework/utility/pixmap.hpp>

#include <littl/Stream.hpp>

#define LODEPNG_NO_COMPILE_DISK
#define LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS
#define LODEPNG_NO_COMPILE_ERROR_TEXT

#include <lodepng.cpp>

namespace zfw
{
    static void BGR8toRGB8(const Pixmap_t* pm_in, Pixmap_t* pm_out)
    {
        const uint8_t* in = Pixmap::GetPixelDataForReading(pm_in);
        uint8_t* out =      Pixmap::GetPixelDataForWriting(pm_out);

        const unsigned int fixup = (pm_in->info.size.x * 3) % 4;

        for (int y = 0; y < pm_in->info.size.y; y++)
        {
            for (int x = 0; x < pm_in->info.size.x; x++)
            {
                out[0] = in[2];
                out[1] = in[1];
                out[2] = in[0];

                in += 3;
                out += 3;
            }

            in += fixup;
            out += fixup;
        }
    }

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    class LodePngDecoder : public IPixmapDecoder
    {
        public:
            virtual const char* GetName() override { return "zfw::LodePngDecoder"; }
            virtual bool GetFileSignature(const uint8_t** signature_out, size_t* signatureLength_out) override;

            virtual IDecoder::DecodingResult_t DecodePixmap(Pixmap_t* pm_out, InputStream* stream,
                    const char* fileNameOrNull) override;
    };

    class LodePngEncoder : public IPixmapEncoder
    {
        public:
            virtual bool GetFileTypes(const char*** fileTypes_out, size_t* numFileTypes_out) override;
            virtual const char* GetName() override { return "zfw::LodePngEncoder"; }

            virtual IEncoder::EncodingResult_t EncodePixmap(const Pixmap_t* pm, OutputStream* stream,
                    const char* fileNameOrNull) override;
    };

    // ====================================================================== //
    //  class LodePngDecoder
    // ====================================================================== //

    IPixmapDecoder* p_CreateLodePngDecoder(ISystem* sys)
    {
        return new LodePngDecoder();
    }

    IDecoder::DecodingResult_t LodePngDecoder::DecodePixmap(Pixmap_t* pm_out, InputStream* stream, const char* fileNameOrNull)
    {
        zombie_assert(stream != nullptr);

        const size_t inputLength = stream->getSize();

        std::vector<uint8_t> buffer;
        buffer.resize(inputLength);
        
        if (stream->read(&buffer[0], inputLength) != inputLength)
            return ErrorBuffer::SetReadError(fileNameOrNull, li_functionName), kError;

        unsigned w, h;

        if (lodepng::decode(pm_out->pixelData, w, h, &buffer[0], inputLength, LCT_RGBA) != 0)
        {
            ErrorBuffer::SetError3(EX_ASSET_CORRUPTED, 2,
                    "desc", "An error occured in PNG decoding. The file is corrupted.",
                    "fileName", fileNameOrNull);

            return kError;
        }

        Pixmap::Initialize(pm_out, Int2(w, h), PixmapFormat_t::RGBA8);
        return kOK;
    }

    bool LodePngDecoder::GetFileSignature(const uint8_t** signature_out, size_t* signatureLength_out)
    {
        static const uint8_t signature[] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

        *signature_out = signature;
        *signatureLength_out = li_lengthof(signature);

        return true;
    }

    // ====================================================================== //
    //  class LodePngEncoder
    // ====================================================================== //

    IPixmapEncoder* p_CreateLodePngEncoder(ISystem* sys)
    {
        return new LodePngEncoder();
    }
    
    IEncoder::EncodingResult_t LodePngEncoder::EncodePixmap(const Pixmap_t* pm, OutputStream* stream,
            const char* fileNameOrNull)
    {
        zombie_assert(pm->info.size.x > 0);
        zombie_assert(pm->info.size.y > 0);

        Pixmap_t pm_new;

        if (pm->info.format == PixmapFormat_t::BGR8)
        {
            Pixmap::Initialize(&pm_new, pm->info.size, PixmapFormat_t::RGB8);
            BGR8toRGB8(pm, &pm_new);
            pm = &pm_new;
        }

        if (pm->info.format != PixmapFormat_t::RGB8 && pm->info.format != PixmapFormat_t::RGBA8)
        {
            ErrorBuffer::SetError3(errRequestedEncodingNotSupported, 2,
                    "desc", "Unsupported PNG format for encoding. Supported formats are: BGR8, RGB8, RGBA8",
                    "fileName", fileNameOrNull);
            return kError;
        }

        std::vector<uint8_t> buffer;

        if (lodepng::encode(buffer, pm->pixelData, pm->info.size.x, pm->info.size.y,
                (pm->info.format == PixmapFormat_t::RGB8) ? LCT_RGB : LCT_RGBA, 8) != 0)
        {
            ErrorBuffer::SetError3(EX_ASSET_CORRUPTED, 2,
                    "desc", "An error occured in PNG encoding.",
                    "fileName", fileNameOrNull);

            return kError;
        }

        if (stream->write(&buffer[0], buffer.size()) != buffer.size())
            return ErrorBuffer::SetWriteError(fileNameOrNull, li_functionName), kError;

        return kOK;
    }

    bool LodePngEncoder::GetFileTypes(const char*** fileTypes_out, size_t* numFileTypes_out)
    {
        static const char* fileTypes[] = {"png"};

        *fileTypes_out = fileTypes;
        *numFileTypes_out = li_lengthof(fileTypes);

        return true;
    }
}
