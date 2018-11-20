#pragma once

/*
 *  Rendering Kit II
 *
 *  Part of Zombie Framework
 *  Copyright (c) 2013, 2014, 2016 Minexew Games
 */

#include <framework/datamodel.hpp>
#include <framework/modulehandler.hpp>
#include <framework/resource.hpp>
#include <framework/utility/util.hpp>

#include <reflection/magic.hpp>

// >2013
// >windows still being windows
#undef DrawText
#undef LoadBitmap
#undef LoadImage

#pragma warning( push )
#pragma warning( disable : 4200 )

namespace RenderingKit
{
    using zfw::Byte2;
    using zfw::Byte3;
    using zfw::Byte4;
    using zfw::Int2;
    using zfw::Int3;
    using zfw::Int4;
    using zfw::Float2;
    using zfw::Float3;
    using zfw::Float4;

    using std::shared_ptr;
    using std::unique_ptr;

    class Paragraph;
    class IFontFace;
    class IFontQuadSink;
    class IGeomBuffer;
    class IGeomChunk;
    class IMaterial;
    class IRKUIThemer;
    class IShader;
    class ITexture;
    class ITextureAtlas;
    class IVertexFormat;
    class IWorldGeometry;

    enum RKAttribDataType_t
    {
        RK_ATTRIB_UBYTE,
        RK_ATTRIB_UBYTE_2,
        RK_ATTRIB_UBYTE_3,
        RK_ATTRIB_UBYTE_4,

        RK_ATTRIB_SHORT,
        RK_ATTRIB_SHORT_2,
        RK_ATTRIB_SHORT_3,
        RK_ATTRIB_SHORT_4,

        RK_ATTRIB_INT,
        RK_ATTRIB_INT_2,
        RK_ATTRIB_INT_3,
        RK_ATTRIB_INT_4,

        RK_ATTRIB_FLOAT,
        RK_ATTRIB_FLOAT_2,
        RK_ATTRIB_FLOAT_3,
        RK_ATTRIB_FLOAT_4,
    };

    enum
    {
        RK_ATTRIB_NOT_NORMALIZED = 1
    };

    enum RKPrimitiveType_t
    {
        RK_LINES,
        RK_TRIANGLES,
    };

    enum RKRenderStateEnum_t
    {
        RK_DEPTH_TEST
    };

    enum RKFPMaterialFlags_t
    {
        kFPMaterialIgnoreVertexColour = 1,
    };

    enum RKTextureFlags_t
    {
        kTextureNoMipmaps = 1,
    };

    enum RKTextureWrap_t
    {
        kTextureWrapClamp = 0,
        kTextureWrapRepeat,
    };

    enum RKRenderBufferFlags_t
    {
        kRenderBufferColourTexture = 1,
        kRenderBufferDepthTexture = 2,
    };

    enum RKBuiltinVertexAttrib_t
    {
        kBuiltinPosition,
        kBuiltinNormal,
        kBuiltinColor,
        kBuiltinUV,
    };

    struct VertexAttrib_t
    {
        const char* name;
        uint32_t offset;
        RKAttribDataType_t datatype;
        int flags;
    };

    struct ProjectionBuffer_t
    {
        glm::mat4x4 projection;
        glm::mat4x4 modelView;
    };

    struct MaterialSetupOptions
    {
        enum Type { kNone, kSingleTextureOverride };

        struct SingleTextureOverride
        {
            ITexture* tex;
        };
        
        Type type;

        union
        {
            SingleTextureOverride singleTex;
        }
        data;

        bool operator ==(const MaterialSetupOptions& other)
        {
            if (type != other.type)
                return false;

            switch (type)
            {
                case kSingleTextureOverride: return data.singleTex.tex == other.data.singleTex.tex;
                default: return true;
            }
        }
    };

    template <typename DataType> struct ComponentInst
    {
        DataType* data;
        size_t advance;

        void Advance()
        {
            data = reinterpret_cast<DataType*>(advance + reinterpret_cast<uint8_t*>(data));
        }

        void SetNext(const DataType& value)
        {
            *data = value;
            Advance();
        }
    };

    template <typename DataType> struct Component
    {
        size_t offset;
        size_t advance;

        Component<DataType>() : offset(0), advance(0) {}
        Component<DataType>(size_t offset, size_t advance) : offset(offset), advance(advance) {}

        ComponentInst<DataType> ForPtr(void* baseptr) const
        {
            ComponentInst<DataType> c = { reinterpret_cast<DataType*>(
                    reinterpret_cast<uint8_t*>(baseptr) + offset),
                    advance };

            return c;
        }

        void Invalidate() { advance = 0; }
        bool Valid() { return advance != 0; }
    };

    // Interfaces

    class IPixmap
    {
        public:
            virtual ~IPixmap() {}

            virtual const uint8_t* GetData() = 0;
            virtual zfw::PixmapFormat_t GetFormat() = 0;
            virtual Int2 GetSize() = 0;

            virtual uint8_t* GetDataWritable() = 0;
            virtual bool SetFormat(zfw::PixmapFormat_t fmt) = 0;
            virtual bool SetSize(Int2 size) = 0;
    };

	// Deprecated and unused, removed in API 2016.01
#if ZOMBIE_API_VERSION < 201601
    class IRenderingKitHost
    {
        public:
            virtual ~IRenderingKitHost() {}

            // TODO: WTF get rid of this
            virtual bool LoadBitmap( const char* path, zfw::Pixmap_t* pm ) = 0;
    };
#else
	class IRenderingKitHost;
#endif

    // Resources

    /**
     * TODO: No need to be this opaque. Camera should be entirely a client-side utility.
     */
    class ICamera
    {
        public:
            virtual ~ICamera() {}

            // Not thread-safe
            virtual const char* GetName() = 0;

            //virtual ProjectionBuffer* Clone() = 0;

            virtual float CameraGetDistance() = 0;
            virtual void CameraMove(const Float3& vec) = 0;
            virtual void CameraRotateXY(float alpha, bool absolute) = 0;
            virtual void CameraRotateZ(float alpha, bool absolute) = 0;
            virtual void CameraZoom(float amount, bool absolute) = 0;

            virtual Float3 GetCenter() = 0;
            virtual Float3 GetEye() = 0;
            virtual Float3 GetUp() = 0;

            virtual void GetModelViewMatrix(glm::mat4* output) = 0;
            //virtual void GetProjectionMatrix(glm::mat4* output) = 0;
            virtual void GetProjectionModelViewMatrix(glm::mat4* output) = 0;

            virtual void SetClippingDist(float nearClip, float farClip) = 0;
            virtual void SetOrtho(float left, float right, float top, float bottom) = 0;
            virtual void SetOrthoFakeFOV() = 0;
            virtual void SetOrthoScreenSpace() = 0;
            virtual void SetPerspective() = 0;
            virtual void SetVFov(float vfov_radians) = 0;
            virtual void SetView(const Float3& eye, const Float3& center, const Float3& up) = 0;
            virtual void SetView2(const Float3& center, const float eyeDistance, float yaw, float pitch) = 0;

            virtual void SetViewWithCenterDistanceYawPitch(const Float3& center, const float eyeDistance, float yaw, float pitch) {
                SetView2(center, eyeDistance, yaw, pitch);
            }

            Float3 GetDirection() { return glm::normalize(GetCenter() - GetEye()); }
    };

    class IFontFace: public zfw::IResource
    {
        public:
            enum { FLAG_SHADOW = 1 };
            enum { LAYOUT_FORCE_FIRST_LINE = zfw::ALIGN_MAX };

            // Thread-safe
            virtual Paragraph* CreateParagraph() = 0;

            // Not thread-safe
            virtual const char* GetName() = 0;

            virtual void SetTextureAtlas(shared_ptr<ITextureAtlas> atlas) = 0;

            virtual bool Open(const char* path, int size, int flags) = 0;

            virtual unsigned int GetLineHeight() = 0;
            virtual void SetQuadSink(IFontQuadSink* sink) = 0;

            //const char* styleParams
            virtual void LayoutParagraph(Paragraph* p, const char* textUtf8, Byte4 colour, int layoutFlags) = 0;
            virtual Int2 MeasureParagraph(Paragraph* p) = 0;
            virtual void ReleaseParagraph(Paragraph* p) = 0;

            virtual Int2 MeasureText(const char* textUtf8) = 0;

            virtual bool GetCharNear(Paragraph* p, const Float3& posInPS, ptrdiff_t* followingCharIndex_out, Float3* followingCharPosInPS_out) = 0;
            virtual bool GetCharPos(Paragraph* p, ptrdiff_t charIndex, Float3* charPosInPS_out) = 0;
            virtual Float3 ToParagraphSpace(Paragraph* p, const Float3& areaPos, const Float2& areaSize, const Float3& pos) = 0;
            virtual Float3 FromParagraphSpace(Paragraph* p, const Float3& areaPos, const Float2& areaSize, const Float3& posInPS) = 0;

            //virtual void RenderParagraph(Paragraph* p, const Float3& pos, IGeomChunk* gc, const char* styleParams) = 0;
            //virtual void DrawText(IGeomChunk* gc) = 0;

            virtual void DrawParagraph(Paragraph* p, const Float3& areaPos, const Float2& areaSize) = 0;
            virtual void DrawParagraph(Paragraph* p, const Float3& areaPos, const Float2& areaSize, Byte4 colour) = 0;
            virtual void DrawText(const char* textUtf8, Byte4 colour, int layoutFlags, const Float3& areaPos, const Float2& areaSize) = 0;

            /*virtual void SetShadowProperties(Int2 offset, Byte4 colour) = 0;
            virtual Material* GetMaterial() = 0;*/

            //virtual ITexture* HACK_GetTexture() = 0;
    };

    class IFontQuadSink
    {
        public:
            virtual ~IFontQuadSink() {}

            virtual int OnFontQuad(const Float3& pos, const Float2& size, const Float2 uv[2], Byte4 colour) = 0;
    };

    class IMaterial: public zfw::IResource, public zfw::IResource2
    {
        public:
            virtual IShader* GetShader() = 0;

#if ZOMBIE_API_VERSION < 201701
            virtual intptr_t SetTexture(const char* name, shared_ptr<ITexture>&& texture) = 0;
#endif

            virtual intptr_t SetTexture(const char* name, ITexture* texture) = 0;

            virtual void SetTextureByIndex(intptr_t index, ITexture* texture) = 0;
    };

    class IGeomChunk
    {
        public:
            virtual ~IGeomChunk() {}

            virtual bool AllocVertices(IVertexFormat* fmt, size_t count, int flags) = 0;
            virtual void UpdateVertices(size_t first, const void* buffer, size_t sizeInBytes) = 0;
    };

    class IGeomBuffer
    {
        protected:
            ~IGeomBuffer() {}

        public:
            enum AllocStrategy { ALLOC_LINEAR };
            enum AllocChunkFlags { ALLOCCHUNK_MAP = 1, ALLOCCHUNK_MIRRORED = 2 };

            virtual void AddRef() = 0;
            virtual const char* GetName() = 0;
            virtual void Release() = 0;

            virtual void SetVBMinSize(size_t size) = 0;
            virtual void SetVBMaxSize(size_t size) = 0;

            virtual IGeomChunk* CreateGeomChunk() = 0;
    };

    class IRenderBuffer: public zfw::IResource
    {
        public:
            virtual ~IRenderBuffer() {}

            virtual Int2 GetViewportSize() = 0;
            virtual shared_ptr<ITexture> GetDepthTexture() = 0;
            virtual shared_ptr<ITexture> GetTexture() = 0;
    };

    class IShader: public zfw::IResource, public zfw::IResource2 {
        public:
            virtual intptr_t GetUniformLocation(const char* name) = 0;
            virtual void SetUniformInt(intptr_t location, int value) = 0;
            virtual void SetUniformInt2(intptr_t location, Int2 value) = 0;
            virtual void SetUniformFloat(intptr_t location, float value) = 0;
            virtual void SetUniformFloat3(intptr_t location, const Float3& value) = 0;
            virtual void SetUniformFloat4(intptr_t location, const Float4& value) = 0;
            virtual void SetUniformMat4x4(intptr_t location, const glm::mat4x4& value) = 0;
    };

    class ITexture: public zfw::IResource, public zfw::IResource2
    {
        public:
            virtual void SetWrapMode(int axis, RKTextureWrap_t mode) = 0;

            virtual bool GetContentsIntoPixmap(IPixmap* pixmap) = 0;
            virtual Int2 GetSize() = 0;
            virtual bool SetContentsFromPixmap(IPixmap* pixmap) = 0;
    };

    class ITextureAtlas
    {
        public:
            virtual shared_ptr<ITexture> GetTexture() = 0;

            virtual bool AllocWithoutResizing(Int2 dimensions, Int2* pos_out) = 0;
    };

    class IVertexFormat
    {
        public:
            virtual ~IVertexFormat() {}

            virtual uint32_t GetVertexSize() = 0;
            virtual bool IsGroupedByAttrib() = 0;

            //virtual bool GetComponent(const char* componentName, Component<Float3>& component) = 0;
            //virtual bool GetComponent(const char* componentName, Component<Byte4>& component) = 0;
            //virtual bool GetComponent(const char* componentName, Component<Float2>& component) = 0;
    };

    // Managers

    class IDeferredShaderBinding
    {
        public:
            virtual ~IDeferredShaderBinding() {}
    };

    class IDeferredShadingManager
    {
        public:
            virtual ~IDeferredShadingManager() {}

            virtual void AddBufferByte4(Int2 size, const char* nameInShader) = 0;
            virtual void AddBufferFloat4(Int2 size, const char* nameInShader) = 0;

            virtual void BeginScene() = 0;
            virtual void EndScene() = 0;

            virtual void BeginDeferred() = 0;
            virtual void EndDeferred() = 0;

            virtual void InjectTexturesInto(IMaterial* material) = 0;

#if ZOMBIE_API_VERSION < 201701
            virtual unique_ptr<IDeferredShaderBinding> CreateShaderBinding(shared_ptr<IShader> shader) = 0;
            virtual void BeginShading(IDeferredShaderBinding* binding, const Float3& cameraPos) = 0;
            virtual void DrawPointLight(const Float3& pos, const Float3& ambient, const Float3& diffuse, float range,
                    ITexture* shadowMap, const glm::mat4x4& shadowMatrix) = 0;
            virtual void EndShading() = 0;
#endif
    };

    class IRenderingManager
    {
        protected:
            ~IRenderingManager() {}

        public:
#if ZOMBIE_API_VERSION < 201701
            virtual void RegisterResourceProviders(zfw::IResourceManager* res) = 0;
            virtual zfw::IResourceManager* GetSharedResourceManager() = 0;
#endif

            virtual void RegisterResourceProviders(zfw::IResourceManager2* res) = 0;
            virtual zfw::IResourceManager2* GetSharedResourceManager2() = 0;

            // Creators
            virtual shared_ptr<ICamera>        CreateCamera(const char* name) = 0;
            virtual shared_ptr<IDeferredShadingManager>    CreateDeferredShadingManager() = 0;
            virtual shared_ptr<IFontFace>      CreateFontFace(const char* name) = 0;

            virtual shared_ptr<IGeomBuffer>    CreateGeomBuffer(const char* name) = 0;
            virtual shared_ptr<zfw::IGraphics> CreateGraphicsFromTexture(shared_ptr<ITexture> texture) = 0;
            virtual shared_ptr<zfw::IGraphics> CreateGraphicsFromTexture2(shared_ptr<ITexture> texture, const Float2 uv[2]) = 0;
            virtual shared_ptr<IMaterial>      CreateMaterial(const char* name, shared_ptr<IShader> program) = 0;
            virtual shared_ptr<IRenderBuffer>  CreateRenderBuffer(const char* name, Int2 size, int flags) = 0;
            virtual shared_ptr<IRenderBuffer>  CreateRenderBufferWithTexture(const char* name, shared_ptr<ITexture> texture, int flags) = 0;
            virtual shared_ptr<ITexture>       CreateTexture(const char* name) = 0;
            virtual shared_ptr<ITextureAtlas>  CreateTextureAtlas2D(const char* name, Int2 size) = 0;

            /**
             *
             * @param program
             * @param vertexSize
             * @param attributes
             * @param groupedByAttrib must be false
             * @return
             */
            virtual shared_ptr<IVertexFormat>  CompileVertexFormat(IShader* program, uint32_t vertexSize,
                    const VertexAttrib_t* attributes, bool groupedByAttrib) = 0;

            // Frame
            virtual void BeginFrame() = 0;
            virtual void Clear() = 0;
            virtual void Clear(Float4 color) = 0;
            virtual void ClearBuffers(bool color, bool depth, bool stencil) = 0;
            [[deprecated]] virtual void ClearDepth() = 0;
            virtual void EndFrame(int ticksElapsed) = 0;
            virtual void SetClearColour(const Float4& colour) = 0;

            // Viewport
            virtual Int2 GetViewportSize() = 0;
            virtual void GetViewportPosAndSize(Int2* viewportPos_out, Int2* viewportSize_out) = 0;
            virtual void SetViewportPosAndSize(Int2 viewportPos, Int2 viewportSize) = 0;

            // Rendering Setup
            virtual void PopRenderBuffer() = 0;
            virtual void PushRenderBuffer(IRenderBuffer* rb) = 0;
            virtual void SetCamera(ICamera* camera) = 0;
            virtual void SetRenderState(RKRenderStateEnum_t state, int value) = 0;
            virtual void SetProjection(const glm::mat4x4& projection) = 0;
            virtual void SetProjectionOrthoScreenSpace(float nearZ, float farZ) = 0;

            // Material Override (FX, Picking)
            virtual void SetMaterialOverride(IMaterial* materialOrNull) = 0;

            // Rendering
            virtual void DrawPrimitives(IMaterial* material, RKPrimitiveType_t primitiveType, IGeomChunk* gc) = 0;

            // Vertex Cache
            virtual void FlushMaterial(IMaterial* material) = 0;
            virtual void FlushCachedMaterialOptions() = 0;
            virtual void* VertexCacheAlloc(IVertexFormat* vertexFormat, IMaterial* material,
                    RKPrimitiveType_t primitiveType, size_t numVertices) = 0;
            virtual void* VertexCacheAlloc(IVertexFormat* vertexFormat, IMaterial* material,
                    const MaterialSetupOptions& options, RKPrimitiveType_t primitiveType, size_t numVertices) = 0;
            virtual void VertexCacheFlush() = 0;

            template <typename VertexType>
            VertexType* VertexCacheAllocAs(IVertexFormat* vertexFormat, IMaterial* material,
                    RKPrimitiveType_t primitiveType, size_t numVertices)
            {
                return reinterpret_cast<VertexType*>(VertexCacheAlloc(vertexFormat, material, primitiveType,
                        numVertices));
            }

            template <typename VertexType>
            VertexType* VertexCacheAllocAs(IVertexFormat* vertexFormat, IMaterial* material,
                    const MaterialSetupOptions& options, RKPrimitiveType_t primitiveType, size_t numVertices)
            {
                return reinterpret_cast<VertexType*>(VertexCacheAlloc(vertexFormat, material, options,
                        primitiveType, numVertices));
            }

            // Successfully Stalling The Pipeline Since 2013
            // Returned depth is normalized to -1..1
            virtual bool ReadPixel(Int2 posInFB, Byte4* colour_outOrNull, float* depth_outOrNull) = 0;

            //
            //virtual void EnableZGrouping(bool enable) = 0;
            //virtual IVertexFormat* GetImmediateRenderingVertexFormatRef() = 0;
            //virtual void SetMaterial(IMaterial* materialRef) = 0;
            /*virtual void DrawFilledRectangle(const Float3& pos, const Float2& size, Byte4 colour) = 0;
            virtual void DrawLines(IGeomChunk* gc) = 0;
            virtual void DrawQuads(IGeomChunk* gc) = 0;
            virtual void DrawTriangles(IGeomChunk* gc) = 0;*/
    };

    class IWindowManager
    {
        protected:
            ~IWindowManager() {}

        public:
            virtual bool LoadDefaultSettings(zfw::IConfig* conf) = 0;
            virtual bool ResetVideoOutput() = 0;

            virtual Int2 GetWindowSize() = 0;
            virtual bool MoveWindow(Int2 vec) = 0;
            virtual void ReceiveEvents(zfw::MessageQueue* msgQueue) = 0;

            //virtual void SetVideoCapture(IVideoCapture* cap) = 0;
    };

    // Main Interface

    class IRenderingKit
    {
        public:
            virtual ~IRenderingKit() {}

			// `host` is deprecated and ignored
            virtual bool Init(zfw::ISystem* sys, zfw::ErrorBuffer_t* eb, IRenderingKitHost* host) = 0;

            virtual IRenderingManager*  GetRenderingManager() = 0;
            virtual IWindowManager*     GetWindowManager() = 0;

            REFL_CLASS_NAME("IRenderingKit", 1)
    };

#ifdef RENDERING_KIT_STATIC
    IRenderingKit* CreateRenderingKit();

    ZOMBIE_IFACTORYLOCAL(RenderingKit)
#else
    ZOMBIE_IFACTORY(RenderingKit, "RenderingKit")
#endif
}

#pragma warning( pop )
