
#include "framework.hpp"

#include <setjmp.h>

#ifdef ZOMBIE_WINNT
// On Windows, system headers define boolean
#define HAVE_BOOLEAN
#endif

#include <jpeglib.h>

#include <lodepng.h>

#define INPUT_BUFFER_SIZE       4096

namespace zfw
{
    namespace media
    {
        static bool isJpg( SeekableInputStream* input )
        {
            uint8_t magic[2] = { 0, 0 };

            intptr_t read = input->read( magic, 2 );
            input->seek( -read );

            return magic[0] == 0xFF && magic[1] == 0xD8;
        }

        static bool isPng( SeekableInputStream* input )
        {
            uint8_t magic[4] = { 0, 0, 0, 0 };

            intptr_t read = input->read( magic, 4 );
            input->seek( -read );

            return magic[0] == 0x89 && magic[1] == 'P' && magic[2] == 'N' && magic[3] == 'G';
        }

        struct JpegLoadingState
        {
            struct jpeg_source_mgr pub;

            SeekableInputStream* input;
            uint8_t buffer[INPUT_BUFFER_SIZE];
        };

        static void init_source( j_decompress_ptr cinfo )
        {
            return;
        }

        static boolean fill_input_buffer( j_decompress_ptr cinfo )
        {
            JpegLoadingState* state = ( JpegLoadingState* ) cinfo->src;

            size_t numRead = state->input->read( state->buffer, INPUT_BUFFER_SIZE );

            if ( numRead == 0 )
            {
                state->buffer[0] = ( uint8_t ) 0xFF;
                state->buffer[1] = ( uint8_t ) JPEG_EOI;
                numRead = 2;
            }

            state->pub.next_input_byte = ( const JOCTET* ) state->buffer;
            state->pub.bytes_in_buffer = numRead;

            return TRUE;
        }

        static void skip_input_data( j_decompress_ptr cinfo, long numBytes )
        {
            JpegLoadingState* state = ( JpegLoadingState* ) cinfo->src;

            if ( numBytes > 0 )
            {
                while ( numBytes > ( long ) state->pub.bytes_in_buffer )
                {
                    numBytes -= ( long ) state->pub.bytes_in_buffer;
                    state->pub.fill_input_buffer( cinfo );
                }

                state->pub.next_input_byte += ( size_t ) numBytes;
                state->pub.bytes_in_buffer -= ( size_t ) numBytes;
            }
        }

        static void term_source( j_decompress_ptr cinfo )
        {
            return;
        }

        static void jpeg_SDL_RW_src( j_decompress_ptr cinfo, SeekableInputStream* input )
        {
            JpegLoadingState *state;

            if ( !cinfo->src )
                cinfo->src = ( struct jpeg_source_mgr* )( *cinfo->mem->alloc_small )( ( j_common_ptr ) cinfo, JPOOL_PERMANENT, sizeof( JpegLoadingState ) );

            state = ( JpegLoadingState* ) cinfo->src;
            state->pub.init_source = init_source;
            state->pub.fill_input_buffer = fill_input_buffer;
            state->pub.skip_input_data = skip_input_data;
            state->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
            state->pub.term_source = term_source;
            state->input = input;
            state->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
            state->pub.next_input_byte = 0; /* until buffer loaded */
        }

        struct my_error_mgr
        {
            jpeg_error_mgr errmgr;
            jmp_buf escape;
        };

        static void my_error_exit( j_common_ptr cinfo )
        {
            struct my_error_mgr *err = ( my_error_mgr* )cinfo->err;
            longjmp( err->escape, 1 );
        }

        static void output_no_message( j_common_ptr cinfo )
        {
        }

        static bool loadJpg( SeekableInputStream* input, DIBitmap* volatile bmp )
        {
            if ( !isReadable( input ) )
                return false;

            /* Create a decompression structure and load the JPEG header */
            jpeg_decompress_struct cinfo;
            my_error_mgr jerr;

            cinfo.err = jpeg_std_error( &jerr.errmgr );

            jerr.errmgr.error_exit = my_error_exit;
            jerr.errmgr.output_message = output_no_message;

            if ( setjmp( jerr.escape ) )
            {
                /* If we get here, libjpeg found an error */
                jpeg_destroy_decompress( &cinfo );

                return false;
            }

            jpeg_create_decompress( &cinfo );
            jpeg_SDL_RW_src( &cinfo, input );
            jpeg_read_header( &cinfo, TRUE );

            ZFW_ASSERT ( cinfo.num_components == 3 )

            // JCS_CMYK as TEX_RGBA8??
            // That's, of course, not right at all!

            /*if ( cinfo.num_components == 4 )
            {
                cinfo.out_color_space = JCS_CMYK;
                cinfo.quantize_colors = FALSE;
                jpeg_calc_output_dimensions( &cinfo );

                bmp->format = TEX_RGBA8;
            }
            else*/
            {
                cinfo.out_color_space = JCS_RGB;
                cinfo.quantize_colors = FALSE;
                jpeg_calc_output_dimensions( &cinfo );

                bmp->format = TEX_RGB8;
            }

            // align to 4 bytes which is default for OpenGL
            bmp->size = Int2( cinfo.output_width, cinfo.output_height );
            bmp->stride = (cinfo.output_width * cinfo.num_components + 3) & ~3;
            bmp->data.resize( cinfo.output_height * bmp->stride );

            JSAMPROW rowptr[1];

            jpeg_start_decompress( &cinfo );

            while ( cinfo.output_scanline < cinfo.output_height )
            {
                rowptr[0] = ( JSAMPROW ) bmp->data.getPtr( cinfo.output_scanline * bmp->stride );
                jpeg_read_scanlines( &cinfo, rowptr, ( JDIMENSION ) 1 );
            }

            jpeg_finish_decompress( &cinfo );
            jpeg_destroy_decompress( &cinfo );

            return true;
        }


        static bool loadPng( SeekableInputStream* input, DIBitmap* volatile bmp )
        {
			const size_t inputLength = input->getSize();

			std::vector<uint8_t> buffer;
			buffer.resize(inputLength);

			if (input->read(&buffer[0], inputLength) != inputLength)
				return false;

			std::vector<uint8_t> pixels;
			unsigned width, height;

			if ( lodepng::decode( pixels, width, height, &buffer[0], inputLength, LCT_RGBA ) != 0 )
				return false;

            bmp->format = TEX_RGBA8;
            bmp->size = Int2( width, height );
            bmp->stride = width * 4;
            bmp->data.resize( height * bmp->stride );
			memcpy( &bmp->data[0], &pixels[0], height * bmp->stride );

            return true;
        }

        bool MediaLoader::LoadBitmap( const char* path, DIBitmap* bmp, bool required )
        {
            Reference<SeekableInputStream> input = Sys::OpenInput( path );

            if ( input == nullptr )
            {
                if (required)
                    Sys::RaiseException( EX_ASSET_OPEN_ERR, "MediaLoader::LoadBitmap", "failed to open bitmap '%s'", path );

                return false;
            }

            if ( isJpg( input ) )
                return loadJpg( input, bmp );
            else if ( isPng( input ) )
                return loadPng( input, bmp );
            else
            {
                if (required)
                    Sys::RaiseException( EX_ASSET_OPEN_ERR, "MediaLoader::LoadBitmap", "'%s': unsupported file format", path );

                return false;
            }
        }
    }
}
