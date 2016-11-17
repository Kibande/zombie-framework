
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
            GLMaterial(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name,
                   IGLShaderProgram* program);

            virtual const char* GetName() override { return name.c_str(); }

            virtual IShader* GetShader() override { return program; }
            virtual void SetTexture(const char* name, shared_ptr<ITexture>&& texture) override;

            virtual void GLSetup(const MaterialSetupOptions& options, const glm::mat4x4& projection, const glm::mat4x4& modelView) override;

            // IResource2
            bool BindDependencies(IResourceManager2* resMgr) { return true; }
            bool Preload(IResourceManager2* resMgr) { return true; }
            void Unload() {}
            bool Realize(IResourceManager2* resMgr) { return true; }
            void Unrealize() {}

            virtual void* Cast(const TypeID& resourceClass) final override { return DefaultCast(this, resourceClass); }

            virtual State_t GetState() const final override { return state; }

            virtual bool StateTransitionTo(State_t targetState, IResourceManager2* resMgr) final override
            {
                return DefaultStateTransitionTo(this, targetState, resMgr);
            }

        private:
            void BuiltinUniformsInitialize();
            void BuiltinUniformsSetup(const glm::mat4x4& projection, const glm::mat4x4& modelView);

            struct Texture_t
            {
                shared_ptr<IGLTexture> texture;
                intptr_t samplerUniformIndex;
            };

            State_t state = CREATED;

            zfw::ErrorBuffer_t* eb;
            RenderingKit* rk;
            IRenderingManagerBackend* rm;
            String name;

            IGLShaderProgram* program;
            shared_ptr<IGLShaderProgram> programReference;

            unsigned int numTextures = 0;
            Texture_t textures[MAX_TEX];

            // OpenGL 3 Core NEW
            intptr_t u_ModelViewProjectionMatrix, u_ProjectionMatrix;

        friend class IResource2;
    };

    shared_ptr<IGLMaterial> p_CreateMaterial(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name,
                                             shared_ptr<IGLShaderProgram> program)
    {
        zombie_assert(program != nullptr);

        return std::make_shared<GLMaterial>(eb, rk, rm, name, move(program));
    }

    unique_ptr<IGLMaterial> p_CreateMaterialUniquePtr(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name,
                                                      IGLShaderProgram* program)
    {
        zombie_assert(program != nullptr);

        return std::make_unique<GLMaterial>(eb, rk, rm, name, program);
    }

    GLMaterial::GLMaterial(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name,
            shared_ptr<IGLShaderProgram>&& program)
            : name(name)
    {
        this->eb = eb;
        this->rk = rk;
        this->rm = rm;

        this->programReference = move(program);
        this->program = programReference.get();

        BuiltinUniformsInitialize();
    }

    GLMaterial::GLMaterial(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name,
                           IGLShaderProgram* program)
            : name(name)
    {
        this->eb = eb;
        this->rk = rk;
        this->rm = rm;

        this->program = program;

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
