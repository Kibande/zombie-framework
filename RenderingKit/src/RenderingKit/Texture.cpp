
#include "RenderingKitImpl.hpp"

#include <RenderingKit/RenderingKitUtility.hpp>

#include <framework/errorbuffer.hpp>
#include <framework/resource.hpp>
#include <framework/utility/pixmap.hpp>

#include <littl/String.hpp>

// FIXME: Ensure alignment of pixmaps

#ifdef ZOMBIE_WINNT
#include <malloc.h>
#endif

namespace RenderingKit
{
    using namespace zfw;

    static const GLint internalFormats[] = { GL_RGBA8, GL_RGBA32F, GL_DEPTH_COMPONENT };
    static const GLint wrapModes[] = { GL_CLAMP_TO_EDGE, GL_REPEAT };

    class GLTexture : public IGLTexture
    {
        zfw::ErrorBuffer_t* eb;
        RenderingKit* rk;
        IRenderingManagerBackend* rm;
        String path;

        RKTextureWrap_t wrap[2];

        public:
            GLTexture(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name);
            ~GLTexture();

            virtual const char* GetName() override { return path.c_str(); }

            virtual void SetWrapMode(int axis, RKTextureWrap_t mode) override { wrap[axis] = mode; }

            virtual bool GetContentsIntoPixmap(IPixmap* pixmap) override;
            virtual Int2 GetSize() override { return size; }
            virtual bool SetContentsFromPixmap(IPixmap* pixmap) override;

            virtual void GLBind(int unit) override;
            virtual GLuint GLGetTex() override { return handle; }
            virtual void SetContentsUndefined(Int2 size, int flags, RKTextureFormat_t format) override;
            virtual void TexSubImage(uint32_t x, uint32_t y, uint32_t w, uint32_t h, PixmapFormat_t pixmapFormat, const uint8_t* data) override;

            // IResource2
            bool BindDependencies(IResourceManager2* resMgr) { return true; }
            bool Preload(IResourceManager2* resMgr);
            void Unload();
            bool Realize(IResourceManager2* resMgr);
            void Unrealize();

            virtual void* Cast(const TypeID& resourceClass) final override { return DefaultCast(this, resourceClass); }

            virtual State_t GetState() const final override { return state; }

            virtual bool StateTransitionTo(State_t targetState, IResourceManager2* resMgr) final override
            {
                return DefaultStateTransitionTo(this, targetState, resMgr);
            }

        private:
            // Private methods
            GLuint p_CreateEmptyTexture(int flags, const RKTextureWrap_t wrap[2], bool isDepthTexture);
            static const uint8_t* ps_Flip(const uint8_t*& p_data, uint32_t width, uint32_t& height, uint32_t bytesPerPixel, uint32_t& count_out);
            static void ps_FlipInPlace(IPixmap* pixmap);
            bool p_ToGLFormat(PixmapFormat_t pixmapFormat, GLenum& format_out);

            State_t state = CREATED;

            // preloaded
            Pixmap_t pm;

            // realized
            GLuint handle;
            Int2 size;

        friend class IResource2;
    };

    shared_ptr<IGLTexture> p_CreateTexture(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name)
    {
        return std::make_shared<GLTexture>(eb, rk, rm, name);
    }

    unique_ptr<IGLTexture> p_CreateTextureUniquePtr(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name)
    {
        return std::make_unique<GLTexture>(eb, rk, rm, name);
    }

    GLTexture::GLTexture(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name)
            : path(name)
    {
        this->eb = eb;
        this->rk = rk;
        this->rm = rm;

        wrap[0] = kTextureWrapClamp;
        wrap[1] = kTextureWrapClamp;

        handle = 0;
    }

    GLTexture::~GLTexture()
    {
        Unrealize();
        Unload();
    }

    bool GLTexture::GetContentsIntoPixmap(IPixmap* pixmap)
    {
        zombie_assert_resource_state(REALIZED, path.c_str());

#ifndef RENDERING_KIT_USING_OPENGL_ES
        if (!pixmap->SetSize(size) || !pixmap->SetFormat(PixmapFormat_t::RGBA8))
            return false;

        uint8_t* buffer = pixmap->GetDataWritable();

        if (buffer == nullptr)
            return false;

        GLStateTracker::BindTexture(0, handle);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

        ps_FlipInPlace(pixmap);
        return true;
#else
		return ErrorBuffer::SetError(eb, EX_INVALID_OPERATION, "desc", "ITexture::GetContentsIntoPixmap is not supported in OpenGL ES mode.", nullptr),
				false;
#endif
    }

    void GLTexture::GLBind(int unit)
    {
        zombie_assert_resource_state(REALIZED, path.c_str());
        GLStateTracker::BindTexture(unit, handle);
    }

    GLuint GLTexture::p_CreateEmptyTexture(int flags, const RKTextureWrap_t wrap[2], bool isDepthTexture)
    {
        GLuint tex;
        glGenTextures(1, &tex);

        GLStateTracker::BindTexture(0, tex);

        if (flags & kTextureNoMipmaps)
        {
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        }
        else
        {
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        }

        if (!isDepthTexture)
        {
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapModes[wrap[0]] );
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapModes[wrap[1]] );
        }
        else
        {
            //glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
            //glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
        }

        // FIXME: Error Checking
        rm->CheckErrors(li_functionName);

        return tex;
    }

    const uint8_t* GLTexture::ps_Flip(const uint8_t*& p_data, uint32_t width, uint32_t& height, uint32_t bytesPerPixel, uint32_t& count_out)
    {
        enum { bufferSize = 256 * 1024 };
        static uint8_t buffer[bufferSize];

        const auto bytesPerLine = li::align<4>(width * bytesPerPixel);
        const auto linesToCopy = glm::min(bufferSize / bytesPerLine, height);

        uint8_t* p_buffer = buffer + linesToCopy * bytesPerLine;

        for (uint32_t y = 0; y < linesToCopy; y++)
        {
            p_buffer -= bytesPerLine;
            memcpy(p_buffer, p_data, bytesPerLine);
            p_data += bytesPerLine;
        }

        height -= linesToCopy;

        count_out = linesToCopy;
        return buffer;
    }

    void GLTexture::ps_FlipInPlace(IPixmap* pixmap)
    {
        const PixmapFormat_t pixmapFormat = pixmap->GetFormat();
        const Int2 size = pixmap->GetSize();
        const uint32_t Bpp = Pixmap::GetBytesPerPixel(pixmapFormat);
        const auto bytesPerLine = li::align<4>(size.x * Bpp);
        
        uint8_t* lineBuffer = (uint8_t*) alloca(bytesPerLine);
        uint8_t* bytes = pixmap->GetDataWritable();

        for (int y = 0; y < size.y / 2; y++)
        {
            memcpy(lineBuffer, bytes + y * bytesPerLine, bytesPerLine);
            memcpy(bytes + y * bytesPerLine, bytes + (size.y - y - 1) * bytesPerLine, bytesPerLine);
            memcpy(bytes + (size.y - y - 1) * bytesPerLine, lineBuffer, bytesPerLine);
        }
    }

    bool GLTexture::p_ToGLFormat(PixmapFormat_t pixmapFormat, GLenum& format_out)
    {
        switch ( pixmapFormat )
        {
            case PixmapFormat_t::RGB8: format_out = GL_RGB; break;
            case PixmapFormat_t::RGBA8: format_out = GL_RGBA; break;

            default:
                return ErrorBuffer::SetError(eb, EX_ASSET_FORMAT_UNSUPPORTED, "desc", "Unsupported image format.",
                        "format", (const char*) sprintf_t<15>("%i", static_cast<int>(pixmapFormat)),
                        nullptr),
                        false;
        }

        return true;
    }

    bool GLTexture::Preload(IResourceManager2* resMgr)
    {
        if (state == PRELOADED || state == REALIZED)
            return true;

        if (!path.isEmpty())
        {
            if (!Pixmap::LoadFromFile(rk->GetEngine(), &pm, path))
                return ErrorBuffer::SetError3(EX_ASSET_OPEN_ERR, 2,
                        "desc", (const char*) sprintf_t<255>("Failed to load texture '%s'.", path.c_str()),
                        "function", li_functionName
                ), false;
        }

        return true;
    }

    bool GLTexture::Realize(IResourceManager2* resMgr)
    {
        //zombie_assert_resource_state(PRELOADED, path.c_str());
        zombie_assert(!path.isEmpty());

        PixmapWrapper wrapper(&pm, false);
        return SetContentsFromPixmap(&wrapper);
    }

    void GLTexture::SetContentsUndefined(Int2 size, int flags, RKTextureFormat_t format)
    {
        Unrealize();

#ifdef RENDERING_KIT_USING_OPENGL_ES
		zombie_assert(format == RKTextureFormat_t::kTextureRGBA8);

		const auto fmt = GL_RGBA;
#else
		const auto fmt = internalFormats[format];
#endif

        switch (format)
        {
            case kTextureRGBA8:
                handle = p_CreateEmptyTexture(flags, wrap, false);
                glTexImage2D(GL_TEXTURE_2D, 0, fmt, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                break;

            case kTextureRGBAFloat:
                handle = p_CreateEmptyTexture(flags, wrap, false);
                glTexImage2D(GL_TEXTURE_2D, 0, fmt, size.x, size.y, 0, GL_RGBA, GL_FLOAT, nullptr);
                break;

            case kTextureDepth:
                handle = p_CreateEmptyTexture(flags, wrap, true);
                glTexImage2D( GL_TEXTURE_2D, 0, fmt, size.x, size.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr );
                break;
        }

        rm->CheckErrors(li_functionName);

        this->size = size;
        this->state = REALIZED;
    }

    bool GLTexture::SetContentsFromPixmap(IPixmap* pixmap)
    {
        Unrealize();

        const PixmapFormat_t pixmapFormat = pixmap->GetFormat();
        GLenum format;

        if (!p_ToGLFormat(pixmapFormat, format))
            return false;

        const Int2 size = pixmap->GetSize();

        handle = p_CreateEmptyTexture(0, wrap, false);
        //glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, format, GL_UNSIGNED_BYTE, pixmap->GetData() );

        // TODO: Better way to flip the texture?
        // FIXME: Error Checking
        
        const uint32_t Bpp = Pixmap::GetBytesPerPixel(pixmapFormat);

        const uint8_t* p_data = pixmap->GetData();
        uint32_t linesRemaining = size.y;
        uint32_t linesReady;

        const uint8_t* flipped = ps_Flip(p_data, size.x, linesRemaining, Bpp, linesReady);

#ifdef RENDERING_KIT_USING_OPENGL_ES
		const auto fmt = format;
#else
		const auto fmt = GL_RGBA8;
#endif

        if (linesRemaining == 0)
            glTexImage2D( GL_TEXTURE_2D, 0, fmt, size.x, size.y, 0, format, GL_UNSIGNED_BYTE, flipped );
        else
        {
            glTexImage2D( GL_TEXTURE_2D, 0, fmt, size.x, size.y, 0, format, GL_UNSIGNED_BYTE, NULL );
            glTexSubImage2D( GL_TEXTURE_2D, 0, 0, linesRemaining, size.x, linesReady,format, GL_UNSIGNED_BYTE, flipped );
        }

        while (linesRemaining > 0)
        {
            flipped = ps_Flip(p_data, size.x, linesRemaining, Bpp, linesReady);
            glTexSubImage2D( GL_TEXTURE_2D, 0, 0, linesRemaining, size.x, linesReady,format, GL_UNSIGNED_BYTE, flipped );
        }

        glGenerateMipmap(GL_TEXTURE_2D);

        rm->CheckErrors(li_functionName);

        this->size = size;
        this->state = REALIZED;

        return true;
    }

    void GLTexture::TexSubImage(uint32_t x, uint32_t y, uint32_t w, uint32_t h, PixmapFormat_t pixmapFormat, const uint8_t* data)
    {
        zombie_assert_resource_state(REALIZED, path.c_str());

        GLenum format;

        if (!p_ToGLFormat(pixmapFormat, format))
            return;

        const uint32_t Bpp = Pixmap::GetBytesPerPixel(pixmapFormat);

        GLStateTracker::BindTexture(0, handle);

        const uint8_t* p_data = data;
        uint32_t linesRemaining = h;
        uint32_t linesReady;

        while (linesRemaining > 0)
        {
            const uint8_t* flipped = ps_Flip(p_data, w, linesRemaining, Bpp, linesReady);
            glTexSubImage2D( GL_TEXTURE_2D, 0, x, linesRemaining + (size.y - y - h), w, linesReady, format, GL_UNSIGNED_BYTE, flipped );
        }

        rm->CheckErrors(li_functionName);
    }

    void GLTexture::Unload()
    {
        Pixmap::DropContents(&pm);
    }

    void GLTexture::Unrealize()
    {
        if (handle != 0)
        {
            GLStateTracker::InvalidateTexture(handle);

            glDeleteTextures(1, &handle);
            handle = 0;
        }
    }
}
