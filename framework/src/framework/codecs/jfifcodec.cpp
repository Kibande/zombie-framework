
#include <framework/errorbuffer.hpp>
#include <framework/mediacodechandler.hpp>
#include <framework/utility/essentials.hpp>
#include <framework/utility/pixmap.hpp>

#include <littl/Stream.hpp>

#include <setjmp.h>

#ifdef _MSC_VER
// On Windows, system headers define boolean
#define HAVE_BOOLEAN
#endif

#include <jpeglib.h>

namespace zfw
{
    enum { kInputBufferSize = 4 * 1024 };

    struct JpegLoadingState
    {
        struct jpeg_source_mgr pub;

        InputStream* input;
        uint8_t buffer[kInputBufferSize];
    };

    static void init_source(j_decompress_ptr cinfo)
    {
    }

    static boolean fill_input_buffer(j_decompress_ptr cinfo)
    {
        JpegLoadingState* state = (JpegLoadingState*)cinfo->src;

        size_t numRead = state->input->read(state->buffer, kInputBufferSize);

        if (numRead == 0)
        {
            state->buffer[0] = (uint8_t)0xFF;
            state->buffer[1] = (uint8_t)JPEG_EOI;
            numRead = 2;
        }

        state->pub.next_input_byte = (const JOCTET*)state->buffer;
        state->pub.bytes_in_buffer = numRead;

        return TRUE;
    }

    static void skip_input_data(j_decompress_ptr cinfo, long numBytes)
    {
        JpegLoadingState* state = (JpegLoadingState*)cinfo->src;

        if (numBytes > 0)
        {
            while (numBytes > (long)state->pub.bytes_in_buffer)
            {
                numBytes -= (long)state->pub.bytes_in_buffer;
                state->pub.fill_input_buffer(cinfo);
            }

            state->pub.next_input_byte += (size_t)numBytes;
            state->pub.bytes_in_buffer -= (size_t)numBytes;
        }
    }

    static void term_source(j_decompress_ptr cinfo)
    {
    }

    static void jpegInitFromInputStream(j_decompress_ptr cinfo, InputStream* input)
    {
        JpegLoadingState* state;

        if (!cinfo->src)
            cinfo->src = (struct jpeg_source_mgr*)(*cinfo->mem->alloc_small)(
            (j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(JpegLoadingState));

        state = (JpegLoadingState*)cinfo->src;
        state->pub.init_source = init_source;
        state->pub.fill_input_buffer = fill_input_buffer;
        state->pub.skip_input_data = skip_input_data;
        state->pub.resync_to_restart = jpeg_resync_to_restart;
        state->pub.term_source = term_source;
        state->input = input;
        state->pub.bytes_in_buffer = 0;
        state->pub.next_input_byte = nullptr;
    }

    struct my_error_mgr
    {
        jpeg_error_mgr errmgr;
        jmp_buf escape;
    };

    static void my_error_exit(j_common_ptr cinfo)
    {
        struct my_error_mgr *err = (my_error_mgr*)cinfo->err;
        longjmp(err->escape, 1);
    }

    static void output_no_message(j_common_ptr cinfo)
    {
    }

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    class JfifDecoder : public IPixmapDecoder
    {
        public:
            virtual const char* GetName() override { return "zfw::JfifDecoder"; }
            virtual bool GetFileSignature(const uint8_t** signature_out, size_t* signatureLength_out) override;

            virtual IDecoder::DecodingResult_t DecodePixmap(Pixmap_t* pm_out, InputStream* stream,
                const char* fileName) override;
    };

    // ====================================================================== //
    //  class JfifDecoder
    // ====================================================================== //

    IPixmapDecoder* p_CreateJfifDecoder()
    {
        return new JfifDecoder();
    }

    IDecoder::DecodingResult_t JfifDecoder::DecodePixmap(Pixmap_t* pm_out, InputStream* stream, const char* fileName)
    {
        zombie_assert(stream != nullptr);

        IDecoder::DecodingResult_t rc = kOK;

        // Create a decompression structure and load the JPEG header
        jpeg_decompress_struct cinfo;
        my_error_mgr jerr;

        uint8_t* pixelData_out;
        size_t bytesPerLine;

        cinfo.err = jpeg_std_error(&jerr.errmgr);

        jerr.errmgr.error_exit = my_error_exit;
        jerr.errmgr.output_message = output_no_message;

        if (setjmp(jerr.escape))
        {
            // If we end up here, libjpeg found an error
            ErrorBuffer::SetError3(EX_ASSET_CORRUPTED, 2,
                    "desc", "An error occured in JPEG decoding. The file is corrupted.",
                    "fileName", fileName);

            goto error;
        }

        jpeg_create_decompress(&cinfo);
        jpegInitFromInputStream(&cinfo, stream);
        jpeg_read_header(&cinfo, TRUE);

        if (cinfo.num_components != 3)
        {
            ErrorBuffer::SetError3(EX_ASSET_FORMAT_UNSUPPORTED, 2,
                    "desc", "Unsupported number of channels for JPEG file",
                    "fileName", fileName);
            goto error;
        }

        cinfo.out_color_space = JCS_RGB;
        cinfo.quantize_colors = FALSE;
        jpeg_calc_output_dimensions(&cinfo);

        pm_out->info.size = Int2(cinfo.output_width, cinfo.output_height);
        pm_out->info.format = PixmapFormat_t::RGB8;

        pixelData_out = Pixmap::GetPixelDataForWriting(pm_out);
        bytesPerLine = Pixmap::GetBytesPerLine(pm_out->info);

        JSAMPROW rowptr[1];

        jpeg_start_decompress(&cinfo);

        while (cinfo.output_scanline < cinfo.output_height)
        {
            rowptr[0] = (JSAMPROW)(pixelData_out + cinfo.output_scanline * bytesPerLine);
            jpeg_read_scanlines(&cinfo, rowptr, (JDIMENSION) 1);
        }

        jpeg_finish_decompress(&cinfo);
        goto done;

    error:
        rc = kError;

    done:
        jpeg_destroy_decompress(&cinfo);

        return rc;
    }

    bool JfifDecoder::GetFileSignature(const uint8_t** signature_out, size_t* signatureLength_out)
    {
        static const uint8_t signature[] = {0xFF, 0xD8, 0xFF};

        *signature_out = signature;
        *signatureLength_out = li_lengthof(signature);

        return true;
    }
}