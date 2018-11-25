#pragma once

/*
 *  Rendering Kit II
 *
 *  Part of Zombie Framework
 *  Copyright (c) 2013, 2014, 2016, 2018 Minexew Games
 */

#include <RenderingKit/RenderingKit.hpp>

#include <framework/errorbuffer.hpp>
#include <framework/graphics.hpp>

#include <littl/Base.hpp>
#include <littl/List.hpp>
#include <littl/Thread.hpp>

#pragma warning( push )
#pragma warning( disable : 4201 )
#include <glm/glm.hpp>
#pragma warning( pop )

#include <GL/glew.h>

// To prevent redefinition errors
#ifndef GL_SGIX_fragment_lighting
#define GL_SGIX_fragment_lighting
#endif

#include <SDL_opengl.h>

#include <stx/variant.hpp>

// FIXME: Implement OpenGL sanity checking

namespace RenderingKit
{
    using namespace li;

    using std::make_unique;
    using std::unique_ptr;

    struct Region_t;

    class GlobalCache;
    class GLVertexFormat;
    class IGLMaterial;
    class IGLTexture;
    class IRenderingManagerBackend;
    class PixelAtlas;
    class RenderingKit;

    enum State { ST_GL_BLEND, ST_GL_DEPTH_TEST, ST_MAX };
    enum { MAX_SHADER_OUTPUTS = 8 };
    enum { MAX_TEX = 8 };

    enum RKTextureFormat_t
    {
        kTextureRGBA8,
        kTextureRGBAFloat,
        kTextureDepth,
    };

    struct GLVertexAttrib_t
    {
        int location;
        uint32_t offset;
        int components;
        uint32_t sizeInBytes;
        GLenum type;
        int flags;
    };

    typedef stx::variant<float, Float3, Float4, glm::mat4x4> ShaderValueVariant;

    class GLStateTracker
    {
        public:
            static void Init();

            static void BindArrayBuffer(GLuint handle);
            static void BindElementArrayBuffer(GLuint handle);
            static void BindTexture(int unit, GLuint handle);
            static void ClearStats();
            static void InvalidateTexture(GLuint handle);
            static void SetActiveTexture(int unit);
            static void SetState(State state, bool enabled);
            static void UseProgram(GLuint handle);
    };

    class GLVertexFormat : public IVertexFormat
    {
        public:
            GLVertexFormat(const VertexFormatInfo& vf_in, GlobalCache& gc);

            uint32_t GetVertexSize() final { return vertexSize; }

            //virtual bool GetComponent(const char* componentName, Component<Float3>& component) override;
            //virtual bool GetComponent(const char* componentName, Component<Byte4>& component) override;
            //virtual bool GetComponent(const char* componentName, Component<Float2>& component) override;

            bool Equals(const VertexFormatInfo& vf_in);
            void Setup();

        private:
            uint32_t vertexSize;

            unsigned int numAttribs;
            GLVertexAttrib_t attribs[8];    // "temporary"
    };

    class IGLVertexCache;

    class IVertexCacheOwner
    {
        public:
            virtual void OnVertexCacheFlush(IGLVertexCache* vb, size_t bytesUsed) = 0;
    };

    class IGLVertexCache
    {
        public:
			virtual ~IGLVertexCache() {}

            virtual void* Alloc(IVertexCacheOwner* owner, size_t sizeInBytes) = 0;
            virtual void Flush() = 0;

            virtual GLuint GetVBO() = 0;
    };

    class IGLCamera : public ICamera
    {
        public:
            virtual void GLSetUpMatrices(const Int2& viewport, glm::mat4x4*& projection_out, glm::mat4x4*& modelView_out) = 0;
    };

#ifndef RENDERING_KIT_USING_OPENGL_ES
    class IGLDeferredShadingManager : public IDeferredShadingManager
    {
        public:
            virtual bool Init(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm) = 0;
    };
#endif

    class IGLFontFace : public IFontFace
    {
    };

    class IGLGeomBuffer : public IGeomBuffer
    {
        public:
            virtual void GLBindChunk(IGeomChunk* gc) = 0;
    };

    class IGLGraphics : public zfw::IGraphics
    {
        public:
            virtual bool Init(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm,
                    zfw::IResourceManager* res, const char* normparamSets, int flags) = 0;
            virtual bool InitWithTexture(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm,
                    shared_ptr<ITexture> texture, const Float2 uv[2]) = 0;
    };

    class IGLMaterial : public IMaterial
    {
        public:
            virtual void GLSetup(const MaterialSetupOptions& options, const glm::mat4x4& projection_out, const glm::mat4x4& modelView_out) = 0;
    };

    class IGLRenderBuffer : public IRenderBuffer
    {
        public:
            virtual bool Init(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm,
                    const char* name, Int2 viewportSize) = 0;

            virtual void GLAddColourAttachment(shared_ptr<IGLTexture> tex) = 0;
            virtual void GLCreateDepthBuffer(Int2 size) = 0;
            virtual GLuint GLGetFBO() = 0;
            //virtual IGLTexture* GLGetTexture() = 0;
            virtual void GLSetDepthAttachment(shared_ptr<IGLTexture> tex) = 0;
    };

    class IGLShaderProgram : public IShader
    {
        public:
            virtual bool GLCompile(const char* path, const char** outputNames, size_t numOutputNames) = 0;
            virtual int GLGetAttribLocation(const char* name) = 0;
            virtual void GLSetup() = 0;

            //virtual void SetGlobalUniformByIndex(int index, const ShaderValueVariant& value) = 0;
            virtual void SetOutputNames(const char** outputNames, size_t numOutputNames) = 0;
    };

    class IGLTexture : public ITexture
    {
        public:
            virtual void GLBind(int unit) = 0;
            virtual GLuint GLGetTex() = 0;
            virtual void SetContentsUndefined(Int2 size, int flags, RKTextureFormat_t fmt) = 0;
            virtual void TexSubImage(uint32_t x, uint32_t y, uint32_t w, uint32_t h, zfw::PixmapFormat_t pixmapFormat,
                    const uint8_t* data) = 0;
    };

    class IGLTextureAtlas : public ITextureAtlas
    {
        public:
            virtual bool GLInitWith(shared_ptr<IGLTexture> texture, PixelAtlas* pixelAtlas) = 0;
            shared_ptr<IGLTexture> GLGetTexture() { return std::static_pointer_cast<IGLTexture>(GetTexture()); }
    };

    class GlobalCache
    {
        public:
            int GetAttribLocationByName(const char* name);
            //void VisitAttribLocationMappings(const std::function<void(const char*, int)>& visitor);

            GLVertexFormat* ResolveVertexFormat(const VertexFormatInfo& vf_in);

        private:
    };

    class IRenderingManagerBackend : public IRenderingManager
    {
        public:
            virtual ~IRenderingManagerBackend() {}

            virtual bool Startup() = 0;
            virtual bool CheckErrors(const char* caller) = 0;

            virtual void OnWindowResized(Int2 newSize) = 0;
            virtual void SetupMaterial(IGLMaterial* material, const MaterialSetupOptions& options) = 0;

            // Uniform globals
            virtual GlobalCache& GetGlobalCache() = 0;
            virtual size_t GetNumGlobalUniforms() = 0;
            virtual const char* GetGlobalUniformNameByIndex(size_t index) = 0;
            virtual const ShaderValueVariant& GetGlobalUniformValueByIndex(size_t index) = 0;
    };

    class IWindowManagerBackend : public IWindowManager
    {
        public:
            virtual ~IWindowManagerBackend() = default;
            virtual void Release() = 0;

            virtual bool Init() = 0;

            virtual void Present() = 0;
            virtual void SetWindowCaption(const char* caption) = 0;
    };

    class RenderingKit : public IRenderingKit
    {
        zfw::IEngine* sys;
        zfw::ErrorBuffer_t* eb;

        unique_ptr<zfw::ShaderPreprocessor> shaderPreprocessor;

        //IOManager* iom;
        unique_ptr<IWindowManagerBackend> wm;
        unique_ptr<IRenderingManagerBackend> rm;

        public:
            RenderingKit();

            virtual bool Init(zfw::IEngine* sys, zfw::ErrorBuffer_t* eb, IRenderingKitHost* host) override;

#if ZOMBIE_API_VERSION >= 201901
            IRenderingManager*          StartupRendering() override;
#else
            IRenderingManager*          StartupRendering();
#endif

#if ZOMBIE_API_VERSION < 201901
            virtual IRenderingManager*  GetRenderingManager() override { return rm.get(); }
#endif
            virtual IWindowManager*     GetWindowManager() override { return wm.get(); }

            //zfw::Env* GetEnv() { return sys->GetEnv(); }
            zfw::IEngine* GetEngine() { return sys; }
            zfw::ShaderPreprocessor* GetShaderPreprocessor();

            IRenderingManagerBackend*   GetRenderingManagerBackend() { return rm.get(); }
            IWindowManagerBackend*      GetWindowManagerBackend() { return wm.get(); }
    };

    shared_ptr<ICamera>             p_CreateCamera(zfw::ErrorBuffer_t* eb, RenderingKit* rk,
            IRenderingManagerBackend* rm, const char* name);
    shared_ptr<IGLFontFace>         p_CreateFontFace(zfw::ErrorBuffer_t* eb, RenderingKit* rk,
            IRenderingManagerBackend* rm, const char* name);
    shared_ptr<IGLGeomBuffer>       p_CreateGeomBuffer(zfw::ErrorBuffer_t* eb, RenderingKit* rk,
            IRenderingManagerBackend* rm, const char* name);
    shared_ptr<IGLGraphics>         p_CreateGLGraphics();
    shared_ptr<IGLRenderBuffer>     p_CreateGLRenderBuffer();
    shared_ptr<IGLMaterial>         p_CreateMaterial(zfw::ErrorBuffer_t* eb, RenderingKit* rk,
            IRenderingManagerBackend* rm, const char* name, shared_ptr<IGLShaderProgram> program);
    //ISceneGraph*    p_CreateSceneGraph(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name);
    shared_ptr<IGLShaderProgram>    p_CreateShaderProgram(zfw::ErrorBuffer_t* eb, RenderingKit* rk,
            IRenderingManagerBackend* rm, const char* name);
    shared_ptr<IGLTexture>          p_CreateTexture(zfw::ErrorBuffer_t* eb, RenderingKit* rk,
            IRenderingManagerBackend* rm, const char* name);
    shared_ptr<IGLTextureAtlas>     p_CreateTextureAtlas(zfw::ErrorBuffer_t* eb, RenderingKit* rk,
            IRenderingManagerBackend* rm, const char* name);
    IGLVertexCache*                 p_CreateVertexCache(zfw::ErrorBuffer_t* eb, RenderingKit* rk,
            IRenderingManagerBackend* rm, size_t size);

    unique_ptr<IGLMaterial>         p_CreateMaterialUniquePtr(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name,
                                            IGLShaderProgram* program);
    unique_ptr<IGLTexture>          p_CreateTextureUniquePtr(zfw::ErrorBuffer_t* eb, RenderingKit* rk,
                                            IRenderingManagerBackend* rm, const char* name);
    unique_ptr<IGLShaderProgram>    p_CreateShaderProgramUniquePtr(zfw::ErrorBuffer_t* eb, RenderingKit* rk,
                                            IRenderingManagerBackend* rm, const char* name);

    unique_ptr<IWorldGeometry>      p_CreateWorldGeometry(zfw::ErrorBuffer_t* eb, RenderingKit* rk,
                                            IRenderingManagerBackend* rm, const char* path, const char* worldShaderRecipe);

#ifndef RENDERING_KIT_USING_OPENGL_ES
	shared_ptr<IGLDeferredShadingManager> p_CreateGLDeferredShadingManager();
#endif

    void p_DrawChunk(IRenderingManagerBackend* rm, IGeomChunk* gc_in, GLenum mode);

    IRenderingManagerBackend* CreateRenderingManager(zfw::ErrorBuffer_t* eb, RenderingKit* rk);
    IWindowManagerBackend* CreateSDLWindowManager(zfw::ErrorBuffer_t* eb, RenderingKit* rk);
}
