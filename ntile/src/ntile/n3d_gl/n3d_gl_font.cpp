
#include "n3d_gl_internal.hpp"

#include <framework/colorconstants.hpp>
#include <framework/errorbuffer.hpp>
#include <framework/utility/pixmap.hpp>

namespace n3d
{
    // 24 bytes
    struct FontVertex
    {
        int x, y, z;
        uint8_t rgba[4];
        float u, v;
    };
    
    GLFont::GLFont(String&& path, unsigned int size, int textureFlags, int loadFlags)
            : state(CREATED), path(std::forward<String>(path)), size(size), textureFlags(textureFlags), loadFlags(loadFlags)
    {
        texture = new GLTexture("", textureFlags);
    }

    GLFont::~GLFont()
    {
        Unrealize();
        Unload();

        delete texture;
    }

    bool GLFont::Preload(IResourceManager2* resMgr)
    {
        unique_ptr<InputStream> input(g_sys->OpenInput(path + ".bmf1"));

        if (input == nullptr)
            return ErrorBuffer::SetError3(EX_ASSET_OPEN_ERR, 2,
                        "desc", (const char*) sprintf_t<255>("Failed to load font '%s'.", path.c_str()),
                        "function", li_functionName
                        ), false;

        ZFW_ASSERT(input != nullptr)

        uint32_t kind;
        uint8_t numChars, h, spacing, space_width;

        ZFW_ASSERT(input->readLE<uint32_t>(&kind) && kind == 0x01)
        ZFW_ASSERT(input->readLE<uint8_t>(&numChars))
        ZFW_ASSERT(input->readLE<uint8_t>(&h))
        ZFW_ASSERT(input->readLE<uint8_t>(&spacing))
        ZFW_ASSERT(input->readLE<uint8_t>(&space_width))

        Array<uint8_t> charData;
        const size_t charDataSize = numChars * (2 + h * 2);

        charData.resize(charDataSize);
        ZFW_ASSERT(input->read(charData.c_array(), charDataSize) == charDataSize)

        unsigned texWidth = 0;
        uint8_t* p_charData = charData.c_array();

        for (unsigned i = 0; i < numChars; i++)
        {
            p_charData++;
            texWidth += 1 + (*p_charData++) + 1;
            p_charData += h * 2;
        }

        texWidth =              li::align<32>(texWidth);
        unsigned texHeight =    li::align<16>(h);

        Pixmap::Initialize(&pm, Int2(texWidth, texHeight), PixmapFormat_t::RGBA8);
        uint8_t* pixelData_in = Pixmap::GetPixelDataForWriting(&pm);

        unsigned x = 0;
        p_charData = charData.c_array();

        for (unsigned int i = 0; i < numChars; i++)
        {
            unsigned int index = *p_charData++;
            unsigned int width = *p_charData++;
            
            x++;

            Glyph& glyph = glyphs[index];
            glyph.width = width;
            glyph.uv[0] = Float2((float) (x) / texWidth, 0.0f);
            glyph.uv[1] = Float2((float) (x + width) / texWidth, (float) h / texHeight);

            for (unsigned int y = 0; y < h; y++)
            {
                unsigned int line = *p_charData++;
                line = line | (*p_charData++ << 8);

                uint8_t* pixelData = &pixelData_in[(y * pm.info.size.x + x) * 4];

                for (unsigned int xx = 0; xx < width; xx++)
                {
                    pixelData[0] = 0xFF;
                    pixelData[1] = 0xFF;
                    pixelData[2] = 0xFF;
                    pixelData[3] = (line & 1) * 0xFF;

                    pixelData += 4;
                    line >>= 1;
                }
            }

            glyph.width *= size;
            x += width + 1;
        }

        this->height = h * size;
        this->spacing = spacing * size;
        this->space_width = space_width * size;

        return true;
    }

    void GLFont::DrawText(int x1, int y1, int z1, int flags, const char* text, size_t length, Byte4 colour)
    {
        Int2 textSize;
        int x1_0;

        if (text == nullptr)
            return;

        textSize = MeasureText(text, flags);

        if (flags & ALIGN_HCENTER)
            x1 -= textSize.x / 2;
        else if (flags & ALIGN_RIGHT)
            x1 -= textSize.x;

        if (flags & ALIGN_VCENTER)
            y1 -= textSize.y / 2;
        else if (flags & ALIGN_BOTTOM)
            y1 -= textSize.y;

        if (length == 0 && text != NULL)
            length = strlen(text);

        x1_0 = x1;

        glr->SetColour(colour);

        while (length)
        {
            uint8_t c = *text++;

            if (c == '\n')
            {
                x1 = x1_0;
                y1 += height;
            }
            else if (c == ' ')
                x1 += space_width;
            else if (c < glyphs.getCapacity())
            {
                auto& glyph = glyphs[c];

                // TODO: Should we allocate once or every time?
                FontVertex* vertices = static_cast<FontVertex*>(
                        vertexCache->Alloc(this, 4 * sizeof(FontVertex)));
                
                vertices[0].x = x1;
                vertices[0].y = y1;
                vertices[0].z = 0;
                memcpy(&vertices[0].rgba[0], &colour[0], 4);
                vertices[0].u = glyph.uv[0].x;
                vertices[0].v = glyph.uv[0].y;
                
                vertices[1].x = x1;
                vertices[1].y = y1 + height;
                vertices[1].z = 0;
                memcpy(&vertices[1].rgba[0], &colour[0], 4);
                vertices[1].u = glyph.uv[0].x;
                vertices[1].v = glyph.uv[1].y;
                
                vertices[2].x = x1 + glyph.width;
                vertices[2].y = y1 + height;
                vertices[2].z = 0;
                memcpy(&vertices[2].rgba[0], &colour[0], 4);
                vertices[2].u = glyph.uv[1].x;
                vertices[2].v = glyph.uv[1].y;
                
                vertices[3].x = x1 + glyph.width;
                vertices[3].y = y1;
                vertices[3].z = 0;
                memcpy(&vertices[3].rgba[0], &colour[0], 4);
                vertices[3].u = glyph.uv[1].x;
                vertices[3].v = glyph.uv[0].y;
                
                //glr->DrawTexture(texture, Int2(x1, y1), Int2(glyph.width, height) * size, glyph.pos, glyph.size);

                x1 += glyph.width + spacing;
            }

            length--;
        }
        
        vertexCache->Flush();

        glr->SetColour(RGBA_WHITE);
    }

    Int2 GLFont::MeasureText(const char* text, int flags)
    {
        int width, height, maxw;

        if (text == nullptr)
            return Int2();

        width = 0;
        height = (*text) ? (this->height) : 0;
        maxw = 0;

        while (*text)
        {
            uint8_t c = *text++;

            if (c == '\n')
            {
                if (width > maxw)
                    maxw = width;

                width = 0;
                height += this->height;
            }
            else if (c == ' ')
                width += space_width + spacing;
            else if (c < glyphs.getCapacity())
                width += glyphs[c].width + spacing;
        }

        if (width > maxw)
            maxw = width;

        return Int2(maxw, height);
    }

    void GLFont::OnVertexCacheFlush(GLVertexBuffer* vb, size_t bytesUsed)
    {
        gl.SetState(ST_GL_TEXTURE_2D, true);
        glr->SetTextureUnit(0, texture);
        glr->DrawPrimitives(vb, PRIMITIVE_QUADS, glr->GetFontVertexFormat(), 0,
                (uint32_t)(bytesUsed / sizeof(FontVertex)));
    }

    bool GLFont::Realize(IResourceManager2* resMgr)
    {
        texture->InitLevel(0, pm);

        Pixmap::DropContents(&pm);

        return true;
    }

    void GLFont::Unload()
    {
        Pixmap::DropContents(&pm);
        glyphs.resize(0, false);
    }

    void GLFont::Unrealize()
    {
        if (texture)
            texture->Unrealize();
    }
}
