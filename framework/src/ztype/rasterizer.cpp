#include <ztype/ztype.hpp>

#include <littl/Allocator.hpp>
#include <littl/Stream.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace ztype
{
    using namespace li;

    static FT_Library library = nullptr;

    static void ExitFreeType()
    {
        FT_Done_FreeType(library);
    }

    class RasterizerImpl : public Rasterizer
    {
        private:
            FT_Face face;
            uint8_t* bufferToRelease;

            FaceMetrics faceMetrics;

            GlyphBitmap bitmap;
            GlyphMetrics metrics;

        public:
            RasterizerImpl(FT_Face face, uint8_t* bufferToRelease);
            virtual ~RasterizerImpl();

            virtual bool GetChar(uint32_t cp, int flags, const GlyphBitmap** bitmap_ptr, const GlyphMetrics** metrics_ptr) override;
            virtual const FaceMetrics* GetMetrics() override { return &faceMetrics; }
    };

    RasterizerImpl::RasterizerImpl(FT_Face face, uint8_t* bufferToRelease)
            : face(face), bufferToRelease(bufferToRelease)
    {
        faceMetrics.ascent =        (int16_t)(face->size->metrics.ascender >> 6);
        faceMetrics.descent =       (int16_t)(face->size->metrics.descender >> 6);
        faceMetrics.lineheight =    (int16_t)(face->size->metrics.height >> 6);
        faceMetrics.reserved =  0;
    }

    RasterizerImpl::~RasterizerImpl()
    {
        FT_Done_Face(face);
        Allocator<uint8_t>::release(bufferToRelease);
    }

    bool RasterizerImpl::GetChar(uint32_t cp, int flags, const GlyphBitmap** bitmap_ptr, const GlyphMetrics** metrics_ptr)
    {
        int error;

        FT_UInt glyph_index = FT_Get_Char_Index( face, cp );

        if (glyph_index == 0)
            return false;

        error = FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );

        if ( error )
            return false;

        if (flags & GETCHAR_RENDER)
        {
            error = FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL );

            if ( error )
                return false;
        }

        FT_GlyphSlot glyph = face->glyph;

        if (flags & GETCHAR_RENDER)
        {
            FT_Bitmap *glyphBitmap = &glyph->bitmap;

            if (glyphBitmap->pitch < 0)
            {
                fprintf(stderr, "[Rasterizer::RenderChar]\tError: negative bitmap pitch\n");
                return false;
            }

            bitmap.width = (uint16_t) glyphBitmap->width;
            bitmap.height = (uint16_t) glyphBitmap->rows;
            bitmap.pitch = glyphBitmap->pitch;
            bitmap.pixels = (uint8_t *) glyphBitmap->buffer;

            *bitmap_ptr = &bitmap;
        }

        if (flags & GETCHAR_METRICS)
        {
            metrics.width =     (uint16_t)(glyph->metrics.width >> 6);
            metrics.height =    (uint16_t)(glyph->metrics.height >> 6);
            metrics.x =         (int16_t)(glyph->metrics.horiBearingX >> 6);
            metrics.y =         faceMetrics.ascent - (int16_t)(glyph->metrics.horiBearingY >> 6);
            metrics.advance =   (int16_t)(glyph->metrics.horiAdvance >> 6);

            *metrics_ptr = &metrics;
        }

        return true;
    }

    Rasterizer* Rasterizer::Create(const char *ttfFile, int pxsize)
    {
        int error;

        if (library == nullptr)
        {
            error = FT_Init_FreeType( &library );

            if ( error )
                return nullptr;

            atexit(ExitFreeType);
        }

        FT_Face face;
        error = FT_New_Face( library, ttfFile, 0, &face );

        if ( error == FT_Err_Unknown_File_Format )
        {
            return nullptr;
        }
        else if ( error )
        {
            return nullptr;
        }

        error = FT_Set_Pixel_Sizes(face, 0, pxsize);

        return new RasterizerImpl(face, nullptr);
    }

    Rasterizer* Rasterizer::Create(InputStream* ttfFile, int pxsize)
    {
        int error;

        const auto size = ttfFile->getSize();

        if (size == 0 || size > (size_t) ~(size_t) 0)
            return nullptr;

        if (library == nullptr)
        {
            error = FT_Init_FreeType( &library );

            if ( error )
                return nullptr;

            atexit(ExitFreeType);
        }

        uint8_t* bytes = Allocator<uint8_t>::allocate((size_t) size);

        if (bytes == nullptr)
            return nullptr;

        if (ttfFile->read(bytes, (size_t) size) != size)
        {
            Allocator<uint8_t>::release(bytes);
            return nullptr;
        }

        FT_Face face;
        error = FT_New_Memory_Face( library, bytes, (size_t) size, 0, &face );

        if ( error == FT_Err_Unknown_File_Format )
        {
            Allocator<uint8_t>::release(bytes);
            return nullptr;
        }
        else if ( error )
        {
            Allocator<uint8_t>::release(bytes);
            return nullptr;
        }

        error = FT_Set_Pixel_Sizes(face, 0, pxsize);

        return new RasterizerImpl(face, bytes);
    }
}
