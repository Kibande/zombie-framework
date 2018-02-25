#pragma once

#include "../nbase.hpp"
#include "../n3d.hpp"

#include <framework/graphics.hpp>
#include <framework/messagequeue.hpp>
#include <framework/pixmap.hpp>
#include <framework/resourcemanager2.hpp>
#include <framework/shader_preprocessor.hpp>

#include <GL/glew.h>

#include <SDL2/SDL.h>
#undef main

// To prevent redefinition errors
#define GL_SGIX_fragment_lighting
#include <SDL2/SDL_opengl.h>

#include <littl/Array.hpp>

#undef DrawText

namespace n3d
{
    using namespace li;

    class GLRenderer;
    class GLTexture;
    class GLVertexBuffer;

    enum
    {
        BUILTIN_POS = -2,
        BUILTIN_NORMAL = -3,
        BUILTIN_UV0 = -4,
        BUILTIN_COLOUR = -5
    };
    
    struct GLVertexAttrib
    {
        int location;
        unsigned int offset;
        int components;
        GLenum type;
    };
    
    class IVertexCacheOwner
    {
        public:
            virtual void OnVertexCacheFlush(GLVertexBuffer* vb, size_t bytesUsed) = 0;
    };
    
    class GLVertexFormat : public IVertexFormat
    {
        public:
            static const int MAX_ATTRIBS = 8;
        
            GLVertexAttrib pos, normal, uv0, colour;
        
            uint32_t vertexSize;
            //unsigned int numAttribs;
            //GLVertexAttrib attribs[MAX_ATTRIBS];

        public:
            virtual uint32_t GetVertexSize() final override { return vertexSize; }
    };

    class GLFont : public IFont, public IVertexCacheOwner
    {
        friend class IResource2;

        public:
            GLFont(String&& path, unsigned int size, int textureFlags, int loadFlags);
            virtual ~GLFont();

            bool BindDependencies(IResourceManager2* resMgr) { return true; }
            bool Preload(IResourceManager2* resMgr);
            void Unload();
            bool Realize(IResourceManager2* resMgr);
            void Unrealize();

            virtual void DrawText(const char* ascii, Int3 pos, Byte4 colour, int flags) final override
            {
                DrawText(pos.x, pos.y, pos.z, flags, ascii, 0, colour);
            }

            virtual void DrawText(const char* ascii, size_t length, Int3 pos, Byte4 colour, int flags) final override
            {
                DrawText(pos.x, pos.y, pos.z, flags, ascii, length, colour);
            }

            virtual Int2 MeasureText(const char* text, int flags) final override;
        
            virtual void OnVertexCacheFlush(GLVertexBuffer* vb, size_t bytesUsed) final override;

            void DrawText(int x1, int y1, int z1, int flags, const char* text, size_t length, Byte4 colour);

            // IResource2*
            virtual void* Cast(const TypeID& resourceClass) final override { return DefaultCast(this, resourceClass); }
            virtual State_t GetState() const final override { return state; }
            virtual bool StateTransitionTo(State_t targetState, IResourceManager2* resMgr) final override
            {
                return DefaultStateTransitionTo(this, targetState, resMgr);
            }

        protected:
            struct Glyph
            {
                Float2 uv[2];
                unsigned int width;

                Glyph()
                {
                    width = 0;
                }
            };

            State_t state;
            String path;
            unsigned int size;
            int textureFlags, loadFlags;
            float lodBias;

            GLTexture* texture;

            // preload
            Pixmap_t pm;
            unsigned int height, spacing, space_width;

            // preload/realize
            Array<Glyph> glyphs;
    };

    class GLModel : public IModel
    {
        friend class IResource2;
        friend class GLRenderer;

        public:
            GLModel(const GLModel&) = delete;
            GLModel(String&& path);
            virtual ~GLModel();

            static bool IsPathLoadable(const String& path);

            virtual void Draw() final override;
            virtual Mesh* GetMeshByIndex(unsigned int index) final override;

            bool BindDependencies(IResourceManager2* resMgr) { return true; }
            bool Preload(IResourceManager2* resMgr);
            void Unload();
            bool Realize(IResourceManager2* resMgr);
            void Unrealize();

            // IResource2*
            virtual void* Cast(const TypeID& resourceClass) final override { return DefaultCast(this, resourceClass); }
            virtual State_t GetState() const final override { return state; }
            virtual bool StateTransitionTo(State_t targetState, IResourceManager2* resMgr) final override
            {
                return DefaultStateTransitionTo(this, targetState, resMgr);
            }

        private:
            State_t state;

            String path;

            // preloaded
            std::vector<uint8_t> mdl1;

            // realized
            std::vector<Mesh> meshes;
    };
    
    class GLProjectionBuffer : public IProjectionBuffer
    {
        Projection proj;
        Float3 eye, center, up;
        float hfov, vfov;                   // persp
        float left, right, top, bottom;     // ortho
        float nearZ, farZ;

        glm::mat4 projection, modelView;

        void BuildModelView();

        public:
            GLProjectionBuffer();
            virtual ~GLProjectionBuffer() {}

            virtual float CameraGetDistance() final override;
            virtual void CameraMove(const Float3& vec) final override;
            virtual void CameraRotateXY(float alpha, bool absolute) final override;
            virtual void CameraRotateZ(float alpha, bool absolute) final override;
            virtual void CameraZoom(float amount, bool absolute) final override;

            virtual void GetModelViewMatrix(glm::mat4* output) final override
            {
                *output = modelView;
            }

            virtual void GetProjectionModelViewMatrix(glm::mat4* output) final override
            {
                *output = (projection * modelView);
            }

            virtual void SetOrtho(float left, float right, float top, float bottom, float nearZ, float farZ) final override;
            virtual void SetOrthoScreenSpace(float nearZ, float farZ) final override;
            virtual void SetPerspective(float nearClip, float farClip) final override { proj = PROJ_PERSPECTIVE; this->nearZ = nearClip; this->farZ = farClip; }
            virtual void SetVFov(float vfov) final override;
            virtual void SetView(const Float3& eye, const Float3& center, const Float3& up) final override;

            void R_SetUpMatrices();
    };

    class GLShaderProgram : public IShaderProgram
    {
        friend class IResource2;
        friend class GLRenderer;

        public:
            GLShaderProgram(GLRenderer* glr, String&& path);
            virtual ~GLShaderProgram() { Unrealize(); Unload(); }

            bool BindDependencies(IResourceManager2* resMgr) { return true; }
            bool Preload(IResourceManager2* resMgr) { return true; }
            void Unload() {}
            bool Realize(IResourceManager2* resMgr);
            void Unrealize();

            virtual int GetUniform(const char* name) final override;
            virtual void SetUniformInt(int id, int value);
            virtual void SetUniformVec3(int id, const Float3& value) final override;
            virtual void SetUniformVec4(int id, const Float4& value) final override;

            // IResource2*
            virtual void* Cast(const TypeID& resourceClass) final override { return DefaultCast(this, resourceClass); }
            virtual State_t GetState() const final override { return state; }
            virtual bool StateTransitionTo(State_t targetState, IResourceManager2* resMgr) final override
            {
                return DefaultStateTransitionTo(this, targetState, resMgr);
            }

        protected:
            GLuint p_CreateShader(const char* filename, const char* source, GLuint type);

            GLRenderer* glr;
            State_t state;
            String path;

            GLuint program;
    };

    class GLTexture : public ITexture
    {
        friend class IResource2;
        friend class GLFrameBuffer;
        friend class GLRenderer;

        public:
            GLTexture(String&& path, int flags, unsigned int numMipmaps = 1, float lodBias = 0.0f);
            virtual ~GLTexture() { Unrealize(); Unload(); }

            bool BindDependencies(IResourceManager2* resMgr) { return true; }
            bool Preload(IResourceManager2* resMgr);
            void Unload();
            bool Realize(IResourceManager2* resMgr);
            void Unrealize();

            bool InitLevel(int level, const Pixmap_t& pm);
            bool Init(Int2 size);
            //bool Init(PixmapFormat_t format, Int2 size, const void* data);

            virtual Int2 GetSize() final override { return size; }

            // IResource2*
            virtual void* Cast(const TypeID& resourceClass) final override { return DefaultCast(this, resourceClass); }
            virtual State_t GetState() const final override { return state; }
            virtual bool StateTransitionTo(State_t targetState, IResourceManager2* resMgr) final override
            {
                return DefaultStateTransitionTo(this, targetState, resMgr);
            }

            void Bind();
            //void SetAlignment(int alignment) { this->alignment = alignment; }

        protected:
            bool PreInit();

            State_t state;
            String path;
            int flags;
            unsigned int numMipmaps;
            float lodBias;

            // preloaded
            Pixmap_t pm;

            // realized
            GLuint tex;
            Int2 size;

            int alignment;
    };

    class GLFrameBuffer : public IFrameBuffer
    {
        friend class GLRenderer;

        protected:
            GLuint fbo, depth;
            shared_ptr<GLTexture> texture;

        public:
            GLFrameBuffer(Int2 size, bool withDepth);
            virtual ~GLFrameBuffer();

            bool GetSize(Int2& size);
            virtual ITexture* GetTexture() final override { return texture.get(); }
    };

    class GLVertexBuffer : public IVertexBuffer
    {
        friend class GLRenderer;

        protected:
            int32_t numRefs;
            GLuint vbo;
            size_t size;

        public:
            GLVertexBuffer();
            virtual ~GLVertexBuffer();

            virtual bool Alloc(size_t size) final override;

            virtual void* Map(bool read, bool write) final override;
            virtual void Unmap() final override;

            virtual bool Upload(const uint8_t* data, size_t length) final override;
            virtual bool UploadRange(size_t offset, size_t length, const uint8_t* data) final override;
    };

    class GLVertexCache : public GLVertexBuffer
    {
        protected:
            IVertexCacheOwner* owner;
            size_t bytesUsed;
            void* mapped;

        public:
            GLVertexCache(size_t initSize);
        
            void* Alloc(IVertexCacheOwner* owner, size_t count);
            void Flush();
    };
    
    class SMAA
    {
        protected:
            bool active;

            shared_ptr<GLTexture> areaTex, searchTex;
            shared_ptr<GLFrameBuffer> edgesTex, blendTex;
            shared_ptr<GLShaderProgram> smaa_edge, smaa_weight, smaa_blending;
            int smaa_edge_colorTex, smaa_weight_edgesTex, smaa_weight_areaTex, smaa_weight_searchTex, smaa_blending_colorTex, smaa_blending_blendTex;

        public:
            SMAA();

            bool Init();
            void Process(GLFrameBuffer* image);
            void SetActive(bool active) { this->active = active; }
    };

    class GLRenderer : public IRenderer, public IResourceProvider2
    {
        friend class SMAA;

        protected:
            GLProjectionBuffer proj;
            bool picking;
            
            // FrameBuffer
            GLFrameBuffer* currentFB;
            unique_ptr<GLFrameBuffer> renderBuffer;

            // Shaders
            unique_ptr<ShaderPreprocessor> shaderPreprocessor;

            // Global resources
            unique_ptr<IVertexFormat> fontVertexFormat;
            unique_ptr<IVertexFormat> modelVertexFormat;

            // SMAA
            unique_ptr<SMAA> smaa;

        public:
            GLRenderer();
            virtual ~GLRenderer() {}

            bool Init();
            void Shutdown();

            virtual void RegisterResourceProviders(IResourceManager2* resMgr) final override;

            virtual IVertexFormat*  CompileVertexFormat(IShaderProgram* program, unsigned int vertexSize, const VertexAttrib* attributes) final override;
            virtual IFrameBuffer*   CreateFrameBuffer(Int2 size, bool withDepth) final override { return new GLFrameBuffer(size, withDepth); }
            virtual IModel*         CreateModelFromMeshes(Mesh** meshes, size_t count) final override;
            virtual IVertexBuffer*  CreateVertexBuffer() final override { return new GLVertexBuffer(); }
            //IFont*          LoadFont(const char* path, int scale, int textureFlags, int loadFlags);
            //GLShaderProgram*        LoadShaderProgram(const char* path, const char* vertexShaderPrepend, const char* pixelShaderPrepend, int loadFlags);
            //IShaderProgram* LoadShaderProgram(const char* path, int loadFlags) final override { return LoadShaderProgram(path, nullptr, nullptr, loadFlags); }
            //ITexture*       LoadTexture(const char* path, int textureFlags, int loadFlags);
            //ITexture*       LoadTextureMipmaps(const char* path, unsigned count, float lodBias, int loadFlags);

            virtual void Begin3DScene() final override;
            virtual void End3DScene() final override;

            virtual void BeginPicking() final override;
            virtual uint32_t EndPicking(Int2 samplePos) final override;

            virtual float SampleDepth(Int2 samplePos) final override;

            virtual void Clear(Byte4 colour) final override;
            virtual void DrawPrimitives(IVertexBuffer* vb, PrimitiveType type, IVertexFormat* format, uint32_t offset, uint32_t count) final override;
            virtual void DrawQuad(const Int3 abcd[4]) final override;
            virtual void DrawRect(Int3 pos, Int2 size) final override;
            virtual void DrawTexture(ITexture* tex, Int2 pos) final override;
            virtual void DrawTexture(ITexture* tex, Int2 pos, Int2 size, Int2 areaPos, Int2 areaSize) final override;
            virtual void DrawTextureBillboard(ITexture* tex, Float3 pos, Float2 size) final override;
            virtual void GetModelView(glm::mat4x4& modelView) final override;
            virtual void GetProjectionModelView(glm::mat4x4& output) final override;
            virtual void PopTransform() final override;
            virtual void PushTransform(const glm::mat4x4& transform) final override;
            //virtual void ReleaseVertexFormat(IVertexFormat*& format) final override { zfw::Release(format); }
            virtual void SetColour(const Byte4 colour) final override;
            virtual void SetColourv(const uint8_t* rgba) final override;
            virtual void SetFrameBuffer(IFrameBuffer* fb) final override;
            virtual void SetOrthoScreenSpace(float nearZ, float farZ) final override;
            virtual void SetPerspective(float nearClip, float farClip) final override;
            virtual void SetShaderProgram(IShaderProgram* program) final override;
            virtual void SetTextureUnit(int unit, ITexture* tex) final override;
            virtual void SetVFov(float vfov) final override;
            virtual void SetView(const Float3& eye, const Float3& center, const Float3& up) final override;

            // zfw::IResourceProvider2
            virtual IResource2* CreateResource(IResourceManager2* res, const TypeID& resourceClass,
                    const char* recipe, int flags) final override;

            void SetSMAA(bool active) { if (smaa != nullptr) smaa->SetActive(active); }

            IVertexFormat* GetFontVertexFormat();
            IVertexFormat* GetModelVertexFormat();
            bool LoadShader(const char* path, char** source_out);
            void InitSMAA();
    };

    class SDLPlatform : public IPlatform
    {
        protected:
            SDL_Window* displayWindow;
            unique_ptr<GLRenderer> glr;

            MessageQueue* eventMsgQueue;
        
            int multisampleLevel, swapControl;

        public:
            SDLPlatform(ISystem* sys, MessageQueue* eventMsgQueue);
            virtual ~SDLPlatform() {}

            virtual void Init() final override;
            virtual void Shutdown() final override;

            IRenderer* SetGLVideoMode(int w, int h, int flags);
            void SetMultisampleLevel(int level) { multisampleLevel = level; }
            void SetSwapControl(int swapControl) { this->swapControl = swapControl; }

            MessageQueue* GetEventMessageQueue() { return eventMsgQueue; }
            SDL_Window* GetSdlWindow() { return displayWindow; }
            void Swap();
    };

    IGraphics2* p_CreateGLGraphics2(String&& recipe);
}
