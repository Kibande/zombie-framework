
#include "rendering.hpp"

namespace zr
{
    Font::Font()
    {
        cp_first = 0;
        count = 0;
        tex = nullptr;

        shadow = 1;
    }

    Font::~Font()
    {
        li::release(tex);
    }

    Font* Font::Open(ztype::FaceDesc *desc)
    {
        // FIXME: the retrieved font should be checked against desc->range_min and desc->range_max

        String cacheFilename = ztype::FontCache::GetCacheFilename(desc, "fontcache");

        Font* font = OpenFont(cacheFilename);

        if (font == nullptr)
            Sys::RaiseException(EX_ASSET_OPEN_ERR, "Font::Open", "Failed to load font `%s` @ %d\n(cache file `%s`)",
                    desc->filename, desc->size, cacheFilename.c_str());

        return font;
    }

    Font* Font::Open(const char* cache_filename)
    {
        Font* font = OpenFont(cache_filename);

        if (font == nullptr)
            Sys::RaiseException(EX_ASSET_OPEN_ERR, "Font::Open", "Failed to load cached font `%s`", cache_filename);

        return font;
    }

    Font* Font::Open(const char* filename, int size, int range_min, int range_max)
    {
        ztype::FaceDesc desc = {filename, nullptr, size, 0, range_min, range_max};

        return Open(&desc);
    }

    Font* Font::OpenFont(const String& cache_filename)
    {
        Object<ztype::FaceFile> faceFile = ztype::FaceFile::Open(cache_filename + ".ztype");

        if (faceFile == nullptr)
            return nullptr;

        size_t num_ranges;
        const uint32_t *ranges;
        
        if (!faceFile->GetRanges(&num_ranges, &ranges) || num_ranges < 1)
            return nullptr;

        ztype::CharEntry *entries = (ztype::CharEntry *) malloc(ranges[1] * sizeof(ztype::CharEntry));

        if (entries == nullptr)
            return nullptr;

        if (!faceFile->ReadFTexData(ranges[0], ranges[1], entries))
        {
            free(entries);
            return nullptr;
        }

        ztype::FaceMetrics metrics;

        if (!faceFile->ReadMetrSection(&metrics))
            return nullptr;

        ITexture *tex = zfw::render::Loader::LoadTexture(cache_filename + ".tex", true);

        if (tex == nullptr)
        {
            free(entries);
            return nullptr;
        }

        Object<Font> font = new Font();

        font->tex = tex;
        font->metrics = metrics;
        font->cp_first = ranges[0];
        font->count = ranges[1];
        font->entries.load(entries, ranges[1]);

        free(entries);

        return font.detach();
    }

    void Font::DrawAsciiString( const char* text, CShort2& pos_in, CByte4& colour_in )
    {
        Short2 pos = pos_in;
        Byte4 colour = colour_in;

        //zfw::render::R::SetBlendColour(colour_in);
        //R::PushTransform( drawTransform );

        zr::R::SetTexture(tex);

        for ( ; *text; text++ )
        {
            int32_t cp = *text;

            if ( cp == '\n' )
            {
                pos.x = pos_in.x;
                pos.y += metrics.lineheight;
            }

            int index = cp - cp_first;

            if (index < 0 || (size_t) index >= count)
                continue;

            const ztype::CharEntry& entry = entries[index];

            if ( entry.width != 0 && entry.height != 0 )
                //zfw::render::R::DrawTexture(tex, Float2(pos.x + entry.draw_x, pos.y + entry.draw_y),
                //        Float2(entry.width, entry.height), (Float2*) &entry.uv, glm::mat4());
                zr::R::DrawFilledRect(Short2(pos.x + entry.draw_x, pos.y + entry.draw_y),
                        Short2(pos.x + entry.draw_x + entry.width, pos.y + entry.draw_y + entry.height),
                        Float2(entry.uv[0], entry.uv[1]), Float2(entry.uv[2], entry.uv[3]), colour);

            pos.x += entry.advance;
        }

        //R::PopTransform();
    }

    void Font::DrawAsciiString( const char* text, CInt2& pos_in, CByte4& colour, int flags )
    {
        if (text == nullptr || *text==0)
            return;

        Int2 pos;

        if (flags & (ALIGN_CENTER|ALIGN_RIGHT|ALIGN_MIDDLE|ALIGN_BOTTOM))
        {
            Int2 size = MeasureAsciiString(text);
            pos = Util::Align(pos_in, size, flags);
        }
        else
            pos = pos_in;

        if (shadow != 0)
            DrawAsciiString(text, Short2(pos.x + shadow, pos.y + shadow), RGBA_BLACK_A(colour.a * 65 / 100));

        DrawAsciiString(text, Short2(pos.x, pos.y), colour);
    }

    Int2 Font::MeasureAsciiString( const char* text )
    {
        Int2 pos, size;

        if (text == nullptr)
            return size;

        for ( ; *text; text++ )
        {
            int32_t cp = *text;

            if ( cp == '\n' )
            {
                pos.x = 0;
                size.x = glm::max(size.x, pos.x);
                size.y += metrics.lineheight;
            }

            int index = cp - cp_first;

            if (index < 0 || (size_t) index >= count)
                continue;

            pos.x += entries[index].advance;
        }

        size.x = glm::max(size.x, pos.x);
        size.y += metrics.lineheight;
        return size;
    }
}