#define SHADOW_MAPPING
#include "RenderingKitImpl.hpp"

#include <RenderingKit/utility/BasicPainter.hpp>

#include <framework/colorconstants.hpp>

#include <littl/String.hpp>

namespace RenderingKit
{
    using namespace zfw;

    struct Buffer_t
    {
        shared_ptr<IGLTexture> texture;
        String nameInShader;
    };

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    class DeferredShaderBinding : public IDeferredShaderBinding
    {
        public:
            shared_ptr<IShader> shader;
            BasicPainter2D<zfw::Float2> bp;

            List<intptr_t> attachments;
            intptr_t lightPos, lightAmbient, lightDiffuse, lightRange, cameraPos;
            intptr_t shadowMap, shadowMatrix;
    };

    class DeferredShadingManager : public IGLDeferredShadingManager
    {
        public:
            virtual bool Init(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm) override;

            virtual void AddBufferByte4(Int2 size, const char* nameInShader) override;
            virtual void AddBufferFloat4(Int2 size, const char* nameInShader) override;

            virtual void BeginScene() override;
            virtual void EndScene() override;

            virtual void BeginDeferred() override;
            virtual void EndDeferred() override;

            virtual void InjectTexturesInto(IMaterial* material) override;

#if ZOMBIE_API_VERSION < 201701
            virtual unique_ptr<IDeferredShaderBinding> CreateShaderBinding(shared_ptr<IShader> shader) override;
            virtual void BeginShading(IDeferredShaderBinding* binding, const Float3& cameraPos) override;
            virtual void DrawPointLight(const Float3& pos, const Float3& ambient, const Float3& diffuse, float range,
                    ITexture* shadowMap, const glm::mat4x4& shadowMatrix) override;
            virtual void EndShading() override { EndDeferred(); }
#endif

        private:
            ErrorBuffer_t* eb;
            RenderingKit* rk;
            IRenderingManagerBackend* rm;

            shared_ptr<IGLRenderBuffer> renderBuffer;
            DeferredShaderBinding* binding;

            List<Buffer_t> buffers;

            Float3 cameraPos;
    };

    // ====================================================================== //
    //  class DeferredShadingManager
    // ====================================================================== //

    shared_ptr<IGLDeferredShadingManager> p_CreateGLDeferredShadingManager()
    {
        return std::make_shared<DeferredShadingManager>();
    }

    bool DeferredShadingManager::Init(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm)
    {
        this->eb = eb;
        this->rk = rk;
        this->rm = rm;

        renderBuffer = p_CreateGLRenderBuffer();

        Int2 size = Int2(1280, 720); // FIXME Resolution hack

        if (!renderBuffer->Init(eb, rk, rm, "DeferredShadingManager/renderBuffer", size))
            return false;

        renderBuffer->GLCreateDepthBuffer(size);

        return true;
    }

    void DeferredShadingManager::AddBufferByte4(Int2 size, const char* nameInShader)
    {
        auto& buffer = buffers.addEmpty();

        buffer.texture = p_CreateTexture(eb, rk, rm, "DeferredShadingManager/buffer[]");
        buffer.texture->SetContentsUndefined(size, kTextureNoMipmaps, kTextureRGBA8);

        buffer.nameInShader = nameInShader;

        renderBuffer->GLAddColourAttachment(buffer.texture);
    }

    void DeferredShadingManager::AddBufferFloat4(Int2 size, const char* nameInShader)
    {
        auto& buffer = buffers.addEmpty();

        buffer.texture = p_CreateTexture(eb, rk, rm, "DeferredShadingManager/buffer[]");
        buffer.texture->SetContentsUndefined(size, kTextureNoMipmaps, kTextureRGBAFloat);

        buffer.nameInShader = nameInShader;

        renderBuffer->GLAddColourAttachment(buffer.texture);
    }

#if ZOMBIE_API_VERSION < 201701
    unique_ptr<IDeferredShaderBinding> DeferredShadingManager::CreateShaderBinding(shared_ptr<IShader> shader)
    {
        unique_ptr<DeferredShaderBinding> binding(new DeferredShaderBinding());
        binding->shader = shader;

        // FIXME: Releasing
        if (!binding->bp.InitWithShader(rm, binding->shader))
            return nullptr;

        iterate2 (i, buffers)
            binding->attachments.add(shader->GetUniformLocation((*i).nameInShader));

        binding->lightPos = shader->GetUniformLocation("lightPos");
        binding->lightAmbient = shader->GetUniformLocation("lightAmbient");
        binding->lightDiffuse = shader->GetUniformLocation("lightDiffuse");
        binding->lightRange = shader->GetUniformLocation("lightRange");

        binding->cameraPos = shader->GetUniformLocation("cameraPos");

        binding->shadowMap = shader->GetUniformLocation("shadowMap");
        binding->shadowMatrix = shader->GetUniformLocation("shadowMatrix");

        return std::move(binding);
    }
#endif

    void DeferredShadingManager::BeginDeferred()
    {
        rm->SetProjection(glm::ortho(-1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f));

        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        rm->SetClearColour(RGBA_BLACK);
        rm->ClearBuffers(true, false, false);
    }

    void DeferredShadingManager::BeginScene()
    {
        rm->PushRenderBuffer(renderBuffer.get());

        // Specify what to render an start acquiring
        // FIXME: this must match attachments.length
        GLenum buffers[] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT, 
                         GL_COLOR_ATTACHMENT2_EXT };
        glDrawBuffers(3, buffers);
    }

#if ZOMBIE_API_VERSION < 201701
    void DeferredShadingManager::BeginShading(IDeferredShaderBinding* binding_in, const Float3& cameraPos)
    {
        // TODO: Evaluate passing of these
        this->cameraPos = cameraPos;

        binding = static_cast<DeferredShaderBinding*>(binding_in);

        rm->SetProjection(glm::ortho(-1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f));

        iterate2 (i, buffers)
        {
            GLStateTracker::BindTexture(i.getIndex(), (*i).texture->GLGetTex());
            binding->shader->SetUniformInt(binding->attachments[i.getIndex()], i.getIndex());
        }

        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        rm->Clear();
    }

    void DeferredShadingManager::DrawPointLight(const Float3& pos, const Float3& ambient, const Float3& diffuse,
            float range, ITexture* shadowMap, const glm::mat4x4& shadowMatrix)
    {
        rm->VertexCacheFlush();

        binding->shader->SetUniformFloat3(binding->lightPos, pos);
        binding->shader->SetUniformFloat3(binding->lightAmbient, ambient);
        binding->shader->SetUniformFloat3(binding->lightDiffuse, diffuse);
        binding->shader->SetUniformFloat(binding->lightRange, range);
        binding->shader->SetUniformFloat3(binding->cameraPos, cameraPos);

#ifdef SHADOW_MAPPING
        GLStateTracker::BindTexture(3, static_cast<IGLTexture*>(shadowMap)->GLGetTex());
        binding->shader->SetUniformInt(binding->shadowMap, 3);
        binding->shader->SetUniformMat4x4(binding->shadowMatrix, shadowMatrix);
#endif

        binding->bp.DrawFilledRectangle(Float2(-1.0f, -1.0f), Float2(2.0f, 2.0f), RGBA_WHITE);
    }
#endif

    void DeferredShadingManager::EndDeferred()
    {
        rm->VertexCacheFlush();
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void DeferredShadingManager::EndScene()
    {
        rm->PopRenderBuffer();

        // Specify what to render an start acquiring
        //GLenum buffers[] = { GL_COLOR_ATTACHMENT0_EXT };
        //glDrawBuffers(1, buffers);
        glDrawBuffer(GL_BACK);
    }

    void DeferredShadingManager::InjectTexturesInto(IMaterial* material)
    {
        for (auto& buffer : buffers)
            material->SetTexture(buffer.nameInShader, buffer.texture.get());
    }
}
