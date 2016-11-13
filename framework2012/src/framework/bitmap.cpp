
#include "framework.hpp"

namespace zfw
{
    namespace media
    {
        void Bitmap::Alloc( DIBitmap* bmp, uint16_t w, uint16_t h, TexFormat format )
        {
            bmp->format = format;
            bmp->size = Int2( w, h );
            bmp->stride = w * Bitmap::GetBytesPerPixel( format );
            bmp->data.resize( h * bmp->stride );
        }

        void Bitmap::Blit( DIBitmap* dst, int x, int y, DIBitmap* src )
        {
            return Blit(dst, x, y, src, 0, 0, src->size.x, src->size.y);
        }

        void Bitmap::Blit( DIBitmap* dst, int x, int y, DIBitmap* src, int x1, int y1, int w, int h )
        {
            if ( dst->format != src->format )
            {
                Sys::RaiseException( EX_INVALID_OPERATION, "Bitmap::Blit", "both bitmaps must be of the same format" );
                return;
            }

            w = minimum( w, dst->size.x - x1 );
            h = minimum( h, dst->size.y - y1 );

            if ( w <= 0 || h <= 0 )
                return;

            size_t Bpp = GetBytesPerPixel( src->format );

            for ( int yy = 0; yy < h; yy++ )
            {
                size_t srcoff = (y1 + yy) * src->stride + x1 * Bpp;
                size_t dstoff = (y + yy) * dst->stride + x * Bpp;

                memcpy( dst->data.getPtr( dstoff ), src->data.getPtrUnsafe( srcoff ), w * Bpp );
            }
        }

        size_t Bitmap::GetBytesPerPixel( TexFormat format )
        {
            switch ( format )
            {
                case TEX_RGB8: return 3;
                case TEX_RGBA8: return 4;
                default: return 0;
            }
        }

        void Bitmap::ResizeToFit( DIBitmap* bmp, bool lazy )
        {
            bmp->data.resize( bmp->size.x * bmp->size.y * Bitmap::GetBytesPerPixel( bmp->format ), lazy );
        }
    }
}
