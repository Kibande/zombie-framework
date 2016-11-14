
#include "RenderingKitImpl.hpp"
#include <RenderingKit/RenderingKitUtility.hpp>

#include <framework/errorcheck.hpp>
#include <framework/resourcemanager.hpp>

#include <ztype/ztype.hpp>

namespace RenderingKit
{
    using namespace zfw;

    typedef glm::tvec4<uint16_t, glm::defaultp> UShort4;

    enum { FONT_VERTEX_SIZE = 24 };

    enum
    {
        c_byte4
    };

    struct FontVertex_t
    {
        float x, y, z;      // 12
        float u, v;         // 8
        uint8_t rgba[4];    // 4
    };

    static const VertexAttrib_t fontVertexAttribs[] =
    {
        {"in_Position",     0,  RK_ATTRIB_FLOAT_3},
        {"in_UV",           12, RK_ATTRIB_FLOAT_2},
        {"in_Color",        20, RK_ATTRIB_UBYTE_4},
        {}
    };

    inline uint32_t s_MkCmd(uint32_t cmd)
    {
        return 0x80000000 | cmd;
    }

    static inline Unicode::Char s_FilterGlyphAdvance(Unicode::Char cp, int advance)
    {
        if (cp == '\t')
            return advance * 4;
        else
            return advance;
    }

    static size_t s_SkipCmd(size_t i, uint32_t cp)
    {
        switch (cp ^ 0x80000000)
        {
            case c_byte4:
                i++;
                break;
        }

        return i;
    }

    class Paragraph
    {
        public:
            IGeomChunk* gc;

            Byte4 initColour;
            int alignFlags;

            uint32_t numChars, maxWidth;
            uint32_t numLines, allocLines;

            uint32_t *lines;

        public:
            Paragraph()
            {
                gc = nullptr;
                numLines = 0;
                allocLines = 0;
                lines = nullptr;
            }

            ~Paragraph()
            {
                Allocator<>::release(lines);
            }

            void Size(size_t needed)
            {
                if (needed > allocLines)
                {
                    allocLines = (uint32_t) std::max<size_t>(allocLines * 2, align<8>(needed));
                    lines = Allocator<uint32_t>::resize(lines, allocLines);
                }
            }
    };

    class GLFontFace: public IGLFontFace, public IFontQuadSink
    {
        // X * 64 * 64 chars (20:6:6 bits)
        static const unsigned int LowBits = 6, MidBits = 6;
        enum { INIT_TEXTURE_SIZE = 256 };

        struct Bucket
        {
            ztype::CharEntry glyph[1 << LowBits];
        };

        struct Layer
        {
            Short2 offset;
            Byte4 colour;
            bool blend;
        };

        zfw::ErrorBuffer_t* eb;
        RenderingKit* rk;
        IRenderingManagerBackend* rm;
        String name;

        Paragraph sharedP;

        shared_ptr<IGLTexture> texture;
        shared_ptr<IGLTextureAtlas> atlas;
        shared_ptr<IGLMaterial> material;
        shared_ptr<IVertexFormat> vertexFormat;

        // ztype resources
        unique_ptr<ztype::FaceFile> faceFile;
        unique_ptr<ztype::Rasterizer> rasterizer;

        ztype::FaceMetrics metrics;

        // Glyph management
        //PixelAtlas* atlas;
        Array<Bucket**> buckets;

        // Rendering management
        IFontQuadSink* quadSink;

        Int2 shadowOffset;
        Byte4 shadowColour;

        List<Layer> layers;

        void p_DropResources();
        ztype::CharEntry* p_GetGlyph(Unicode::Char cp, bool canAlloc, bool& didAlloc);
        bool p_RasterizeGlyph(ztype::CharEntry* glyph, Unicode::Char cp);

        //bool p_RenderParagraph(Paragraph* p, const Float3& pos0, const char* styleParams, IGeomChunk* gc);

        public:
            GLFontFace(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name);
            ~GLFontFace();

            // Thread-safe
            virtual Paragraph* CreateParagraph() override { return new Paragraph; }

            // Not thread-safe
            virtual const char* GetName() override { return name.c_str(); }

            virtual void SetTextureAtlas(shared_ptr<ITextureAtlas> atlas) override;

            virtual bool Open(const char* path, int size, int flags) override;

            virtual unsigned int GetLineHeight() { return metrics.lineheight; }
            virtual void SetQuadSink(IFontQuadSink* sink) { quadSink = sink; }

            virtual void LayoutParagraph(Paragraph* p, const char* textUtf8, Byte4 colour, int layoutFlags) override;
            virtual Int2 MeasureParagraph(Paragraph* p) override;
            virtual void ReleaseParagraph(Paragraph* p) override;

            virtual Int2 MeasureText(const char* textUtf8) override;

            virtual bool GetCharNear(Paragraph* p, const Float3& posInPS, ptrdiff_t* followingCharIndex_out, Float3* followingCharPosInPS_out) override;
            virtual bool GetCharPos(Paragraph* p, ptrdiff_t charIndex, Float3* charPosInPS_out) override;
            virtual Float3 ToParagraphSpace(Paragraph* p, const Float3& areaPos, const Float2& areaSize, const Float3& pos) override;
            virtual Float3 FromParagraphSpace(Paragraph* p, const Float3& areaPos, const Float2& areaSize, const Float3& posInPS) override;

            virtual void DrawParagraph(Paragraph* p, const Float3& areaPos, const Float2& areaSize) override;
            virtual void DrawParagraph(Paragraph* p, const Float3& areaPos, const Float2& areaSize, Byte4 colour) override;
            virtual void DrawText(const char* textUtf8, Byte4 colour, int layoutFlags, const Float3& areaPos, const Float2& areaSize) override;

            virtual int OnFontQuad(const Float3& pos, const Float2& size, const Float2 uv[2], Byte4 colour) override;

            //virtual ITexture* HACK_GetTexture() override { return texture; }

            /*virtual IObjectSceneNode* RenderParagraph(Paragraph* p, const Float3& pos,
                    ISceneGraph* sceneGraph, IGeomBuffer* gb, const char* styleParams) override;
            virtual void RenderParagraph(Paragraph* p, const Float3& pos,
                    IObjectSceneNode* obj, IGeomBuffer* gb, const char* styleParams) override;
            virtual IObjectSceneNode* RenderString(const char* textUtf8, int layoutFlags, const Float3& pos,
                    ISceneGraph* sceneGraph, IGeomBuffer* gb, const char* styleParams) override;
            virtual void RenderString(const char* textUtf8, int layoutFlags, const Float3& pos,
                    IObjectSceneNode* obj, IGeomBuffer* gb, const char* styleParams) override;*/

            //virtual void RenderParagraph(Paragraph* p, const Float3& pos, IGeomChunk* gc, const char* styleParams) override;

            //virtual void DrawText(IGeomChunk* gc) override;
    };

    shared_ptr<IGLFontFace> p_CreateFontFace(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm,
            const char* name)
    {
        return std::make_shared<GLFontFace>(eb, rk, rm, name);
    }

    GLFontFace::GLFontFace(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name)
            : name(name)
    {
        this->eb = eb;
        this->rk = rk;
        this->rm = rm;

        rasterizer = nullptr;
        faceFile = nullptr;

        atlas = nullptr;

        quadSink = this;

        shadowOffset = Int2(1, 1);
        shadowColour = RGBA_BLACK_A(96);
    }

    GLFontFace::~GLFontFace()
    {
        p_DropResources();
    }

    bool GLFontFace::Open(const char* path, int size, int flags)
    {
        if (atlas == nullptr)
        {
            texture = p_CreateTexture(eb, rk, rm, sprintf_255("%s/texture", name.c_str()));
            texture->SetContentsUndefined(Int2(INIT_TEXTURE_SIZE, INIT_TEXTURE_SIZE), kTextureNoMipmaps, kTextureRGBA8);

            auto pixelAtlas = new PixelAtlas(texture->GetSize(), Int2(2, 2), Int2(0, 0), Int2(8, 8));

            atlas = p_CreateTextureAtlas(eb, rk, rm, sprintf_255("%s/atlas", name.c_str()));
            atlas->GLInitWith(texture, pixelAtlas);
        }
        else
            texture = atlas->GLGetTexture();

        auto shader = rm->GetSharedResourceManager()->GetResource<IGLShaderProgram>("path=RenderingKit/basicTextured", zfw::RESOURCE_REQUIRED, 0);
        zombie_ErrorCheck(shader);

        vertexFormat = rm->CompileVertexFormat(shader.get(), FONT_VERTEX_SIZE, fontVertexAttribs, false);

        material = p_CreateMaterial(eb, rk, rm, sprintf_255("%s/material", name.c_str()), shader);
        material->SetTexture("tex", texture);

        //ztype::FaceDesc desc = {filename, nullptr, size, 0, 0, 0};

        //R_DropResources();

        //String cacheFilename = ztype::FontCache::GetCacheFilename(desc, "fontcache");

        unique_ptr<ztype::FaceFile> faceFile;// = ztype::FaceFile::Open(cache_filename + ".ztype");

        if (faceFile == nullptr)
        {
            //We'll need the Rasterizer right now

            unique_ptr<InputStream> is(rk->GetSys()->OpenInput(path));

            if (is == nullptr)
                return ErrorBuffer::SetError(eb, EX_ASSET_OPEN_ERR, "desc", (const char*) sprintf_t<255>("Failed to open font file '%s'.", path),
                        "function", li_functionName,
                        nullptr),
                        false;

            rasterizer.reset(ztype::Rasterizer::Create(is.get(), size));

            if (rasterizer == nullptr)
                return ErrorBuffer::SetError(eb, EX_ASSET_OPEN_ERR, "desc", (const char*) sprintf_t<255>("Failed to open font '%s'.", path),
                        "function", li_functionName,
                        nullptr),
                        false;

            memcpy(&metrics, rasterizer->GetMetrics(), sizeof(metrics));
        }
        else
        {
            /*size_t num_ranges;
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

            if (!faceFile->ReadMetrSection(&metrics))
                return nullptr;

            ITexture *tex = zfw::render::Loader::LoadTexture(cache_filename + ".tex", true);

            if (tex == nullptr)
            {
                free(entries);
                return nullptr;
            }*/
        }

        layers.clear();

        if (flags & FLAG_SHADOW)
        {
            auto& shadow = layers.addEmpty();
            shadow.offset = shadowOffset;
            shadow.colour = shadowColour;
            shadow.blend = false;
        }

        auto& base = layers.addEmpty();
        base.offset = Short2();
        base.colour = RGBA_WHITE;
        base.blend = true;

        return true;
    }

    /*void GLFontFace::DrawText(IGeomChunk* gc)
    {
        rm->SetMaterial(RKReference(material));
        rm->DrawTriangles(gc);
    }*/

    void GLFontFace::DrawParagraph(Paragraph* p, const Float3& areaPos, const Float2& areaSize)
    {
        DrawParagraph(p, areaPos, areaSize, p->initColour);
    }

    void GLFontFace::DrawParagraph(Paragraph* p, const Float3& areaPos, const Float2& areaSize, Byte4 colour)
    {
        const size_t numVertices = layers.getLength() * p->numChars * 6;

        const Int2 paragraphSize(p->maxWidth, p->numLines * metrics.lineheight);

        const Float3 pos0 = FromParagraphSpace(p, areaPos, areaSize, Float3(0.0f, 0.0f, 0.0f));

        //Int2 xy = Util::Align(Int2(), paragraphSize, p->alignFlags);

        float y = pos0.y;
        const float z = pos0.z;

        Unicode::Char cp;
        size_t i = 0;

        for (unsigned int line = 0; line < p->numLines; line++)
        {
            const uint32_t charsOnLine = p->lines[i++];
            const uint32_t lineWidth = p->lines[i++];

            float x = pos0.x;

            if (p->alignFlags & ALIGN_HCENTER)
                x += (paragraphSize.x - lineWidth) / 2;
            else if (p->alignFlags & ALIGN_RIGHT)
                x += paragraphSize.x - lineWidth;

            for (unsigned int j = 0; j < charsOnLine; j++)
            {
                cp = p->lines[i++];

                if (cp & 0x80000000)
                {
                    switch (cp ^ 0x80000000)
                    {
                        case c_byte4:
                            j++;
                            cp = p->lines[i++];
                            memcpy(&colour[0], &cp, 4);
                            break;
                    }

                    continue;
                }

                bool didAlloc = false;
                ztype::CharEntry* glyph = p_GetGlyph(cp, false, didAlloc);

                ZFW_ASSERT(glyph != nullptr)

                for (size_t l = 0; l < layers.getLength(); l++)
                {
                    auto& layer = layers.getUnsafe(l);

                    int xoff = glyph->draw_x + layer.offset.x;
                    int yoff = glyph->draw_y + layer.offset.y;

                    const Byte4 rgba = layer.blend ? (Byte4(UShort4(colour) * UShort4(layer.colour) / UShort4(255, 255, 255, 255))) : layer.colour;

                    quadSink->OnFontQuad(Float3(xoff + x, yoff + y, z), Float2(glyph->width, glyph->height), (const Float2*) glyph->uv, rgba);
                }

                x += s_FilterGlyphAdvance(cp, glyph->advance);
            }

            y += metrics.lineheight;
        }

        //rm->CheckErrors(li_functionName);
    }

    void GLFontFace::DrawText(const char* textUtf8, Byte4 colour, int layoutFlags, const Float3& areaPos, const Float2& areaSize)
    {
        LayoutParagraph(&sharedP, textUtf8, colour, layoutFlags);
        return DrawParagraph(&sharedP, areaPos, areaSize);
    }

    Float3 GLFontFace::FromParagraphSpace(Paragraph* p, const Float3& areaPos, const Float2& areaSize, const Float3& posInPS)
    {
        const Int2 paragraphSize(p->maxWidth, p->numLines * metrics.lineheight);

        Int2 pos0;
        Util::Align(pos0, Int2(areaPos), Int2(areaPos) + Int2(areaSize), paragraphSize, p->alignFlags);

        return Float3(pos0.x, pos0.y, areaPos.z) + posInPS;
    }

    bool GLFontFace::GetCharNear(Paragraph* p, const Float3& posInPS, ptrdiff_t* followingCharIndex_out, Float3* followingCharPosInPS_out)
    {
        const Int2 paragraphSize(p->maxWidth, p->numLines * metrics.lineheight);

        int x;
        int y = 0;

        const int posInPS_x = (int) posInPS.x;

        Unicode::Char cp;
        size_t i = 0, pc = 0;

        for (unsigned int line = 0; line < p->numLines; line++)
        {
            const uint32_t charsOnLine = p->lines[i++];
            const uint32_t lineWidth = p->lines[i++];

            if (line > 0)
                y += metrics.lineheight;

            if (p->alignFlags & ALIGN_HCENTER)
                x = (paragraphSize.x - lineWidth) / 2;
            else if (p->alignFlags & ALIGN_RIGHT)
                x = paragraphSize.x - lineWidth;
            else
                x = 0;

            for (unsigned int j = 0; j < charsOnLine; j++)
            {
                cp = p->lines[i++];

                if (cp & 0x80000000)
                {
                    i = s_SkipCmd(i, cp);
                    continue;
                }

                bool didAlloc = false;
                ztype::CharEntry* glyph = p_GetGlyph(cp, false, didAlloc);
                
                ZFW_ASSERT(glyph != nullptr)

                const int advance = s_FilterGlyphAdvance(cp, glyph->advance);

                if (x + advance / 2 > posInPS_x)
                {
                    // Double break
                    line = p->numLines;
                    break;
                }

                x += advance;
                pc++;
            }
        }

        *followingCharIndex_out = pc;
        *followingCharPosInPS_out = Float3(x, y, 0.0f);
        return true;
    }

    bool GLFontFace::GetCharPos(Paragraph* p, ptrdiff_t charIndex, Float3* charPosInPS_out) 
    {
        const Int2 paragraphSize(p->maxWidth, p->numLines * metrics.lineheight);

        int x;
        int y = 0;

        Unicode::Char cp;
        size_t i = 0, pc = 0;

        for (unsigned int line = 0; line < p->numLines; line++)
        {
            const uint32_t charsOnLine = p->lines[i++];
            const uint32_t lineWidth = p->lines[i++];

            if (line > 0)
                y += metrics.lineheight;

            if (p->alignFlags & ALIGN_HCENTER)
                x = (paragraphSize.x - lineWidth) / 2;
            else if (p->alignFlags & ALIGN_RIGHT)
                x = paragraphSize.x - lineWidth;
            else
                x = 0;

            for (unsigned int j = 0; j < charsOnLine; j++)
            {
                if (pc == charIndex)
                {
                    // Double break
                    line = p->numLines;
                    break;
                }

                cp = p->lines[i++];

                if (cp & 0x80000000)
                {
                    i = s_SkipCmd(i, cp);
                    continue;
                }

                bool didAlloc = false;
                ztype::CharEntry* glyph = p_GetGlyph(cp, false, didAlloc);
                
                ZFW_ASSERT(glyph != nullptr)

                x += s_FilterGlyphAdvance(cp, glyph->advance);
                pc++;
            }
        }

        *charPosInPS_out = Float3(x, y, 0.0f);
        return true;
    }

    void GLFontFace::LayoutParagraph(Paragraph* p, const char* textUtf8, Byte4 colour, int layoutFlags)
    {
        int line_width = 0, line_height = 0, max_width = 0, total_height = 0;
        size_t unitsUsed = 0;
        //uint32_t* lineLength, * lineWidth;
        uint32_t charsOnCurrLine;

        line_height = metrics.lineheight;

        p->initColour = colour;
        p->numChars = 0;
        p->numLines = 0;

        charsOnCurrLine = 0;

        // Reserve for line length+width
        p->Size(unitsUsed + 2);
        unitsUsed += 2;

        Unicode::Char cp;
        for ( ; textUtf8 != nullptr; )
        {
            unsigned int c = Utf8::decode(cp, textUtf8, 4);

            if (c == 0 || cp == Unicode::invalidChar || cp == 0)
                break;

            textUtf8 += c;

            if (cp == '\r')
                continue;
            else if (cp == '\n')
            {
                // WHOA BRO SURE IS BLACK MAGIC
                p->lines[unitsUsed - charsOnCurrLine - 2] = charsOnCurrLine;
                p->lines[unitsUsed - charsOnCurrLine - 1] = line_width;

                max_width = std::max(max_width, line_width);
                total_height += line_height;
                p->numLines++;
                line_width = 0;

                charsOnCurrLine = 0;

                // Reserve for line length+width
                p->Size(unitsUsed + 2);
                unitsUsed += 2;

                continue;
            }
            else if (cp == '<')
            {
                // Parse Command
                char cmd[16];
                unsigned int i;

                for (i = 0; *textUtf8 != ':' && *textUtf8 != 0 && i < 16 - 1; i++)
                    cmd[i] = *(textUtf8++);

                cmd[i] = 0;

                if (strcmp(cmd, "c") == 0 && *textUtf8 == ':')
                {
                    const char* base = (++textUtf8);

                    // Find the end of the command
                    while (*textUtf8 != 0 && *textUtf8 != '>')
                        textUtf8++;

                    if (*textUtf8 == '>')
                    {
                        // Figure out value length
                        const size_t valueLength = textUtf8 - base;

                        Byte4 colour;
                        Util::ParseColourHex(base, valueLength, colour);

                        // Write c_byte4 command; the colour packing is free on little endian
                        p->Size(unitsUsed + 2);

                        p->lines[unitsUsed++] = s_MkCmd(c_byte4);
                        memcpy(&p->lines[unitsUsed++], &colour[0], 4);
                        charsOnCurrLine += 2;

                        textUtf8++;
                    }
                }
                else
                {
                    // Unknown command, skip
                    while (*textUtf8 != 0 && *textUtf8 != '>')
                        textUtf8++;

                    if (*textUtf8 == '>')
                        textUtf8++;
                }

                continue;
            }

            bool didAlloc = false;
            ztype::CharEntry* glyph = p_GetGlyph(cp, true, didAlloc);

            ZFW_ASSERT(glyph != nullptr)

            if (didAlloc)
            {
                if (!p_RasterizeGlyph(glyph, cp))
                    continue;
            }

            p->Size(unitsUsed + 1);

            p->lines[unitsUsed++] = cp;
            charsOnCurrLine++;
            p->numChars++;

            line_width += s_FilterGlyphAdvance(cp, glyph->advance);
        }

        if (line_width > 0 || ((layoutFlags & LAYOUT_FORCE_FIRST_LINE) && p->numLines == 0))
        {
            p->lines[unitsUsed - charsOnCurrLine - 2] = charsOnCurrLine;
            p->lines[unitsUsed - charsOnCurrLine - 1] = line_width;

            max_width = std::max(max_width, line_width);
            total_height += line_height;
            p->numLines++;
        }
        else
        {
            // Nothing to do here; the last line will be simply ignored because p->numLines
        }
        
        p->maxWidth = max_width;
        p->alignFlags = layoutFlags;
    }

    Int2 GLFontFace::MeasureParagraph(Paragraph* p)
    {
        return Int2(p->maxWidth, p->numLines * metrics.lineheight);
    }

    Int2 GLFontFace::MeasureText(const char* textUtf8)
    {
        LayoutParagraph(&sharedP, textUtf8, RGBA_WHITE, 0);
        return MeasureParagraph(&sharedP);
    }
    
    int GLFontFace::OnFontQuad(const Float3& pos, const Float2& size, const Float2 uv[2], Byte4 colour)
    {
        FontVertex_t* p_vertices = rm->VertexCacheAllocAs<FontVertex_t>(vertexFormat.get(), material.get(),
                RK_TRIANGLES, 6);

        // LT, LB, RT; RT, LB, RB

        p_vertices->x = pos.x;
        p_vertices->y = pos.y;
        p_vertices->z = pos.z;
        p_vertices->u = uv[0].x;
        p_vertices->v = uv[0].y;
        memcpy(p_vertices->rgba, &colour[0], 4);
        p_vertices++;

        p_vertices->x = pos.x;
        p_vertices->y = pos.y + size.y;
        p_vertices->z = pos.z;
        p_vertices->u = uv[0].x;
        p_vertices->v = uv[1].y;
        memcpy(p_vertices->rgba, &colour[0], 4);
        p_vertices++;

        p_vertices->x = pos.x + size.x;
        p_vertices->y = pos.y;
        p_vertices->z = pos.z;
        p_vertices->u = uv[1].x;
        p_vertices->v = uv[0].y;
        memcpy(p_vertices->rgba, &colour[0], 4);
        p_vertices++;

        p_vertices->x = pos.x + size.x;
        p_vertices->y = pos.y;
        p_vertices->z = pos.z;
        p_vertices->u = uv[1].x;
        p_vertices->v = uv[0].y;
        memcpy(p_vertices->rgba, &colour[0], 4);
        p_vertices++;

        p_vertices->x = pos.x;
        p_vertices->y = pos.y + size.y;
        p_vertices->z = pos.z;
        p_vertices->u = uv[0].x;
        p_vertices->v = uv[1].y;
        memcpy(p_vertices->rgba, &colour[0], 4);
        p_vertices++;

        p_vertices->x = pos.x + size.x;
        p_vertices->y = pos.y + size.y;
        p_vertices->z = pos.z;
        p_vertices->u = uv[1].x;
        p_vertices->v = uv[1].y;
        memcpy(p_vertices->rgba, &colour[0], 4);
        p_vertices++;

        return 0;
    }

    void GLFontFace::p_DropResources()
    {
        material.reset();
        texture.reset();
        vertexFormat.reset();

        atlas.reset();

        rasterizer.reset();
        faceFile.reset();

        for (size_t row = 0; row < buckets.getCapacity(); row++)
        {
            if (buckets[row] != nullptr)
            {
                for (size_t bucket = 0; bucket < (1 << MidBits); bucket++)
                    if (buckets[row][bucket] != nullptr)
                        delete buckets[row][bucket];

                Allocator<>::release(buckets[row]);
            }
        }

        buckets.resize(4, false);
    }

    ztype::CharEntry* GLFontFace::p_GetGlyph(Unicode::Char cp, bool canAlloc, bool& didAlloc)
    {
        if (cp == '\t')
            cp = ' ';

        const unsigned int Low = cp & ((1 << LowBits) - 1);
        const unsigned int Mid = (cp >> LowBits) & ((1 << MidBits) - 1);
        const unsigned int High = (cp >> (LowBits + MidBits));

        Bucket** row;

        row = (High < buckets.getCapacity()) ? buckets.getUnsafe(High) : nullptr;

        if (row == nullptr && canAlloc)
        {
            row = Allocator<Bucket*>::allocate(1 << MidBits);
            buckets[High] = row;
        }

        if (row == nullptr)
            return nullptr;

        Bucket* bucket = row[Mid];

        if (bucket == nullptr)
        {
            if (canAlloc)
            {
                bucket = new Bucket;
                Allocator<ztype::CharEntry>::clear(bucket->glyph, 1 << LowBits);
                row[Mid] = bucket;
            }
            else
                return nullptr;
        }

        ztype::CharEntry* entry = &bucket->glyph[Low];

        if (!(entry->reserved[0]) & 1)
        {
            if (canAlloc)
            {
                entry->reserved[0] |= 1;
                didAlloc = true;
                return entry;
            }
            else
                return nullptr;
        }

        return entry;
    }

    bool GLFontFace::p_RasterizeGlyph(ztype::CharEntry* glyph, Unicode::Char cp)
    {
        const ztype::GlyphBitmap* bmp;
        const ztype::GlyphMetrics* metr;

        if (!rasterizer->GetChar(cp, ztype::Rasterizer::GETCHAR_METRICS | ztype::Rasterizer::GETCHAR_RENDER, &bmp, &metr))
            return false;

        Int2 tex_pos;
        if (!atlas->AllocWithoutResizing(Int2(bmp->width, bmp->height), &tex_pos))
        {
            // FIXME: grow texture & atlas
            ZFW_ASSERT(false)
        }

        uint32_t* pixels = Allocator<uint32_t>::allocate(bmp->width * bmp->height);
        const static uint32_t FONT_COLOUR = 0xFFFFFF;

        ZFW_ASSERT(pixels != nullptr)
        uint32_t* out = pixels;

        for (int yy = 0; yy < bmp->height; yy++)
        {
            uint8_t *in = bmp->pixels + bmp->pitch * yy;
            
            for (int xx = 0; xx < bmp->width; xx++)
                *out++ = FONT_COLOUR | ((*in++) << 24);
        }

        // TODO: Could/should we batch this?
        texture->TexSubImage(tex_pos.x, tex_pos.y, bmp->width, bmp->height, PixmapFormat_t::RGBA8,
                (const uint8_t*) pixels);

        Allocator<>::release(pixels);

        Int2 texSize = texture->GetSize();
        glyph->uv[0] = (float) tex_pos.x / texSize.x;
        glyph->uv[1] = (float) tex_pos.y / texSize.y;
        glyph->uv[2] = (float) (tex_pos.x + bmp->width) / texSize.x;
        glyph->uv[3] = (float) (tex_pos.y + bmp->height) / texSize.y;
                        
        glyph->width = metr->width;
        glyph->height = metr->height;
        glyph->draw_x = metr->x;
        glyph->draw_y = metr->y;
        glyph->advance = metr->advance;

        // TODO: Add to persistent cache

        return true;
    }

    /*bool GLFontFace::p_RenderParagraph(Paragraph* p, const Float3& pos0, const char* styleParams, IGeomChunk* gc)
    {
        IVertexFormat* vf = material->GetVertexFormat();

        ZFW_ASSERT(!vf->IsGroupedByAttrib())

        const size_t numVertices = layers.getLength() * p->numChars * 6;

        if (!gc->AllocVertices(material->GetVertexFormat(), numVertices, 0))
            return false;
        
        const size_t size = numVertices * vf->GetVertexSize();
        uint8_t* buffer = (uint8_t*) alloca(size);

        Component<Float3> c_pos;
        Component<Float2> c_uv0;
        Component<Byte4> c_rgba;

        vf->GetComponent("pos", c_pos);
        vf->GetComponent("uv0", c_uv0);
        vf->GetComponent("rgba", c_rgba);

        auto pos = c_pos.ForPtr(buffer);
        auto uv0 = c_uv0.ForPtr(buffer);
        auto rgba = c_rgba.ForPtr(buffer);

        Byte4 colour = RGBA_WHITE;

        // Parse styleParams
        const char *key, *value;
        size_t keyLength, valueLength;

        while (Util::ParseDesc(styleParams, key, keyLength, value, valueLength))
        {
            // FIXME: strncmp is incorrect here
            if (strncmp(key, "colour", keyLength) == 0)
                ParseColour(colour, value, valueLength);
        }

        const Int2 paragraphSize(p->maxWidth, p->numLines * metrics.lineheight);

        //Int2 xy;
        //Util::Align(xy, minPos, maxPos, paragraphSize, p->alignFlags);

        Int2 xy = Util::Align(Int2(), paragraphSize, p->alignFlags);

        float y = pos0.y + xy.y;
        const float z = pos0.z;

        Unicode::Char cp;
        size_t i = 0;

        for (unsigned int line = 0; line < p->numLines; line++)
        {
            const uint32_t charsOnLine = p->lines[i++];
            const uint32_t lineWidth = p->lines[i++];

            float x = pos0.x + xy.x;

            if (p->alignFlags & ALIGN_HCENTER)
                x += (paragraphSize.x - lineWidth) / 2;
            else if (p->alignFlags & ALIGN_RIGHT)
                x += paragraphSize.x - lineWidth;

            for (unsigned int j = 0; j < charsOnLine; j++)
            {
                cp = p->lines[i++];

                if (cp & 0x80000000)
                {
                    switch (cp ^ 0x80000000)
                    {
                        case c_byte4:
                            j++;
                            cp = p->lines[i++];
                            memcpy(&colour[0], &cp, 4);
                            break;
                    }

                    continue;
                }

                bool didAlloc = false;
                ztype::CharEntry* glyph = p_GetGlyph(cp, false, didAlloc);

                ZFW_ASSERT(glyph != nullptr)

                for (size_t l = 0; l < layers.getLength(); l++)
                {
                    auto& layer = layers.getUnsafe(l);

                    int xoff = glyph->draw_x + layer.offset.x;
                    int yoff = glyph->draw_y + layer.offset.y;

                    GenQuad(pos, Float3(xoff + x, yoff + y, z), Float3(xoff + x, yoff + y + glyph->height, z),
                        Float3(xoff + x + glyph->width, yoff + y + glyph->height, z), Float3(xoff + x + glyph->width, yoff + y, z));
                    GenQuad(uv0, Float2(glyph->uv[0], glyph->uv[1]), Float2(glyph->uv[0], glyph->uv[3]),
                        Float2(glyph->uv[2], glyph->uv[3]), Float2(glyph->uv[2], glyph->uv[1]));

                    if (layer.blend)
                        GenQuad(rgba, Byte4(UShort4(colour) * UShort4(layer.colour) / UShort4(255, 255, 255, 255)));
                    else
                        GenQuad(rgba, layer.colour);
                }

                x += glyph->advance;
            }

            y += metrics.lineheight;
        }

        gc->UpdateVertices(0, buffer, size);
        return 0;
    }*/
    
    void GLFontFace::ReleaseParagraph(Paragraph* p)
    {
        delete p;
    }

    void GLFontFace::SetTextureAtlas(shared_ptr<ITextureAtlas> atlas)
    {
        ZFW_ASSERT(this->atlas == nullptr)

        this->atlas = std::static_pointer_cast<IGLTextureAtlas>(atlas);
    }

    Float3 GLFontFace::ToParagraphSpace(Paragraph* p, const Float3& areaPos, const Float2& areaSize, const Float3& pos)
    {
        // pos0 calculation is identical to FromParagraphSpace

        const Int2 paragraphSize(p->maxWidth, p->numLines * metrics.lineheight);

        Int2 pos0;
        Util::Align(pos0, Int2(areaPos), Int2(areaPos) + Int2(areaSize), paragraphSize, p->alignFlags);

        return pos - Float3(pos0.x, pos0.y, areaPos.z);
    }

    /*IObjectSceneNode* GLFontFace::RenderParagraph(Paragraph* p, const Float3& pos,
            ISceneGraph* sceneGraph, IGeomBuffer* gb, const char* styleParams)
    {
        IGLGeomBuffer* geomBuffer = static_cast<IGLGeomBuffer*>(gb);
        auto gc = geomBuffer->CreateGeomChunk();

        p_RenderParagraph(p, pos, geomBuffer, styleParams, gc);

        return sceneGraph->CreateObjectNode(RKReference(material), gb, gc);
    }

    void GLFontFace::RenderParagraph(Paragraph* p, const Float3& pos,
            IObjectSceneNode* obj, IGeomBuffer* gb, const char* styleParams)
    {
        IGLGeomBuffer* geomBuffer = static_cast<IGLGeomBuffer*>(gb);
        auto gc = obj->GetGeomChunk();
        
        p_RenderParagraph(p, pos, geomBuffer, styleParams, gc);
        gb->Release();
    }*/

    /*void GLFontFace::RenderParagraph(Paragraph* p, const Float3& pos, IGeomChunk* gc, const char* styleParams) 
    {
        p_RenderParagraph(p, pos, styleParams, gc);
    }*/
    
    /*IObjectSceneNode* GLFontFace::RenderString(const char* textUtf8, int layoutFlags, const Float3& pos,
            ISceneGraph* sceneGraph, IGeomBuffer* gb, const char* styleParams)
    {
        IGLGeomBuffer* geomBuffer = static_cast<IGLGeomBuffer*>(gb);
        auto gc = geomBuffer->CreateGeomChunk();
        
        Paragraph* p = new Paragraph;
        LayoutParagraph(p, textUtf8, layoutFlags);
        p_RenderParagraph(p, pos, geomBuffer, styleParams, gc);
        delete p;
        
        return sceneGraph->CreateObjectNode(RKReference(material), RKReference(gb), gc);
    }
    
    void GLFontFace::RenderString(const char* textUtf8, int layoutFlags, const Float3& pos,
            IObjectSceneNode* obj, IGeomBuffer* gb, const char* styleParams)
    {
        IGLGeomBuffer* geomBuffer = static_cast<IGLGeomBuffer*>(gb);
        auto gc = geomBuffer->CreateGeomChunk();
        
        Paragraph* p = new Paragraph;
        LayoutParagraph(p, textUtf8, layoutFlags);
        p_RenderParagraph(p, pos, geomBuffer, styleParams, gc);
        delete p;
    }*/
}
