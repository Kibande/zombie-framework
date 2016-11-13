#pragma once

#include "../nbase.hpp"
#include "../n3d.hpp"

#include <framework/graphics.hpp>
#include <framework/messagequeue.hpp>
#include <framework/pixmap.hpp>
#include <framework/resourcemanager2.hpp>

#include <vector>

#include <3ds.h>
#include <gl.h>

namespace n3d
{
    using namespace li;

    class CTRTexture;
    class CTRVertexBuffer;

    struct UIVertex
    {
        float x, y, z;
        float u, v;
        int16_t n[4];
        uint8_t rgba[4];
    };

    class IVertexCacheOwner
    {
        public:
            virtual void OnVertexCacheFlush(CTRVertexBuffer* vb, size_t offset, size_t bytesUsed) = 0;
    };

    class CTRFont : public IFont, public IVertexCacheOwner
    {
        friend class IResource2;

        public:
            CTRFont(String&& path, unsigned int size, int textureFlags, int loadFlags);
            virtual ~CTRFont();

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
        
            virtual void OnVertexCacheFlush(CTRVertexBuffer* vb, size_t offset, size_t bytesUsed) final override;

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

            CTRTexture* texture;

            // preload
            Pixmap_t pm;
            unsigned int height, spacing, space_width;

            // preload/realize
            Array<Glyph> glyphs;
    };

    class CTRModel : public IModel
    {
        friend class CTRRenderer;

        List<Mesh*> meshes;
        
        public:
            virtual ~CTRModel();
        
            virtual void Draw() final override;
            virtual Mesh* GetMeshByIndex(unsigned int index) final override;
    };

    class CTRProjectionBuffer : public IProjectionBuffer
    {
        Projection proj;
        Float3 eye, center, up;
        float hfov, vfov;                   // persp
        float left, right, top, bottom;     // ortho
        float nearZ, farZ;

        glm::mat4 /*projection,*/ modelView;
        glm::mat4 modelViewTransp;

        void BuildModelView();

        public:
            CTRProjectionBuffer();
            virtual ~CTRProjectionBuffer() {}

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
                zombie_assert(false);
                //*output = (projection * modelView);
            }

            virtual void SetOrtho(float left, float right, float top, float bottom, float nearZ, float farZ) final override;
            virtual void SetOrthoScreenSpace(float nearZ, float farZ) final override;
            virtual void SetPerspective(float nearClip, float farClip) final override { proj = PROJ_PERSPECTIVE; this->nearZ = nearClip; this->farZ = farClip; }
            virtual void SetVFov(float vfov) final override;
            virtual void SetView(const Float3& eye, const Float3& center, const Float3& up) final override;

            void R_SetUpMatrices();
    };

    class CTRShaderProgram : public IShaderProgram
    {
        friend class IResource2;
        friend class CTRRenderer;

        public:
            CTRShaderProgram(String&& path);
            ~CTRShaderProgram();

            virtual int GetUniform(const char* name) final override;
            virtual void SetUniformInt(int id, int value);
            virtual void SetUniformVec3(int id, const Float3& value) final override;
            virtual void SetUniformVec4(int id, const Float4& value) final override;

            // IResource2*
            virtual void* Cast(const std::type_index& resourceClass) final override { return DefaultCast(this, resourceClass); }
            virtual State_t GetState() const final override { return state; }
            virtual bool StateTransitionTo(State_t targetState, IResourceManager2* resMgr) final override
            {
                return DefaultStateTransitionTo(this, targetState, resMgr);
            }

        protected:
            bool BindDependencies(IResourceManager2* resMgr) { return true; }
            bool Preload(IResourceManager2* resMgr);
            void Unload();
            bool Realize(IResourceManager2* resMgr);
            void Unrealize();

            State_t state;
            String path;

            // preloaded/realized
            void* shbin;
            size_t size;

            // realized
            GLprogramCTR shader;
    };

    class CTRTexture : public ITexture
    {
        friend class IResource2;

        public:
            CTRTexture(String&& path, int flags);

            //virtual const char* GetName() final override { return nullptr; }

            bool InitLevel(int level, const Pixmap_t& pm);
            //bool Init(Int2 size);
            bool Init(PixmapFormat_t format, Int2 size, const void* data);

            virtual Int2 GetSize() final override { return size; }

            void Bind();
            //void SetAlignment(int alignment) { this->alignment = alignment; }

            // IResource2*
            virtual void* Cast(const std::type_index& resourceClass) final override { return DefaultCast(this, resourceClass); }
            virtual State_t GetState() const final override { return state; }
            virtual bool StateTransitionTo(State_t targetState, IResourceManager2* resMgr) final override
            {
                return DefaultStateTransitionTo(this, targetState, resMgr);
            }

            bool BindDependencies(IResourceManager2* resMgr) { return true; }
            bool Preload(IResourceManager2* resMgr);
            void Unload();
            bool Realize(IResourceManager2* resMgr);
            void Unrealize();

        protected:
            State_t state;

            String path;
            int flags;

            GLtextureCTR tex;
            Int2 size;
            GLenum internalFormat;

            //float lodBias;

            bool PreInit();
    };

    class CTRVertexBuffer : public IVertexBuffer
    {
        friend class CTRRenderer;

        public:
            CTRVertexBuffer();
            virtual ~CTRVertexBuffer();

            virtual bool Alloc(size_t size) final override;

            virtual void* Map(bool read, bool write) final override;
            virtual void Unmap() final override;

            virtual bool Upload(const uint8_t* data, size_t length) final override;
            virtual bool UploadRange(size_t offset, size_t length, const uint8_t* data) final override;

        protected:
            GLbufferCTR vbo;
    };

    class CTRVertexCache : public CTRVertexBuffer
    {
        protected:
            IVertexCacheOwner* owner;
            size_t offset, bytesUsed;
            void* mapped;

        public:
            CTRVertexCache(size_t initSize);

            void* Alloc(IVertexCacheOwner* owner, size_t count);
            void Flush();
            void Reset();
    };

    class CTRRenderer : public IRenderer, public IResourceProvider2, public IVertexCacheOwner
    {
        public:
            CTRRenderer();
            virtual ~CTRRenderer() {}

            bool Init();
            void Shutdown();

            virtual void RegisterResourceProviders(IResourceManager2* res) final override;

            virtual IVertexFormat*  CompileVertexFormat(IShaderProgram* program, unsigned int vertexSize, const VertexAttrib* attributes) final override;
            virtual IFrameBuffer*   CreateFrameBuffer(Int2 size, bool withDepth) final override { return nullptr; }
            virtual IModel*         CreateModelFromMeshes(Mesh** meshes, size_t count) final override;
            virtual IVertexBuffer*  CreateVertexBuffer() final override { return new n3d::CTRVertexBuffer(); }

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
            //virtual void SetSMAA(bool active) final override { if (smaa != nullptr) smaa->SetActive(active); }
            virtual void SetTextureUnit(int unit, ITexture* tex) final override;
            virtual void SetVFov(float vfov) final override;
            virtual void SetView(const Float3& eye, const Float3& center, const Float3& up) final override;

            // zfw::IResourceProvider
            virtual IResource2*     CreateResource(IResourceManager2* res, const TypeID& resourceClass,
                    const char* recipe, int flags) final override;
            /*virtual bool            DoParamsAlias(const std::type_index& resourceClass,
                    const char* params1, const char* params2) final override { return false; }
            virtual const char*     TryGetResourceClassName(const std::type_index& resourceClass) final override;*/

            // n3d::IVertexCacheOwner
            virtual void OnVertexCacheFlush(CTRVertexBuffer* vb, size_t offset, size_t bytesUsed) final override;

            void BeginFrame();
            bool CaptureFrame(Pixmap_t* pm_out);
            void EndFrame(int ticksElapsed);
            IVertexFormat* GetFontVertexFormat();

            uint32_t renderingTime;

        private:
            CTRProjectionBuffer proj;

            uint32_t backgroundColor;
            Byte4 drawColor;

            std::vector<glm::mat4x4> matrixStack;

    };

    class CTRPlatform : public IPlatform
    {
        protected:
            unique_ptr<CTRRenderer> ctrr;

            MessageQueue* eventMsgQueue;

        public:
            CTRPlatform(ISystem* sys, MessageQueue* eventMsgQueue);
            virtual ~CTRPlatform() {}

            virtual void Init() final override;
            virtual void Shutdown() final override;

            IRenderer* InitRendering();

            MessageQueue* GetEventMessageQueue() { return eventMsgQueue; }
            void Swap();
    };
}
