
#include "rendering.hpp"

namespace zr
{
    RenderBuffer::RenderBuffer(ITexture* tex)
            : buffer( 0 ), depth( 0 )
    {
        this->tex = static_cast<render::GLTexture *>(tex);

        glGenFramebuffers( 1, &buffer );
        R::PushRenderBuffer( this );

        glFramebufferTexture2D( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, this->tex->id, 0 );

        /*GLenum status = glCheckFramebufferStatus( GL_FRAMEBUFFER_EXT );

        if ( status != GL_FRAMEBUFFER_COMPLETE_EXT )
            ys::RaiseException( "OpenGlDriver.RenderBuffer.RenderBuffer", "RenderBufferIncomplete",
                    ( String ) "Framebuffer Status = " + String::formatInt( status, -1, String::hexadecimal ) + "h" );
        */

        ZFW_ASSERT(glCheckFramebufferStatus( GL_FRAMEBUFFER_EXT ) == GL_FRAMEBUFFER_COMPLETE_EXT)

        R::PopRenderBuffer();
    }

    RenderBuffer* RenderBuffer::Create(ITexture* tex)
    {
        return new RenderBuffer(tex);
    }

    RenderBuffer::~RenderBuffer()
    {
        glDeleteFramebuffers( 1, &buffer );
    }

    unsigned RenderBuffer::getHeight()
    {
        return tex->GetSize().y;
    }

    /*ITexture* RenderBuffer::getTexture()
    {
        return texture->reference();
    }*/

    unsigned RenderBuffer::getWidth()
    {
        return tex->GetSize().x;
    }

    bool RenderBuffer::isSetUp()
    {
        return tex != 0;
    }
}