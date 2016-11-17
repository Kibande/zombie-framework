
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
#if ZOMBIE_API_VERSION < 201701
            virtual intptr_t SetTexture(const char* name, shared_ptr<ITexture>&& texture) override;
#endif
            virtual intptr_t SetTexture(const char* name, ITexture* texture) override;
            virtual void SetTextureByIndex(intptr_t index, ITexture* texture) override;

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
            intptr_t p_GetTextureIndexByUniformIndex(size_t uniformIndex);

            struct Texture_t
            {
                IGLTexture* texture;
                shared_ptr<IGLTexture> textureHandle;
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
            // TODO - optimization: don't redo this on every GLSetup if nont necessary
            for (unsigned int i = 0; i < numTextures; i++)
            {
                textures[i].texture->GLBind(i);
                program->SetUniformInt(textures[i].samplerUniformIndex, i);
            }
        }

        BuiltinUniformsSetup(projection, modelView);
    }

    intptr_t GLMaterial::p_GetTextureIndexByUniformIndex(size_t uniformIndex)
    {
        for (size_t i = 0; i < numTextures; i++)
            if (textures[i].samplerUniformIndex == uniformIndex)
                return i;

        return -1;
    }

#if ZOMBIE_API_VERSION < 201701
    intptr_t GLMaterial::SetTexture(const char* name, shared_ptr<ITexture>&& texture)
    {
        intptr_t uniformIndex = program->GetUniformLocation(name);

        if (uniformIndex < 0)
            return -1;

        intptr_t index = p_GetTextureIndexByUniformIndex(uniformIndex);

        if (index < 0)
        {
            zombie_assert(numTextures < MAX_TEX);
            index = numTextures++;

            textures[index].samplerUniformIndex = uniformIndex;
        }

        textures[index].textureHandle = std::static_pointer_cast<IGLTexture>(texture);
        textures[index].texture = textures[index].textureHandle.get();
        return index;
    }
#endif

    intptr_t GLMaterial::SetTexture(const char* name, ITexture* texture)
    {
        intptr_t uniformIndex = program->GetUniformLocation(name);

        if (uniformIndex < 0)
            return -1;

        intptr_t index = p_GetTextureIndexByUniformIndex(uniformIndex);

        if (index < 0)
        {
            zombie_assert(numTextures < MAX_TEX);
            index = numTextures++;

            textures[index].samplerUniformIndex = uniformIndex;
        }

        textures[index].textureHandle = nullptr;
        textures[index].texture = static_cast<IGLTexture*>(texture);
        return index;
    }

    void GLMaterial::SetTextureByIndex(intptr_t index, ITexture* texture)
    {
        if (index >= 0)
        {
            textures[index].texture = static_cast<IGLTexture*>(texture);

            // TODO: should BindTexture if material is already active
            //GLStateTracker::BindTexture(index, texture->GLGetTex());
        }
    }
}
