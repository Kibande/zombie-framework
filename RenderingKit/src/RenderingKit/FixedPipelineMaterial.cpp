
#include "RenderingKitImpl.hpp"

#include <framework/resource.hpp>

namespace RenderingKit
{
    using namespace zfw;

    class GLFPMaterial : public IGLMaterial, public IFPMaterial,
        public std::enable_shared_from_this<GLFPMaterial>
    {
        zfw::ErrorBuffer_t* eb;
        RenderingKit* rk;
        IRenderingManagerBackend* rm;
        String name;
        int flags;

        Byte4 colour;

        unsigned int numTextures;
        shared_ptr<IGLTexture> textures[MAX_TEX];

        public:
            GLFPMaterial(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name, int flags);

            virtual const char* GetName() override { return name.c_str(); }

            virtual void SetTexture(const char* name, shared_ptr<ITexture>&& texture) override { ZFW_ASSERT(false) }

            virtual void GLSetup(const MaterialSetupOptions& options, const glm::mat4x4& projection, const glm::mat4x4& modelView) override;
            //virtual void GLSetVertexFormat(GLVertexFormat* fmt) { this->vf = fmt; }

            // IFPMaterial
            virtual shared_ptr<IMaterial> GetMaterial() override { return shared_from_this(); }

            virtual int GetFlags() override { return flags; }
            virtual unsigned int GetMaxNumTextures() override { return MAX_TEX; }
            virtual void SetColour(const Byte4& colour) override { this->colour = colour; }
            virtual void SetNumTextures(unsigned int numTextures) override { this->numTextures = std::min<unsigned int>(numTextures, MAX_TEX); }
            virtual void SetTexture(unsigned int index, shared_ptr<ITexture>&& texture) override;
    };

    shared_ptr<IFPMaterial> p_CreateFPMaterial(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name, int flags)
    {
        return std::make_shared<GLFPMaterial>(eb, rk, rm, name, flags);
    }

    GLFPMaterial::GLFPMaterial(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name, int flags)
            : name(name)
    {
        this->eb = eb;
        this->rk = rk;
        this->rm = rm;
        this->flags = flags;

        colour = RGBA_WHITE;
        numTextures = 0;

        memset(textures, 0, sizeof(textures));
    }

    void GLFPMaterial::GLSetup(const MaterialSetupOptions& options, const glm::mat4x4& projection, const glm::mat4x4& modelView)
    {
        glMatrixMode( GL_PROJECTION );
        glLoadMatrixf( &projection[0][0] );

        glMatrixMode( GL_MODELVIEW );
        glLoadMatrixf( &modelView[0][0] );

        GLStateTracker::UseProgram(0);

        glColor4ubv(&colour[0]);

        if (options.type == MaterialSetupOptions::kSingleTextureOverride)
        {
            ZFW_ASSERT(numTextures == 1);

            GLStateTracker::SetEnabledTextureUnits(0, 1);
            static_cast<IGLTexture*>(options.data.singleTex.tex)->GLBind(0);
        }
        else
        {
            GLStateTracker::SetEnabledTextureUnits(0, numTextures);

            for (unsigned int i = 0; i < numTextures; i++)
                textures[i]->GLBind(i);
        }
    }

    void GLFPMaterial::SetTexture(unsigned int index, shared_ptr<ITexture>&& texture)
    {
        if (index < MAX_TEX)
            textures[index] = static_pointer_cast<IGLTexture>(move(texture));
    }
}
