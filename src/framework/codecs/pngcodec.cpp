
#include <framework/errorbuffer.hpp>
#include <framework/mediacodechandler.hpp>
#include <framework/utility/essentials.hpp>
#include <framework/utility/pixmap.hpp>

#ifndef __linux__
#include <setjmp.h>
#endif

#include <png.h>

namespace zfw
{
    static void png_read_data(png_structp ctx, png_bytep area, png_size_t size)
    {
        // FIXME: Error handling should be done here (this way we'll just fail in PNG decoding)

        InputStream* input = (InputStream*) png_get_io_ptr(ctx);

        input->read(area, size);
    }

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    class PngDecoder : public IPixmapDecoder
    {
        public:
            virtual const char* GetName() override { return "zfw::PngDecoder"; }
            virtual bool GetFileSignature(const uint8_t** signature_out, size_t* signatureLength_out) override;

            virtual IDecoder::DecodingResult_t DecodePixmap(Pixmap_t* pm_out, InputStream* stream,
                    const char* fileName) override;
    };

    // ====================================================================== //
    //  class PngDecoder
    // ====================================================================== //

    IPixmapDecoder* p_CreatePngDecoder()
    {
        return new PngDecoder();
    }

    IDecoder::DecodingResult_t PngDecoder::DecodePixmap(Pixmap_t* pm_out, InputStream* stream, const char* fileName)
    {
        zombie_assert(stream != nullptr);

        IDecoder::DecodingResult_t rc = kOK;

        png_structp png_ptr = 0;
        png_infop info_ptr = 0;

        png_uint_32 width, height;
        int bit_depth, color_type, interlace_type;

        unsigned int numChannels;
        uint8_t* pixelData_out;
        size_t bytesPerLine;

        png_bytep* volatile row_pointers = nullptr;

        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
        zombie_assert(png_ptr);

        info_ptr = png_create_info_struct(png_ptr);
        zombie_assert(info_ptr);

#if (PNG_LIBPNG_VER < 10400 || PNG_LIBPNG_VER >= 10500)
        if (setjmp(png_jmpbuf(png_ptr)))
#else
        if (setjmp(png_ptr->jmpbuf))
#endif
        {
            // we'll land here if PNG decoding fails
            ErrorBuffer::SetError3(EX_ASSET_CORRUPTED, 2,
                    "desc", "An error occured in PNG decoding. The file is corrupted.",
                    "fileName", fileName);

            goto error;
        }

        png_set_read_fn(png_ptr, stream, png_read_data);

        png_read_info(png_ptr, info_ptr);
        png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, 0, 0);

        png_set_strip_16(png_ptr);
        png_set_packing(png_ptr);

        if (color_type == PNG_COLOR_TYPE_GRAY)
            png_set_expand(png_ptr);

        if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
            png_set_gray_to_rgb(png_ptr);

        png_read_update_info(png_ptr, info_ptr);
        png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, 0, 0);

        numChannels = png_get_channels(png_ptr, info_ptr);

        if (color_type == PNG_COLOR_TYPE_PALETTE || !(numChannels == 3 || numChannels == 4))
        {
            ErrorBuffer::SetError3(EX_ASSET_FORMAT_UNSUPPORTED, 2,
                    "desc", "Unsupported PNG format. Supported formats are: RGB, RGBA",
                    "fileName", fileName);
            goto error;
        }

        pm_out->info.size = Int2(width, height);
        pm_out->info.format = (numChannels == 3) ? PixmapFormat_t::RGB8 : PixmapFormat_t::RGBA8;

        pixelData_out = Pixmap::GetPixelDataForWriting(pm_out);
        bytesPerLine = Pixmap::GetBytesPerLine(pm_out->info);

        row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
        assert(row_pointers);

        for (unsigned row = 0; row < height; row++)
            row_pointers[row] = (png_bytep)(pixelData_out + row * bytesPerLine);

        png_read_image(png_ptr, row_pointers);
        png_read_end(png_ptr, info_ptr);

        goto done;

    error:
        rc = kError;

    done:
        if (png_ptr)
            png_destroy_read_struct(&png_ptr, info_ptr ? &info_ptr : (png_infopp) 0, (png_infopp) 0);

         free(row_pointers);

         return rc;
    }

    bool PngDecoder::GetFileSignature(const uint8_t** signature_out, size_t* signatureLength_out)
    {
        static const uint8_t signature[] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

        *signature_out = signature;
        *signatureLength_out = lengthof(signature);

        return true;
    }
}
