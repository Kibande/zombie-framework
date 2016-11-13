
#include "n3d_gl_internal.hpp"

namespace n3d
{
    GLFrameBuffer::GLFrameBuffer(Int2 size, bool withDepth)
    {
        glGenFramebuffers( 1, &fbo );
        glr->SetFrameBuffer( this );

        texture.reset(new GLTexture(String(), TEXTURE_BILINEAR, 1, 0.0f));
        texture->Init(size);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture->tex, 0);

        if (withDepth)
        {
            glGenRenderbuffers( 1, &depth );
            glBindRenderbuffer( GL_RENDERBUFFER, depth );
            glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size.x, size.y );

            glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth );
        }
        
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        ZFW_ASSERT(status == GL_FRAMEBUFFER_COMPLETE)

        glr->SetFrameBuffer(nullptr);
    }

    GLFrameBuffer::~GLFrameBuffer()
    {
        glDeleteFramebuffers(1, &fbo);
        glDeleteRenderbuffers(1, &depth);
    }

    bool GLFrameBuffer::GetSize(Int2& size)
    {
        if (texture != nullptr)
            size = texture->size;
        else
            return false;

        return true;
    }
}
