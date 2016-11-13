
#include "n3d_gl_internal.hpp"

#include <framework/errorbuffer.hpp>
#include <framework/pixmap.hpp>

#include <framework/utility/pixmap.hpp>

namespace n3d
{
    static bool GetOpenGLTextureFormat(PixmapFormat_t fmt, GLuint& format, GLuint& internal)
    {
        switch (fmt)
        {
            case PixmapFormat_t::RGB8:   format = GL_RGB;    internal = GL_RGBA8; break;
            case PixmapFormat_t::RGBA8:  format = GL_RGBA;   internal = GL_RGBA8; break;

            default:
                return ErrorBuffer::SetError3(EX_INVALID_ARGUMENT, 2,
                        "desc", (const char*) sprintf_t<63>("Unsupported bitmap format (media::TexFormat == %i)", fmt),
                        "function", li_functionName
                        ), false;
        }

        return true;
    }

    GLTexture::GLTexture(String&& path, int flags, unsigned int numMipmaps, float lodBias)
            : state(CREATED), path(std::forward<String>(path)), flags(flags), numMipmaps(numMipmaps), lodBias(lodBias)
    {
        alignment = 4;
        tex = 0;
    }

    void GLTexture::Bind()
    {
        gl.BindTexture0(tex);
        glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, lodBias);
    }
    
    bool GLTexture::InitLevel(int level, const Pixmap_t& pm)
    {
        if (level == 0)
            if (!PreInit())
                return false;

        GLenum format, internal;
        if (!GetOpenGLTextureFormat(pm.info.format, format, internal))
            return false;

        glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
        glTexImage2D(GL_TEXTURE_2D, level, internal, pm.info.size.x, pm.info.size.y, 0, format, GL_UNSIGNED_BYTE, &pm.pixelData[0]);
        // FIXME: Sanity check

        size = pm.info.size;
        return true;
    }

    bool GLTexture::Init(Int2 size)
    {
        if (!PreInit())
            return false;

        glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        this->size = size;
        return true;
    }

    bool GLTexture::PreInit()
    {
        zombie_assert(tex == 0);
        glGenTextures(1, &tex);

        gl.BindTexture0(tex);

        if (flags & TEXTURE_BILINEAR)
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
            glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE );

        return true;
    }
    
    bool GLTexture::Preload(IResourceManager2* resMgr)
    {
        if (state == PRELOADED || state == REALIZED)
            return true;

        if (!path.isEmpty() && numMipmaps == 1)
        {
            if (!Pixmap::LoadFromFile(g_sys, &pm, path))
                return ErrorBuffer::SetError3(EX_ASSET_OPEN_ERR, 2,
                        "desc", (const char*) sprintf_t<255>("Failed to load texture '%s'.", path.c_str()),
                        "function", li_functionName
                        ), false;
        }

        return true;
    }

    bool GLTexture::Realize(IResourceManager2* resMgr)
    {
        zombie_assert(tex == 0);
        zombie_assert(!path.isEmpty());

        if (numMipmaps == 1)
        {
            if (!InitLevel(0, pm))
                return false;
        }
        else
        {
            ZFW_ASSERT(numMipmaps < 16)

            char filename[FILENAME_MAX];

            for (unsigned i = 0; i < numMipmaps; i++)
            {
                // FIXME: This is a security hole waiting to blow up
                snprintf(filename, sizeof(filename), path, (int) i);

                Pixmap_t pm;

                if (!Pixmap::LoadFromFile(g_sys, &pm, filename))
                    return ErrorBuffer::SetError3(EX_ASSET_OPEN_ERR, 2,
                            "desc", (const char*) sprintf_t<255>("Failed to load texture '%s'.", filename),
                            "function", li_functionName
                            ), false;

                InitLevel(i, pm);
            }
        }

        return true;
    }
    /*
    bool GLTexture::Init(PixmapFormat_t fmt, Int2 size, const void* data)
    {
        if (!PreInit())
            return false;

        GLenum format, internal;
        if (!GetOpenGLTextureFormat(fmt, format, internal))
            return false;

        glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
        glTexImage2D(GL_TEXTURE_2D, 0, internal, size.x, size.y, 0, format, GL_UNSIGNED_BYTE, data);

        this->size = size;
        return true;
    }*/

    void GLTexture::Unload()
    {
        Pixmap::DropContents(&pm);
    }

    void GLTexture::Unrealize()
    {
        if (tex)
        {
            // TODO: ensure not bound
            glDeleteTextures(1, &tex);
            tex = 0;
        }
    }
}
