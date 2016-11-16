
#include "RenderingKitImpl.hpp"

#include <littl/String.hpp>

namespace RenderingKit
{
    using namespace zfw;

    class GLRenderBuffer : public IGLRenderBuffer
    {
        public:
            GLRenderBuffer();
            ~GLRenderBuffer();

            virtual bool Init(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm,
                    const char* name, Int2 viewportSize) override;

            virtual const char* GetName() override { return name; }
            virtual Int2 GetViewportSize() override { return viewportSize; }
            virtual shared_ptr<ITexture> GetTexture() override { return colourAttachments[0]; }
            virtual shared_ptr<ITexture> GetDepthTexture() override { return depthTex; }

            virtual void GLAddColourAttachment(shared_ptr<IGLTexture> tex) override;
            virtual void GLCreateDepthBuffer(Int2 size) override;
            virtual GLuint GLGetFBO() override { return fbo; }
            //virtual IGLTexture* GLGetTexture() override { return colourAttachments[0]; }
            virtual void GLSetDepthAttachment(shared_ptr<IGLTexture> tex) override;

        private:
            IRenderingManagerBackend* rm;
            String name;

            Int2 viewportSize;

            shared_ptr<IGLTexture> depthTex;
            List<shared_ptr<IGLTexture>> colourAttachments;

            GLuint fbo;
            GLuint depth;
    };

    // ====================================================================== //
    //  class GLRenderBuffer
    // ====================================================================== //

    shared_ptr<IGLRenderBuffer> p_CreateGLRenderBuffer()
    {
        return std::make_shared<GLRenderBuffer>();
    }

    GLRenderBuffer::GLRenderBuffer()
    {
        fbo = 0;
        depth = 0;
    }

    GLRenderBuffer::~GLRenderBuffer()
    {
        glDeleteFramebuffers(1, &fbo);
        glDeleteRenderbuffers(1, &depth);
    }

    bool GLRenderBuffer::Init(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name, Int2 viewportSize)
    {
        ZFW_ASSERT(glGenFramebuffers != nullptr)

        this->rm = rm;
        this->viewportSize = viewportSize;

        glGenFramebuffers( 1, &fbo );
        rm->CheckErrors(li_functionName);

        return true;
    }

    void GLRenderBuffer::GLAddColourAttachment(shared_ptr<IGLTexture> tex)
    {
        rm->PushRenderBuffer( this );

        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colourAttachments.getLength(), GL_TEXTURE_2D, tex->GLGetTex(), 0);
        rm->CheckErrors(li_functionName);

        rm->PopRenderBuffer();

        colourAttachments.add(move(tex));
    }

    void GLRenderBuffer::GLSetDepthAttachment(shared_ptr<IGLTexture> tex)
    {
        rm->PushRenderBuffer( this );
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex->GLGetTex(), 0);
        rm->PopRenderBuffer();

        depthTex = move(tex);
    }

    void GLRenderBuffer::GLCreateDepthBuffer(Int2 size)
    {
        rm->PushRenderBuffer( this );
        glGenRenderbuffers( 1, &depth );
        glBindRenderbuffer( GL_RENDERBUFFER, depth );
        glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size.x, size.y );

        glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth );
        rm->PopRenderBuffer();
    }
}
