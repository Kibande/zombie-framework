
#include "n3d_ctr_internal.hpp"

#include <framework/errorbuffer.hpp>
#include <framework/pixmap.hpp>
#include <framework/utility/pixmap.hpp>

namespace n3d
{
    static void EncodeTextureData32(const uint8_t* buffer_in, uint8_t* data_out, unsigned int w, unsigned int h)
    {
        static const uint8_t tileOrder[64] = {
            0,1,8,9,2,3,10,11,
            16,17,24,25,18,19,26,27,
            4,5,12,13,6,7,14,15,
            20,21,28,29,22,23,30,31,
            32,33,40,41,34,35,42,43,
            48,49,56,57,50,51,58,59,
            36,37,44,45,38,39,46,47,
            52,53,60,61,54,55,62,63
        };

        const size_t stride = w * 4;

        for (unsigned int y = 0; y < h; y += 8)
        {
            for (unsigned int x = 0; x < w; x += 8)
            {
                const uint8_t* tile_in = &buffer_in[y * stride + x * 4];

                for (int k = 0; k < 8 * 8; k++)
                {
                    int i = tileOrder[k] % 8;
                    int j = (tileOrder[k] - i) / 8;

                    data_out[0] = tile_in[j * stride + i * 4 + 3];
                    data_out[1] = tile_in[j * stride + i * 4 + 2];
                    data_out[2] = tile_in[j * stride + i * 4 + 1];
                    data_out[3] = tile_in[j * stride + i * 4 + 0];
                    data_out += 4;
                }
            }
        }
    }

    static bool GetGPUTextureFormat(PixmapFormat_t fmt, GLenum& format, uint32_t& bpp)
    {
        switch (fmt)
        {
            //case PixmapFormat_t::RGB8:   format = GL_RGB;    internal = GL_RGBA8; break;
            case PixmapFormat_t::RGBA8:  format = GL_BLOCK_RGBA8_CTR;    bpp = 4; break;

            default:
                return ErrorBuffer::SetError3(EX_INVALID_ARGUMENT, 2,
                        "desc", (const char*) sprintf_t<63>("Unsupported bitmap format (media::TexFormat == %i)", fmt),
                        "function", li_functionName
                        ), false;
        }

        return true;
    }

    CTRTexture::CTRTexture(String&& path, int flags) : state(CREATED), path(path), flags(flags)
    {
        // FIXME: cleanup

        glInitTextureCTR(&tex);

        //lodBias = 0.0f;
    }
    
    bool CTRTexture::PreInit()
    {
        glBindTexture(GL_TEXTURE_2D, (GLuint) &tex);

       /* if (flags & TEXTURE_BILINEAR)
        {
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (flags & TEXTURE_MIPMAPPED) ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        }
        else
        {
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (flags & TEXTURE_MIPMAPPED) ? GL_NEAREST_MIPMAP_LINEAR : GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
        }

        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

        if (flags & TEXTURE_AUTO_MIPMAPS)
            glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE );*/

        return true;
    }

    bool CTRTexture::InitLevel(int level, const Pixmap_t& pm)
    {
        if (level == 0)
            if (!PreInit())
                return false;

        uint32_t bpp;
        if (!GetGPUTextureFormat(pm.info.format, internalFormat, bpp))
            return false;

        size = pm.info.size;

        //glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
        glNamedTexImage2DCTR((GLuint) &tex, 0, internalFormat, size.x, size.y, 0,
                GL_BLOCK_RGBA_CTR, GL_UNSIGNED_BYTE, nullptr);

        EncodeTextureData32(&pm.pixelData[0], (u8*) tex.data, size.x, size.y);
        // FIXME: Sanity check

        return true;
    }

    bool CTRTexture::Init(PixmapFormat_t fmt, Int2 size, const void* data)
    {
        if (!PreInit())
            return false;

        uint32_t bpp;
        if (!GetGPUTextureFormat(fmt, internalFormat, bpp))
            return false;

        this->size = size;

        glNamedTexImage2DCTR((GLuint) &tex, 0, internalFormat, size.x, size.y, 0,
                GL_BLOCK_RGBA_CTR, GL_UNSIGNED_BYTE, data);

        return true;
    }

    /*bool GLTexture::Init(Int2 size)
    {
        if (!PreInit())
            return false;

        glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        this->size = size;
        return true;
    }*/

    void CTRTexture::Bind()
    {
        //GPU_SetTexture(unit, (u32*)osConvertVirtToPhys((u32) data), size.x, size.y,
        //        GPU_TEXTURE_MAG_FILTER(GPU_LINEAR) | GPU_TEXTURE_MIN_FILTER(GPU_NEAREST), gpuFormat);

        glBindTexture(GL_TEXTURE_2D, (GLuint) &tex);

        //glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, lodBias);
    }

    bool CTRTexture::Preload(IResourceManager2* resMgr)
    {
        if (state == PRELOADED || state == REALIZED)
            return true;

        if (!path.isEmpty())
        {
            Pixmap_t pm;

            if (!Pixmap::LoadFromFile(g_sys, &pm, path))
                return ErrorBuffer::SetError3(EX_ASSET_OPEN_ERR, 2,
                        "desc", (const char*) sprintf_t<255>("Failed to load texture '%s'.", path.c_str()),
                        "function", li_functionName
                        ), false;

            if (!PreInit())
                return false;

            uint32_t bpp;
            if (!GetGPUTextureFormat(pm.info.format, internalFormat, bpp))
                return false;

            this->size = pm.info.size;

            zombie_assert(size.x % 4 == 0);
            zombie_assert(size.y % 4 == 0);
            zombie_assert(bpp == 4);

            glNamedTexImage2DCTR((GLuint) &tex, 0, internalFormat, size.x, size.y, 0,
                    GL_BLOCK_RGBA_CTR, GL_UNSIGNED_BYTE, nullptr);

            EncodeTextureData32(&pm.pixelData[0], (u8*) tex.data, size.x, size.y);

            return true;
        }
        else
        {
            if (!PreInit())
                return false;
        }

        return true;
    }

    bool CTRTexture::Realize(IResourceManager2* resMgr)
    {
        return true;
    }

    void CTRTexture::Unload()
    {
    }

    void CTRTexture::Unrealize()
    {
    }
}
