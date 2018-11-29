
#include "RenderingKitImpl.hpp"
#include <RenderingKit/RenderingKitUtility.hpp>
#include <RenderingKit/WorldGeometry.hpp>

#include <framework/resourcemanager.hpp>
#include <framework/resourcemanager2.hpp>
#include <framework/timer.hpp>
#include <framework/varsystem.hpp>

#include <gameui/uithemer.hpp>

#include <littl/Stack.hpp>

namespace RenderingKit
{
    using namespace zfw;
    using std::make_unique;

    // FIXME: Bounds checking for all usages
    static const GLenum primitiveTypeToGLMode[] = { GL_LINES, GL_TRIANGLES, GL_QUADS };

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    struct ShaderUniform
    {
        std::string name;

        ShaderValueVariant value;
    };

    class RenderingManager : public IRenderingManagerBackend, public IVertexCacheOwner,
                             public zfw::IResourceProvider, public zfw::IResourceProvider2
    {
        public:
            RenderingManager(zfw::ErrorBuffer_t* eb, RenderingKit* rk, CoordinateSystem coordSystem);
            ~RenderingManager();

#if ZOMBIE_API_VERSION < 201701
            virtual void RegisterResourceProviders(zfw::IResourceManager* res) override;
            virtual zfw::IResourceManager* GetSharedResourceManager() override;
#endif

            virtual void RegisterResourceProviders(zfw::IResourceManager2* res) override;
            virtual zfw::IResourceManager2* GetSharedResourceManager2() override { return sharedResourceManager2.get(); }

            virtual void BeginFrame() override;
            virtual void Clear() override;
            virtual void Clear(Float4 color) override;
            virtual void ClearBuffers(bool color, bool depth, bool stencil) override;
            virtual void ClearDepth() override;
            virtual void EndFrame(int ticksElapsed) override;
            virtual void SetClearColour(const Float4& colour) override;

            virtual shared_ptr<ICamera>        CreateCamera(const char* name) override;
            virtual shared_ptr<IDeferredShadingManager>    CreateDeferredShadingManager() override;
            virtual shared_ptr<IFontFace>      CreateFontFace(const char* name) override;
            virtual shared_ptr<IGeomBuffer>    CreateGeomBuffer(const char* name) override;
            virtual zfw::shared_ptr<IGraphics> CreateGraphicsFromTexture(shared_ptr<ITexture> texture) override;
            virtual zfw::shared_ptr<IGraphics> CreateGraphicsFromTexture2(shared_ptr<ITexture> texture, const Float2 uv[2]) override;
            unique_ptr<IMaterial>               CreateMaterial(const char* name, IShader* program) final;
            virtual shared_ptr<IMaterial>      CreateMaterial(const char* name, shared_ptr<IShader> program) override;
            virtual shared_ptr<IRenderBuffer>  CreateRenderBuffer(const char* name, Int2 size, int flags) override;
            virtual shared_ptr<IRenderBuffer>  CreateRenderBufferWithTexture(const char* name, shared_ptr<ITexture> texture, int flags) override;
            virtual shared_ptr<ITexture>       CreateTexture(const char* name) override;
            virtual shared_ptr<ITextureAtlas>  CreateTextureAtlas2D(const char* name, Int2 size) override;

            virtual shared_ptr<IVertexFormat>  CompileVertexFormat(IShader* program, uint32_t vertexSize,
                    const VertexAttrib_t* attributes, bool groupedByAttrib) override;

            virtual void DrawPrimitives(IMaterial* material, RKPrimitiveType_t primitiveType, IGeomChunk* gc) override;
            void DrawPrimitivesTransformed(IMaterial* material, RKPrimitiveType_t primitiveType, IGeomChunk* gc,
                                           const glm::mat4x4& transform) final;


            virtual Int2 GetViewportSize() override { return viewportSize; }
            virtual void GetViewportPosAndSize(Int2* viewportPos_out, Int2* viewportSize_out) override;
            virtual void PopRenderBuffer() override;
            virtual void PushRenderBuffer(IRenderBuffer* rb) override;
            virtual bool ReadPixel(Int2 posInFB, Byte4* colour_outOrNull, float* depth_outOrNull) override;
            virtual void SetCamera(ICamera* camera) override;
            virtual void SetMaterialOverride(IMaterial* materialOrNull) override;
            virtual void SetProjection(const glm::mat4x4& projection) override;
            virtual void SetRenderState(RKRenderStateEnum_t state, int value) override;
            virtual void SetProjectionOrthoScreenSpace(float znear, float zfar) override;
            virtual void SetViewportPosAndSize(Int2 viewportPos, Int2 viewportSize) override;

            virtual void FlushMaterial(IMaterial* material) override { VertexCacheFlush(); }
            virtual void FlushCachedMaterialOptions() override { VertexCacheFlush(); }
            virtual void* VertexCacheAlloc(IVertexFormat* vertexFormat, IMaterial* material,
                    RKPrimitiveType_t primitiveType, size_t numVertices) override;
            virtual void* VertexCacheAlloc(IVertexFormat* vertexFormat, IMaterial* material,
                    const MaterialSetupOptions& options, RKPrimitiveType_t primitiveType, size_t numVertices) override;
            virtual void VertexCacheFlush() override;

            // Shader Globals
            intptr_t RegisterShaderGlobal(const char* name) override;
            void SetShaderGlobal(intptr_t handle, Float3 value) override;
            void SetShaderGlobal(intptr_t handle, Float4 value) override;

            // RenderingKit::IRenderingManagerBackend
            virtual bool Startup() override;
            virtual bool CheckErrors(const char* caller) override;

            virtual void OnWindowResized(Int2 newSize) override;
            virtual void SetupMaterial(IGLMaterial* material, const MaterialSetupOptions& options) override;

            void GetModelViewProjectionMatrices(glm::mat4x4** projection_out, glm::mat4x4** modelView_out) final {
                *projection_out = projectionCurrent;
                *modelView_out = modelViewCurrent;
            }

            GlobalCache& GetGlobalCache() override { return globalCache; }
            size_t GetNumGlobalUniforms() override { return globalUniforms.size(); }
            const char* GetGlobalUniformNameByIndex(size_t index) override { return globalUniforms[index].name.c_str(); }
            const ShaderValueVariant& GetGlobalUniformValueByIndex(size_t index) override { return globalUniforms[index].value; }

            // RenderingKit::IVertexCacheOwner
            virtual void            OnVertexCacheFlush(IGLVertexCache* vc, size_t bytesUsed) override;

            // zfw::IResourceProvider
            virtual shared_ptr<IResource> CreateResource(IResourceManager* res, const std::type_index& resourceClass,
                    const char* normparams, int flags) override;
            virtual bool            DoParamsAlias(const std::type_index& resourceClass, const char* params1,
                    const char* params2) override { return false; }
            virtual const char*     TryGetResourceClassName(const std::type_index& resourceClass) override;

            // zfw::IResourceProvider2
            virtual IResource2* CreateResource(IResourceManager2* res, const TypeID& resourceClass,
                                               const char* recipe, int flags) override;

        private:
            CoordinateSystem coordSystem;

            // State (objects)
            Stack<IGLRenderBuffer*> renderBufferStack;
            unique_ptr<zfw::IResourceManager> sharedResourceManager;
            unique_ptr<zfw::IResourceManager2> sharedResourceManager2;

            struct VertexCache_t
            {
                unique_ptr<IGLVertexCache> cache;

                GLuint vao;
                GLVertexFormat* vertexFormat;
                IGLMaterial* material;
                MaterialSetupOptions options;
                RKPrimitiveType_t primitiveType;
                size_t numVertices;
            };

            // State (POD)
            Int2 windowSize, framebufferSize;
            Int2 viewportPos, viewportSize;

            glm::mat4x4* projectionCurrent, * modelViewCurrent;

            void p_SetRenderBuffer(IGLRenderBuffer* rb);

            std::string appName;
            zfw::ErrorBuffer_t* eb;
            RenderingKit* rk;

            unique_ptr<zfw::Timer> fpsTimer;
            int fpsNumFrames;

            // Private resources
            shared_ptr<ICamera> cam;       // used for shortcut methods (SetProjectionOrtho)
            VertexCache_t vertexCache;

            // Material Override
            IGLMaterial* materialOverride;

            //
            GlobalCache globalCache;
            std::vector<ShaderUniform> globalUniforms;
    };

    // ====================================================================== //
    //  class RenderingManager
    // ====================================================================== //

    unique_ptr<IRenderingManagerBackend> IRenderingManagerBackend::Create(zfw::ErrorBuffer_t* eb,
                                                                          RenderingKit* rk,
                                                                          CoordinateSystem coordSystem)
    {
        return make_unique<RenderingManager>(eb, rk, coordSystem);
    }

    RenderingManager::RenderingManager(ErrorBuffer_t* eb, RenderingKit* rk, CoordinateSystem coordSystem)
            : coordSystem(coordSystem)
    {
        this->appName = rk->GetEngine()->GetVarSystem()->GetVariableOrEmptyString("appName");
        this->eb = eb;
        this->rk = rk;

        fpsTimer.reset(rk->GetEngine()->CreateTimer());

        cam = p_CreateCamera(eb, rk, this, "RenderingManager/cam", coordSystem);

        vertexCache.vertexFormat = nullptr;
        vertexCache.material = nullptr;

        materialOverride = nullptr;
    }

    RenderingManager::~RenderingManager()
    {
        sharedResourceManager.reset();
        sharedResourceManager2.reset();
    }

    void RenderingManager::BeginFrame()
    {
        //if (profileFrame == frameNumber)
        //    profiler->ResetProfileRange();

        if (!fpsTimer->IsStarted())
        {
            fpsTimer->Start();
            fpsNumFrames = 0;
        }
        else
        {
            fpsNumFrames++;

            if (fpsTimer->GetMicros() >= 100000)
            {
                float frameTimeMs = fpsTimer->GetMicros() / 1000.0f / fpsNumFrames;
                float fps = 1000.0f / (frameTimeMs);

                char buffer[200];
                snprintf(buffer, sizeof(buffer), "%s / %.1f FPS / %.2f ms", appName.c_str(), glm::round(fps), frameTimeMs);

                rk->GetWindowManagerBackend()->SetWindowCaption(buffer);

                fpsTimer->Reset();
                fpsNumFrames = 0;
            }
        }

        GLStateTracker::ClearStats();
        SetViewportPosAndSize(Int2(), framebufferSize);
    }

    bool RenderingManager::CheckErrors(const char* caller)
    {
        GLenum err = glGetError();

        zombie_debug_assert(err == GL_NO_ERROR);

        if (err != GL_NO_ERROR)
        {
            rk->GetEngine()->Printf(kLogError, "OpenGL error %04Xh, detected in %s\n", err, caller);
            return false;
        }

        return true;
    }

    void RenderingManager::Clear()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void RenderingManager::Clear(Float4 color)
    {
        glClearColor(color.r, color.g, color.b, color.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void RenderingManager::ClearBuffers(bool color, bool depth, bool stencil)
    {
        glClear((color ? GL_COLOR_BUFFER_BIT : 0) | (depth ? GL_DEPTH_BUFFER_BIT : 0) | (stencil ? GL_STENCIL_BUFFER_BIT : 0));
    }

    void RenderingManager::ClearDepth()
    {
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    shared_ptr<IVertexFormat> RenderingManager::CompileVertexFormat(IShader* program, uint32_t vertexSize,
            const VertexAttrib_t* attributes, bool groupedByAttrib)
    {
        size_t numAttributes = 0;

        while (attributes[numAttributes].name) {
            numAttributes++;
        }

        VertexFormatInfo vf_in { vertexSize, attributes, numAttributes, nullptr };
        return std::make_shared<GLVertexFormat>(vf_in, globalCache);
    }

    shared_ptr<ICamera> RenderingManager::CreateCamera(const char* name)
    {
        return p_CreateCamera(eb, rk, this, name, coordSystem);
    }

    shared_ptr<IDeferredShadingManager> RenderingManager::CreateDeferredShadingManager()
    {
#ifndef RENDERING_KIT_USING_OPENGL_ES
        auto dsm = p_CreateGLDeferredShadingManager();
        
        if (!dsm->Init(eb, rk, this))
            return nullptr;

        return dsm;
#else
		return ErrorBuffer::SetError(eb, EX_INVALID_OPERATION, "desc", "Deferred shading is not supported in OpenGL ES mode.", nullptr),
				nullptr;
#endif
    }

    shared_ptr<IFontFace> RenderingManager::CreateFontFace(const char* name)
    {
        return p_CreateFontFace(eb, rk, this, name);
    }

	shared_ptr<IGeomBuffer> RenderingManager::CreateGeomBuffer(const char* name)
    {
        return p_CreateGeomBuffer(eb, rk, this, name);
    }

    shared_ptr<zfw::IGraphics> RenderingManager::CreateGraphicsFromTexture(shared_ptr<ITexture> texture)
    {
        const Float2 uv[] = { Float2(0.0f, 0.0f), Float2(1.0f, 1.0f) };

        auto g = p_CreateGLGraphics();
        
        if (!g->InitWithTexture(eb, rk, this, move(texture), uv))
            return nullptr;

        return g;
    }

    shared_ptr<zfw::IGraphics> RenderingManager::CreateGraphicsFromTexture2(shared_ptr<ITexture> texture, const Float2 uv[2])
    {
        auto g = p_CreateGLGraphics();
        
        if (!g->InitWithTexture(eb, rk, this, move(texture), uv))
            return nullptr;

        return g;
    }

    unique_ptr<IMaterial> RenderingManager::CreateMaterial(const char* name, IShader* program)
    {
        return p_CreateMaterialUniquePtr(eb, rk, this, name, static_cast<IGLShaderProgram*>(program));
    }

    shared_ptr<IMaterial> RenderingManager::CreateMaterial(const char* name, shared_ptr<IShader> program)
    {
        return p_CreateMaterial(eb, rk, this, name,
                std::static_pointer_cast<IGLShaderProgram>(program));
    }

    shared_ptr<IRenderBuffer> RenderingManager::CreateRenderBuffer(const char* name, Int2 size, int flags)
    {
        auto rb = p_CreateGLRenderBuffer();
        
        if (!rb->Init(eb, rk, this, name, size))
            return nullptr;

        if (flags & kRenderBufferColourTexture)
        {
            auto texture = p_CreateTexture(eb, rk, this, name);
            texture->SetContentsUndefined(size, kTextureNoMipmaps, kTextureRGBA8);
            rb->GLAddColourAttachment(move(texture));
        }

        if (flags & kRenderBufferDepthTexture)
        {
            auto texture = p_CreateTexture(eb, rk, this, name);
            texture->SetContentsUndefined(size, kTextureNoMipmaps, kTextureDepth);
            rb->GLSetDepthAttachment(move(texture));
        }

        return rb;
    }

    shared_ptr<IRenderBuffer> RenderingManager::CreateRenderBufferWithTexture(const char* name, shared_ptr<ITexture> texture, int flags)
    {
        auto rb = p_CreateGLRenderBuffer();
        
        if (!rb->Init(eb, rk, this, name, texture->GetSize()))
            return nullptr;

        rb->GLAddColourAttachment(std::static_pointer_cast<IGLTexture>(texture));

        return rb;
    }

    shared_ptr<IResource> RenderingManager::CreateResource(IResourceManager* res, const std::type_index& resourceClass, const char* normparams, int flags)
    {
        if (resourceClass == typeid(IFontFace))
        {
            String path;
            int size, flags;

            if (!gameui::UIThemerUtil::IsFontParams(normparams, path, size, flags))
                return ErrorBuffer::SetError2(eb, EX_INVALID_ARGUMENT, 0), nullptr;

            auto face = p_CreateFontFace(eb, rk, this, normparams);

            if (!face->Open(path, size, flags))
                return nullptr;

            return face;
        }
        else if (resourceClass == typeid(IGraphics))
        {
            auto g = p_CreateGLGraphics();

            if (!g->Init(eb, rk, this, res, normparams, flags))
                return nullptr;

            return g;
        }
        else if (resourceClass == typeid(IMaterial))
        {
			zombie_assert(false);
            return nullptr;
        }
        else if (resourceClass == typeid(IShader) || resourceClass == typeid(IGLShaderProgram))
        {
            String path;
            String outputNames;

            const char *key, *value;

            while (Params::Next(normparams, key, value))
            {
                if (strcmp(key, "path") == 0)
                    path = value;
                else if (strcmp(key, "outputNames") == 0)
                    outputNames = value;
            }

            // FIXME: Replace with a nice check
            ZFW_ASSERT(!path.isEmpty())

            auto program = p_CreateShaderProgram(eb, rk, this, path);

            if (!outputNames.isEmpty())
            {
                List<const char*> outputNames_;

                // FIXME: This will break one day
                char* search = const_cast<char*>(outputNames.c_str());

                while (true)
                {
                    char* space = strchr(search, ' ');

                    if (space != nullptr)
                    {
                        *space = 0;
                        outputNames_.add(search);
                        search = space + 1;
                    }
                    else
                    {
                        outputNames_.add(search);
                        break;
                    }
                }

                if (!program->GLCompile(path, outputNames_.c_array(), outputNames_.getLength()))
                    return nullptr;
            }
            else
            {
                if (!program->GLCompile(path, nullptr, 0))
                    return nullptr;
            }

            return program;
        }
        else if (resourceClass == typeid(ITexture))
        {
            String path;
            RKTextureWrap_t wrapx = kTextureWrapClamp, wrapy = kTextureWrapClamp;

            const char *key, *value;

            while (Params::Next(normparams, key, value))
            {
                if (strcmp(key, "path") == 0)
                    path = value;
                else if (strcmp(key, "wrapx") == 0)
                {
                    if (strcmp(value, "repeat") == 0)
                        wrapx = kTextureWrapRepeat;
                }
                else if (strcmp(key, "wrapy") == 0)
                {
                    if (strcmp(value, "repeat") == 0)
                        wrapy = kTextureWrapRepeat;
                }
            }

            // FIXME: Replace with a nice check
            ZFW_ASSERT(!path.isEmpty())

            Pixmap_t pm;

			if (!Pixmap::LoadFromFile(rk->GetEngine(), &pm, path))
                return nullptr;

            auto texture = p_CreateTexture(eb, rk, this, path);

            texture->SetWrapMode(0, wrapx);
            texture->SetWrapMode(1, wrapy);

            PixmapWrapper container(&pm, false);
            if (!texture->SetContentsFromPixmap(&container))
                return nullptr;

            return texture;
        }
        else
        {
            ZFW_ASSERT(resourceClass != resourceClass)
            return nullptr;
        }
    }

    IResource2* RenderingManager::CreateResource(IResourceManager2* res, const std::type_index& resourceClass, const char* recipe, int flags)
    {
        if (resourceClass == typeid(IGLMaterial) || resourceClass == typeid(IMaterial))
        {
            std::string shaderRecipe;
            std::vector<std::pair<std::string, std::string>> textures;

            const char *key, *value;

            while (Params::Next(recipe, key, value))
            {
                if (strcmp(key, "shader") == 0)
                    shaderRecipe = value;
                else if (strncmp(key, "texture:", 8) == 0)
                    textures.emplace_back(key + 8, value);
            }

            zombie_assert(!shaderRecipe.empty());

            // TODO: this should be done by GLMaterial::BindDependencies
            auto program = res->GetResource<IGLShaderProgram>(shaderRecipe.c_str(), flags);

            if (!program)
                return nullptr;

            auto material = p_CreateMaterialUniquePtr(eb, rk, this, recipe, program);

            // TODO: this should be done by GLMaterial::BindDependencies
            for (const auto& textureAssignment : textures)
            {
                auto texture = res->GetResource<IGLTexture>(textureAssignment.second.c_str(), flags);
                
                if (!texture)
                    return nullptr;

                material->SetTexture(textureAssignment.first.c_str(), texture);
            }

            return material.release();
        }
        else if (resourceClass == typeid(IModel))
        {
            std::string path;

            const char *key, *value;

            while (Params::Next(recipe, key, value))
            {
                if (strcmp(key, "path") == 0)
                    path = value;
            }

            zombie_assert(!path.empty());

            return IGLModel::Create(rk, path.c_str()).release();
        }
        else if (resourceClass == typeid(IShader) || resourceClass == typeid(IGLShaderProgram))
        {
            std::string path;
            std::string outputNames;

            const char *key, *value;

            while (Params::Next(recipe, key, value))
            {
                if (strcmp(key, "path") == 0)
                    path = value;
                else if (strcmp(key, "outputNames") == 0)
                    outputNames = value;
            }

            zombie_assert(!path.empty());

            auto program = p_CreateShaderProgramUniquePtr(eb, rk, this, path.c_str());

            if (!outputNames.empty())
            {
                std::vector<const char*> outputNames_;

                char* search = const_cast<char*>(outputNames.c_str());

                while (true)
                {
                    char* space = strchr(search, ' ');

                    if (space != nullptr)
                    {
                        *space = 0;
                        outputNames_.push_back(search);
                        search = space + 1;
                    }
                    else
                    {
                        outputNames_.push_back(search);
                        break;
                    }
                }

                program->SetOutputNames(&outputNames_[0], outputNames_.size());
            }

            return program.release();
        }
        else if (resourceClass == typeid(IGLTexture) || resourceClass == typeid(ITexture))
        {
            std::string path;
            RKTextureWrap_t wrapx = kTextureWrapClamp, wrapy = kTextureWrapClamp;

            const char *key, *value;

            while (Params::Next(recipe, key, value))
            {
                if (strcmp(key, "path") == 0)
                    path = value;
                else if (strcmp(key, "wrapx") == 0)
                {
                    if (strcmp(value, "repeat") == 0)
                        wrapx = kTextureWrapRepeat;
                }
                else if (strcmp(key, "wrapy") == 0)
                {
                    if (strcmp(value, "repeat") == 0)
                        wrapy = kTextureWrapRepeat;
                }
            }

            zombie_assert(!path.empty());

            auto texture = p_CreateTextureUniquePtr(eb, rk, this, path.c_str());

            texture->SetWrapMode(0, wrapx);
            texture->SetWrapMode(1, wrapy);

            return texture.release();
        }
        else if (resourceClass == typeid(IWorldGeometry))
        {
            std::string path, worldShaderRecipe;

            const char *key, *value;

            while (Params::Next(recipe, key, value))
            {
                if (strcmp(key, "path") == 0)
                    path = value;
                else if (strcmp(key, "worldShader") == 0)
                    worldShaderRecipe = value;
            }

            zombie_assert(!path.empty());
            zombie_assert(!worldShaderRecipe.empty());

            auto geom = p_CreateWorldGeometry(eb, rk, this, path.c_str(), worldShaderRecipe.c_str());

            return geom.release();
        }
        else
        {
            zombie_assert(resourceClass != resourceClass);
            return nullptr;
        }
    }

    /*ISceneGraph* RenderingManager::CreateSceneGraph(const char* name)
    {
        return p_CreateSceneGraph(eb, rk, this, name);
    }*/

    shared_ptr<ITexture> RenderingManager::CreateTexture(const char* name)
    {
        return p_CreateTexture(eb, rk, this, name);
    }

    shared_ptr<ITextureAtlas> RenderingManager::CreateTextureAtlas2D(const char* name, Int2 size)
    {
        auto texture = p_CreateTexture(eb, rk, this, sprintf_255("%s/texture", name));
        texture->SetContentsUndefined(size, kTextureNoMipmaps, kTextureRGBA8);

        auto pixelAtlas = new PixelAtlas(size, Int2(2, 2), Int2(0, 0), Int2(8, 8));

        auto atlas = p_CreateTextureAtlas(eb, rk, this, name);
        atlas->GLInitWith(move(texture), pixelAtlas);
        return atlas;
    }

    /*void RenderingManager::DrawFilledRectangle(const Float3& pos, const Float2& size, Byte4 colour)
    {
        ImmedVertex* vertices = static_cast<ImmedVertex*>(vertexCache->Alloc(&vertexCacheQuads, 4 * sizeof(ImmedVertex)));

        vertices[0].x = pos.x;
        vertices[0].y = pos.y;
        vertices[0].z = pos.z;
        vertices[0].u = 0.0f;
        vertices[0].v = 0.0f;
        memcpy(&vertices[0].rgba[0], &colour[0], 4);

        vertices[1].x = pos.x;
        vertices[1].y = pos.y + size.y;
        vertices[1].z = pos.z;
        vertices[1].u = 0.0f;
        vertices[1].v = 1.0f;
        memcpy(&vertices[1].rgba[0], &colour[0], 4);

        vertices[2].x = pos.x + size.x;
        vertices[2].y = pos.y + size.y;
        vertices[2].z = pos.z;
        vertices[2].u = 1.0f;
        vertices[2].v = 1.0f;
        memcpy(&vertices[2].rgba[0], &colour[0], 4);

        vertices[3].x = pos.x + size.x;
        vertices[3].y = pos.y;
        vertices[3].z = pos.z;
        vertices[3].u = 1.0f;
        vertices[3].v = 0.0f;
        memcpy(&vertices[3].rgba[0], &colour[0], 4);

        vertexCache->Flush();
    }

    void RenderingManager::DrawLines(IGeomChunk* gc)
    {
        vertexCache->Flush();

        p_DrawChunk(gc, currentMaterialRef, GL_LINES);
    }

    void RenderingManager::DrawQuads(IGeomChunk* gc)
    {
        vertexCache->Flush();

        p_DrawChunk(gc, currentMaterialRef, GL_QUADS);
    }

    void RenderingManager::DrawTriangles(IGeomChunk* gc)
    {
        vertexCache->Flush();

        p_DrawChunk(gc, currentMaterialRef, GL_TRIANGLES);
    }*/

    void RenderingManager::DrawPrimitives(IMaterial* material, RKPrimitiveType_t primitiveType, IGeomChunk* gc)
    {
        vertexCache.cache->Flush();

        MaterialSetupOptions options;
        options.type = MaterialSetupOptions::kNone;
        this->SetupMaterial(static_cast<IGLMaterial*>(material), options);

        p_DrawChunk(this, gc, primitiveTypeToGLMode[primitiveType]);
    }

    void RenderingManager::DrawPrimitivesTransformed(IMaterial* material, RKPrimitiveType_t primitiveType, IGeomChunk* gc,
                                                     const glm::mat4x4& transform)
    {
        vertexCache.cache->Flush();

        // This sure smells of a hack.
        // TODO: Document why we don't own modelViewCurrent in the first place
        auto prev = *modelViewCurrent;
        *modelViewCurrent = *modelViewCurrent * transform;
        this->DrawPrimitives(material, primitiveType, gc);
        *modelViewCurrent = prev;
    }

    void RenderingManager::EndFrame(int ticksElapsed)
    {
        vertexCache.cache->Flush();

        this->CheckErrors(li_functionName);

        /*if (profileFrame == frameNumber)
        {
            profiler->LeaveSection(prof_drawframe);
            profiler->EnterSection(prof_SwapBuffers);
        }*/

        // TODO: which would be the correct order for these?
        // (consider how delta calculation works)
        glFlush();

        /*uint8_t* cap_rgb;
        if (cap != nullptr && cap->OfferFrameBGRFlipped(0, payload->ticksElapsed, &cap_rgb) > 0)
        {
            glReadPixels(0, 0, gl.renderWindow.x, gl.renderWindow.y, GL_BGR, GL_UNSIGNED_BYTE, cap_rgb);
            cap->OnFrameReady(0);
        }*/

        rk->GetWindowManagerBackend()->Present();

        /*if (profileFrame == frameNumber)
        {
            profiler->LeaveSection(prof_SwapBuffers);
            profiler->PrintProfile();

            host->Printk(true, "Rendering Kit Profile: %u mat, %u tex, %u vbo, %u draw, %u sceneB",
                    gl.numMaterialSetups, gl.numTextureBinds, gl.numVboBinds, gl.numDrawCalls, gl.sceneBytesSent);
        }*/

        //frameNumber++;
    }

#if ZOMBIE_API_VERSION < 201701
    zfw::IResourceManager* RenderingManager::GetSharedResourceManager()
    {
        if (!sharedResourceManager)
        {
            sharedResourceManager.reset(rk->GetEngine()->CreateResourceManager("RenderingManager::sharedResourceManager"));
            this->RegisterResourceProviders(sharedResourceManager.get());
        }

        return sharedResourceManager.get();
    }
#endif

    void RenderingManager::GetViewportPosAndSize(Int2* viewportPos_out, Int2* viewportSize_out)
    {
        *viewportPos_out = viewportPos;
        *viewportSize_out = viewportSize;
    }

    // Why is it done this way?
    void RenderingManager::OnVertexCacheFlush(IGLVertexCache* vc, size_t bytesUsed)
    {
        this->CheckErrors(li_functionName);

        SetupMaterial(vertexCache.material, vertexCache.options);
        this->CheckErrors("RenderingManager::SetupMaterial");

        glBindVertexArray(vertexCache.vao);

        // TODO: this can be skipped if unchanged from last time
        GLStateTracker::BindArrayBuffer(vc->GetVBO());
        vertexCache.vertexFormat->Setup();
        this->CheckErrors("vertexFormat Setup");

        // Ignore bytesUsed, we're tracking this ourselves (don't have to ask VertexFormat this way)
        glDrawArrays(primitiveTypeToGLMode[vertexCache.primitiveType], 0, vertexCache.numVertices);
        this->CheckErrors("glDrawArrays");

        vertexCache.material = nullptr;
        vertexCache.vertexFormat = nullptr;
        vertexCache.numVertices = 0;
    }

    void RenderingManager::OnWindowResized(Int2 newSize)
    {
        windowSize = newSize;
        framebufferSize = windowSize;
    }

    void RenderingManager::p_SetRenderBuffer(IGLRenderBuffer* rb)
    {
        VertexCacheFlush();

        if (rb != nullptr)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, rb->GLGetFBO());
            framebufferSize = rb->GetViewportSize();
        }
        else
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            framebufferSize = windowSize;
        }

        this->CheckErrors(li_functionName);
        SetViewportPosAndSize(Int2(), framebufferSize);
    }

    void RenderingManager::PopRenderBuffer()
    {
        renderBufferStack.pop();

        auto rb = (!renderBufferStack.isEmpty()) ? renderBufferStack.top() : nullptr;
        p_SetRenderBuffer(rb);
    }

    void RenderingManager::PushRenderBuffer(IRenderBuffer* rb_in)
    {
        auto rb = static_cast<IGLRenderBuffer*>(rb_in);

        renderBufferStack.push(rb);
        p_SetRenderBuffer(rb);
    }

    bool RenderingManager::ReadPixel(Int2 posInFB, Byte4* colour_outOrNull, float* depth_outOrNull)
    {
        if (posInFB.x < 0 || posInFB.y < 0 || posInFB.x >= framebufferSize.x || posInFB.y >= framebufferSize.y)
            return false;

        if (depth_outOrNull != nullptr)
        {
            float depth = 1.0f;
            glReadPixels(posInFB.x, framebufferSize.y - posInFB.y - 1, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

            *depth_outOrNull = depth * 2.0f - 1.0f;
        }

        if (colour_outOrNull != nullptr)
        {
            uint8_t rgba[4] = {};
            glReadPixels(posInFB.x, framebufferSize.y - posInFB.y - 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &rgba);

            memcpy(colour_outOrNull, rgba, 4);
        }

        return true;
    }

#if ZOMBIE_API_VERSION < 201701
    void RenderingManager::RegisterResourceProviders(zfw::IResourceManager* res)
    {
        static const std::type_index resourceClasses[] = {
            typeid(IFontFace), typeid(IGraphics), typeid(IMaterial), typeid(IShader), typeid(ITexture),
            typeid(IGLShaderProgram)
        };

        res->RegisterResourceProvider(resourceClasses, li_lengthof(resourceClasses), this, 0);
    }
#endif

    void RenderingManager::RegisterResourceProviders(zfw::IResourceManager2* res)
    {
        static const std::type_index resourceClasses[] = {
            typeid(IMaterial), typeid(IShader), typeid(ITexture), typeid(IWorldGeometry),
            typeid(IGLMaterial),
            typeid(IModel),
            typeid(IGLShaderProgram), typeid(IGLTexture),
        };

        res->RegisterResourceProvider(resourceClasses, li_lengthof(resourceClasses), this);
    }

    intptr_t RenderingManager::RegisterShaderGlobal(const char* name)
    {
        for (size_t i = 0; i < globalUniforms.size(); i++) {
            if (globalUniforms[i].name == name) {
                return i;
            }
        }

        auto handle = globalUniforms.size();
        zombie_assert(handle < INT_MAX);

        globalUniforms.emplace_back();
        auto& uniform = globalUniforms.back();
        uniform.name = name;

        return handle;
    }

    void RenderingManager::SetCamera(ICamera* camera)
    {
        VertexCacheFlush();

        static_cast<IGLCamera*>(camera)->GLSetUpMatrices(viewportSize, projectionCurrent, modelViewCurrent);
    }

    void RenderingManager::SetClearColour(const Float4& colour)
    {
        glClearColor(colour.r, colour.g, colour.b, colour.a);
    }

    /*void RenderingManager::SetMaterial(IMaterial* materialRef)
    {
        vertexCache->Flush();

        zfw::Release(currentMaterialRef);
        currentMaterialRef = static_cast<IGLMaterial*>(materialRef);
    }*/

    void RenderingManager::SetMaterialOverride(IMaterial* materialOrNull)
    {
        this->materialOverride = static_cast<IGLMaterial*>(materialOrNull);
    }

    void RenderingManager::SetProjection(const glm::mat4x4& projection)
    {
        //glMatrixMode(GL_PROJECTION);
        //glLoadMatrixf(&projection[0][0]);

        // TODO: Not quite happy about this

        static glm::mat4x4 projection_;

        projection_ = projection;
        this->projectionCurrent = &projection_;
    }

    void RenderingManager::SetProjectionOrthoScreenSpace(float nearZ, float farZ)
    {
        cam->SetClippingDist(nearZ, farZ);
        cam->SetOrthoScreenSpace();
        SetCamera(cam.get());
    }

    void RenderingManager::SetRenderState(RKRenderStateEnum_t state, int value)
    {
        VertexCacheFlush();

        switch (state)
        {
            case RK_DEPTH_TEST: GLStateTracker::SetState(ST_GL_DEPTH_TEST, value != 0); break;
        }
    }

    void RenderingManager::SetShaderGlobal(intptr_t handle, Float3 value)
    {
        auto& uniform = globalUniforms[handle];
        uniform.value = value;

//        if (currentShaderProgram) {
//            p_UpdateGlobalUniformInProgram(currentShaderProgram, handle, uniform->value);
//        }
    }

    void RenderingManager::SetShaderGlobal(intptr_t handle, Float4 value)
    {
        auto& uniform = globalUniforms[handle];
        uniform.value = value;

//        if (currentShaderProgram) {
//            p_UpdateGlobalUniformInProgram(currentShaderProgram, handle, uniform->value);
//        }
    }

    void RenderingManager::SetViewportPosAndSize(Int2 viewportPos, Int2 viewportSize)
    {
        VertexCacheFlush();

        this->viewportPos = viewportPos;
        this->viewportSize = viewportSize;

        glViewport(viewportPos.x, framebufferSize.y - viewportPos.y - viewportSize.y, viewportSize.x, viewportSize.y);
    }

    bool RenderingManager::Startup()
    {
        rk->GetEngine()->Printf(kLogInfo, "Rendering Kit: %s | %s | %s", glGetString(GL_VERSION), glGetString(GL_RENDERER), glGetString(GL_VENDOR));

#ifdef RENDERING_KIT_USING_OPENGL_ES
		rk->GetEngine()->Printf(kLogInfo, "Rendering Kit: Using OpenGL ES-compatible subset");
#endif

        /*if (GLEW_ARB_debug_output)
        {
            glDebugMessageCallbackARB(glDebugProc, nullptr);
            glDebugMessageInsertARB(GL_DEBUG_SOURCE_APPLICATION_ARB, GL_DEBUG_TYPE_OTHER_ARB, 1, GL_DEBUG_SEVERITY_MEDIUM_ARB, 12, "Hello World!");
        }
        else if (GLEW_AMD_debug_output)
        {
            glDebugMessageCallbackAMD(glDebugProcAMD, nullptr);
            glDebugMessageEnableAMD(0, 0, 0, nullptr, GL_TRUE);

            glDebugMessageInsertAMD(GL_DEBUG_CATEGORY_APPLICATION_AMD, GL_DEBUG_SEVERITY_MEDIUM_AMD, 1, 12, "Hello World!");
        }*/

        windowSize = rk->GetWindowManagerBackend()->GetWindowSize();

        GLStateTracker::Init();
        CheckErrors(li_functionName);
        bool haveVAOs = (GLEW_ARB_vertex_array_object > 0);

        GLStateTracker::SetState(ST_GL_BLEND, true);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        CheckErrors("glBlendFunc");

        glEnable(GL_CULL_FACE);
        CheckErrors("glEnable(GL_CULL_FACE)");

        // Init private resources
        vertexCache.cache = unique_ptr<IGLVertexCache>(p_CreateVertexCache(eb, rk, this, 256 * 1024));
        glGenVertexArrays(1, &vertexCache.vao);
        vertexCache.numVertices = 0;
        //immedVertexFormat = CompileVertexFormat(nullptr, 24, immedVertexAttribs, false);

        framebufferSize = windowSize;
        SetViewportPosAndSize(Int2(), framebufferSize);
        CheckErrors(li_functionName);

        // Initialize shared ResourceManager2
        sharedResourceManager2.reset(rk->GetEngine()->CreateResourceManager2());
        sharedResourceManager2->SetTargetState(IResource2::REALIZED);
        this->RegisterResourceProviders(sharedResourceManager2.get());

        return true;
    }

    const char* RenderingManager::TryGetResourceClassName(const std::type_index& resourceClass)
    {
        if (resourceClass == typeid(::RenderingKit::IFontFace))
            return "RenderingKit::IFontFace";
        else if (resourceClass == typeid(::RenderingKit::IGraphics))
            return "RenderingKit::IGraphics";
        else if (resourceClass == typeid(::RenderingKit::IMaterial))
            return "RenderingKit::IMaterial";
        else if (resourceClass == typeid(::RenderingKit::IShader))
            return "RenderingKit::IShader";
        else if (resourceClass == typeid(::RenderingKit::ITexture))
            return "RenderingKit::ITexture";
        else
            return nullptr;
    }

    void RenderingManager::SetupMaterial(IGLMaterial* material, const MaterialSetupOptions& options)
    {
        if (materialOverride != nullptr)
            material = materialOverride;

        material->GLSetup(options, *projectionCurrent, *modelViewCurrent);
    }

    void* RenderingManager::VertexCacheAlloc(IVertexFormat* vertexFormat, IMaterial* material,
            RKPrimitiveType_t primitiveType, size_t numVertices)
    {
        if (vertexCache.vertexFormat != vertexFormat
                || vertexCache.material != material
                || vertexCache.options.type != MaterialSetupOptions::kNone
                || vertexCache.primitiveType != primitiveType)
        {
            vertexCache.cache->Flush();

            vertexCache.vertexFormat = static_cast<GLVertexFormat*>(vertexFormat);
            vertexCache.material = static_cast<IGLMaterial*>(material);
            vertexCache.options.type = MaterialSetupOptions::kNone;
            vertexCache.primitiveType = primitiveType;
        }

        void* p = vertexCache.cache->Alloc(this, numVertices * static_cast<GLVertexFormat*>(vertexFormat)->GetVertexSize());

        vertexCache.numVertices += numVertices;
        return p;
    }

    void* RenderingManager::VertexCacheAlloc(IVertexFormat* vertexFormat, IMaterial* material,
            const MaterialSetupOptions& options, RKPrimitiveType_t primitiveType, size_t numVertices)
    {
        if (vertexCache.vertexFormat != vertexFormat
                || vertexCache.material != material
                || !(vertexCache.options == options)
                || vertexCache.primitiveType != primitiveType)
        {
            vertexCache.cache->Flush();

            vertexCache.vertexFormat = static_cast<GLVertexFormat*>(vertexFormat);
            vertexCache.material = static_cast<IGLMaterial*>(material);
            vertexCache.options = options;
            vertexCache.primitiveType = primitiveType;
        }

        void* p = vertexCache.cache->Alloc(this, numVertices * static_cast<GLVertexFormat*>(vertexFormat)->GetVertexSize());

        vertexCache.numVertices += numVertices;
        return p;
    }

    void RenderingManager::VertexCacheFlush()
    {
        this->CheckErrors(li_functionName);

        // PROTIP: this will call our OnVertexCacheFlush
        vertexCache.cache->Flush();

        this->CheckErrors("vertexCache.cache->Flush()");
    }
}
