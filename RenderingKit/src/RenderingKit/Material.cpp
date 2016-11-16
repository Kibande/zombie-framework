
#include "RenderingKitImpl.hpp"

#include <framework/errorbuffer.hpp>
#include <framework/resource.hpp>

#include <littl/String.hpp>

namespace RenderingKit
{
    using namespace zfw;

    class GLMaterial : public IGLMaterial
    {
        public:
            GLMaterial(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name,
                    shared_ptr<IGLShaderProgram>&& program);

            virtual const char* GetName() override { return name.c_str(); }

            virtual void SetTexture(const char* name, shared_ptr<ITexture>&& texture) override;

            virtual void GLSetup(const MaterialSetupOptions& options, const glm::mat4x4& projection, const glm::mat4x4& modelView) override;

        private:
            void BuiltinUniformsInitialize();
            void BuiltinUniformsSetup(const glm::mat4x4& projection, const glm::mat4x4& modelView);

            struct Texture_t
            {
                shared_ptr<IGLTexture> texture;
                intptr_t samplerUniformIndex;
            };

            zfw::ErrorBuffer_t* eb;
            RenderingKit* rk;
            IRenderingManagerBackend* rm;
            String name;

            shared_ptr<IGLShaderProgram> program;

            unsigned int numTextures;
            Texture_t textures[MAX_TEX];

            // OpenGL 3 Core NEW
            intptr_t u_ModelViewProjectionMatrix, u_ProjectionMatrix;
    };

    shared_ptr<IGLMaterial> p_CreateMaterial(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name,
            shared_ptr<IGLShaderProgram> program)
    {
        ZFW_DBGASSERT(program != nullptr)

        return std::make_shared<GLMaterial>(eb, rk, rm, name, move(program));
    }

    GLMaterial::GLMaterial(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name,
            shared_ptr<IGLShaderProgram>&& program)
            : name(name)
    {
        this->eb = eb;
        this->rk = rk;
        this->rm = rm;

        this->program = move(program);

        numTextures = 0;

        BuiltinUniformsInitialize();
    }

    void GLMaterial::BuiltinUniformsInitialize()
    {
        u_ModelViewProjectionMatrix = program->GetUniformLocation("u_ModelViewProjectionMatrix");
        u_ProjectionMatrix = program->GetUniformLocation("u_ProjectionMatrix");
    }

    void GLMaterial::BuiltinUniformsSetup(const glm::mat4x4& projection, const glm::mat4x4& modelView)
    {
        if (u_ModelViewProjectionMatrix >= 0)
        {
            auto modelViewProjection = (projection * modelView);
            program->SetUniformMat4x4(u_ModelViewProjectionMatrix, modelViewProjection);
        }

        if (u_ProjectionMatrix >= 0)
            program->SetUniformMat4x4(u_ProjectionMatrix, projection);
    }

    void GLMaterial::GLSetup(const MaterialSetupOptions& options, const glm::mat4x4& projection, const glm::mat4x4& modelView)
    {
        program->GLSetup();

        if (options.type == MaterialSetupOptions::kSingleTextureOverride)
        {
            ZFW_ASSERT(numTextures <= 1)

            if (numTextures == 1)
            {
                static_cast<IGLTexture*>(options.data.singleTex.tex)->GLBind(0);
                program->SetUniformInt(textures[0].samplerUniformIndex, 0);
            }
        }
        else
        {
            for (unsigned int i = 0; i < numTextures; i++)
            {
                textures[i].texture->GLBind(i);
                program->SetUniformInt(textures[i].samplerUniformIndex, i);
            }
        }

        BuiltinUniformsSetup(projection, modelView);
    }

    void GLMaterial::SetTexture(const char* name, shared_ptr<ITexture>&& texture)
    {
        intptr_t index = program->GetUniformLocation(name);

        if (index >= 0)
        {
            auto& entry = textures[numTextures++];
            entry.texture = std::static_pointer_cast<IGLTexture>(texture);
            entry.samplerUniformIndex = index;
        }
    }
}
