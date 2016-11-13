#pragma once

#include "render.hpp"

#include <ztype/ztype.hpp>

#define ZR_ENABLE_BATCH_STATS

namespace zr
{
    using namespace zfw;

    using zfw::render::IProgram;
    using zfw::render::ITexture;
    
    enum BatchFlags {
        BATCH_REMOTE = 1,
        BATCH_MMAPPED = 2,
        BATCH_THROW_ON_OVERFLOW = 4
    };

    enum RFlags {
        MODEL_DISABLE_MATERIALS = 1
    };

    enum VertexBuffer_Mesh_Flags_ {
        VBUF_REMOTE = 1,
        MESH_INDEXED = 2,       // numIndices being 0 doesn't necessarily imply this
    };
    
    /*enum VertexFlags {
        VTX_RGBA = 1, VTX_NXYZ = 2, VTX_UV = 4
    };*/

    enum DataFormat {
        FMT_NONE, FMT_INT16, FMT_UINT8, FMT_FLOAT32
    };

    enum PrimitiveMode {
        PM_LINE_LIST = 0,
        PM_TRIANGLE_LIST = 1
    };
    
    class MeshDataBlob;
    class VertexBuffer;

    struct FrameStats {
        size_t bytesStreamed;
        int batchesDrawn, polysDrawn;
        int textureSwitches, shaderSwitches;
    };

    struct RendererStats {
        int batchesLost;
    };

    struct VertexAttribFormat {
        uint16_t format, components;
        uint32_t offset;
    };

    struct VertexFormat {
        size_t vertexSize;
        struct VertexAttribFormat pos, normal, uv[1], colour;

        // any interface requiring this function to be set must explicitly declare so
        size_t (* AddVertexCb1)(volatile uint8_t *buffer, size_t count, CShort2* pos, CFloat2* uv, CByte4* colour);
    };

    // public classes
    
    class Renderer
    {
        public:
            static void Init();
            static void Exit();
            
            static void BeginFrame();
            static const FrameStats * EndFrame();
            
            static void BeginPicking();
            static uint32_t EndPicking(CInt2& samplePos);
            static uint32_t GetPickingId();
            static void SetPickingId(uint32_t id);

            //static void Flush();
    };

    class Font
    {
        protected:
            ITexture *tex;

            ztype::FaceMetrics metrics;

            size_t cp_first, count;
            List<ztype::CharEntry> entries;

            int shadow;

            void DrawAsciiString( const char* text, CShort2& pos_in, CByte4& colour_in );

            static Font* Open(ztype::FaceDesc *desc);
            static Font* OpenFont(const String& cache_filename);

        public:
            Font();
            ~Font();

            static Font* Open(const char* cache_filename);
            static Font* Open(const char* filename, int size, int range_min = 0x20, int range_max = 0x7F);

            int GetHeight() const { return metrics.ascent - metrics.descent; }

            void SetShadow(int amount) { shadow = amount; }

            void DrawAsciiString( const char* text, CInt2& pos_in, CByte4& colour, int flags );
            Int2 MeasureAsciiString( const char* text );
    };

    class Material : public ReferencedClass
    {
        protected:
            String name;
            Reference<ITexture> difftexture;

        private:
            Material();
            Material(const Material&);

        public:
            static Material* LoadFromDataBlock(InputStream* stream);

            void Bind();
            static void Unbind();

            li_ReferencedClass_override(Material)
    };

    class Mesh
    {
        public:
            /*struct Blueprint
            {
                size_t numVertices;
                float *coords, *normals, *uvs;
                uint16_t *colours;

                size_t numIndices;
                uint32_t *indices;
            };*/

        protected:
            Reference<Material> mat;

            int flags;
            GLenum primmode;
            VertexFormat format;
            size_t verticesFirst, indicesOffset;
            size_t numVertices, numIndices;

            Reference<VertexBuffer> vbuf;

            bool isAABBValid;
            Float3 aabb[2];

            Mesh();
            void RenderVertices();

            friend class Model;

        public:
            static Mesh* CreateFromMemory(int flags, PrimitiveMode mode, const VertexFormat *format, const void *vertices, size_t numVertices, const void *indices, size_t numIndices);
            static Mesh* CreateFromMemory(const media::DIMesh* blueprint);
            static Mesh* CreateFromMeshBlob(const MeshDataBlob* blob, size_t verticesOffset, size_t indicesOffset);

            //static Mesh* LoadFromDataBlock(InputStream* stream);
            static MeshDataBlob* PreloadBlobFromDataBlock(InputStream* stream);
        
            ~Mesh();

            void BindMaterial();
            void GetAABB(Float3 aabb[2]);
            void SetAABB(CFloat3 aabb[2]);
            void SetMaterial(Material* mat) { this->mat = mat; }

            void Render() { RenderVertices(); }
            void Render(const glm::mat4x4& transform);
    };
    
    class Model
    {
        protected:
            struct MeshCluster
            {
                VertexFormat vf;
                size_t first, count;

                size_t vertexOffset;
                size_t clusterVertexSize, clusterIndexSize;
            };

            List<Mesh*> meshes;

            Reference<VertexBuffer> sharedVbuf;
            List<MeshCluster> meshClusters;
        
            bool isAABBValid;
            Float3 aabb[2];

            Model();

            // used during initialization
            size_t BuildMeshClusters(MeshDataBlob** blobs, size_t count);

        public:
            static Model* CreateFromMeshes(Mesh** meshes, size_t count);
            static Model* CreateFromMeshBlobs(MeshDataBlob** blobs, size_t count);
            static Model* LoadFromFile(const char* fileName, bool required);
            ~Model();
        
            void GetBoundingBox(Float3& min, Float3& max);
            bool IsFinalized();
            //void Render();
            void Render(const glm::mat4x4& transform, int rflags);
    };
    
    class RenderBuffer
    {
        // FIXME: review whole class (also public)
        // TODO: depth support

        public:
            GLuint buffer, depth;
            Reference<render::GLTexture> tex;

            RenderBuffer(ITexture* tex);
            RenderBuffer(const RenderBuffer &);

        public:
            static RenderBuffer* Create(ITexture* tex);
            ~RenderBuffer();

            unsigned getHeight();
            unsigned getWidth();
            bool isSetUp();
    };

    class Batch
    {
        public:
            static const VertexFormat Format_XYs_UV_RGBAb;

            static void     Init();
            static void     Exit();

            static void     AcquireResources();
            static void     DropResources();

            //static void     BeginFrame();
            //static void     EndFrame();

            static void     Begin(size_t capacity, int mode = PM_TRIANGLE_LIST);
            //static void     BeginMapped(int vtxflags, size_t numVertices);
            //static void     BeginMapped(PrimitiveMode mode, const VertexFormat* format, size_t numVertices);
            static size_t   GetVertexSize(int vtxflags);
            //static void     QueryStats(BatchStats* stats);
            static void     SetTexture(GLuint texture);

            static void     Finish();
            static void     Flush();
            //void End();
    };

    class R
    {
        public:
            // Global settings
            static void Clear();
            static void SetClearColour(float r, float g, float b, float a);
            static void SetClearColour(CFloat4& colour) { SetClearColour(colour.r, colour.g, colour.b, colour.a); }

            // Render targets
            static void     PushRenderBuffer(RenderBuffer* renderBuffer);
            static void     PopRenderBuffer();
        
            static bool     ReadDepth(int x, int y, float& depth);

            // Projection/view
            static void     ResetView();
            static void     Set2DView();
            static void     SetCamera( const world::Camera& cam );
            static void     SetCamera( const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up );
            static void     SetPerspectiveProjection( float near, float far, float fov );

            static bool     WorldToViewport( const Float4& world, Int2& pos2d );

            // View clipping
            static void     PushClippingRect(int x, int y, int w, int h);
            static void     PopClippingRect();

            // Batching startup/exit
            //static void     Begin2D(size_t numVertices);
            //static void     BeginMapped2D(size_t numVertices);
            //static void     Flush2D();
        
            // Rendering setup
            static void     EnableDepthTest( bool enable );
            static void     SelectShadingProgram(IProgram* program);
            static void     SetBlendColour(Byte4 colour);
            static void     SetBlendColour(const Float4& colour);
            static void     SetTexture(ITexture* tex);
        
            // Primitives
            static void     DrawTexture(ITexture* tex, CShort2& pos, CByte4 colour = RGBA_WHITE);
            static void     DrawTexture(ITexture* tex, CShort2& pos, CFloat2& uva, CFloat2& uvb, CByte4 colour = RGBA_WHITE);

            static void     DrawBox(CFloat3& pos, CFloat3& size);
            static void     DrawBox(CFloat3& pos, CFloat3& size, CByte4 colour);
            static void     DrawBox(CFloat3& pos, CFloat3& size, CFloat4& colour);

            static void     DrawFilledRect(CShort2& a, CShort2& b, CByte4 colour);
            static void     DrawFilledRect(CShort2& a, CShort2& b, CFloat2& uva, CFloat2& uvb, CByte4 colour);
            static void     DrawFilledRect(CFloat3& a, CFloat3& b, CByte4 colour);

            static void     DrawFilledTri(CShort2& a, CShort2& b, CShort2& c, CByte4 colour);

            static void     DrawFilledQuad(CFloat2 pos[4], CFloat2 uv[4], CByte4 colour[4]);

            static void     DrawRectBillboard(CFloat3& pos, CFloat2& size);
            static void     DrawRectBillboard(CFloat3& pos, CFloat2& size, CFloat2& uva, CFloat2& uvb, CByte4 colour);

            static void     DrawRectRotated(CFloat3& pos, CFloat2& size, CFloat2& origin, CByte4 colour, float angle);
    };
    
    //// INTERNALS BEGIN HERE
#ifdef _DEBUG
#define zr_Dbgprintf(args)  printf args
#else
#define zr_Dbgprintf(args)
#endif

    struct BatchState
    {
        int flags;

        VertexFormat format;
        GLenum primitiveMode;

        GLuint vbo;
        size_t bufferCapacity, bufferUsed;
        void *buffer;
        size_t numVertices;
    };

    class MeshDataBlob
    {
        public:
            uint8_t* data;
            size_t vertexDataSize, indexDataSize;

            int flags;
            GLenum primmode;
            VertexFormat format;
            size_t numVertices, numIndices;

            uint32_t materialIndex;
            Float3 aabb[2];

            MeshDataBlob() { data = nullptr; }
            ~MeshDataBlob() { free(data); }
    };

    class VertexBuffer : public li::ReferencedClass
    {
        public:
            // Renderer internals access these directly
            GLuint vbo;
            size_t vboCapacity;
        
        protected:
            virtual ~VertexBuffer();

        public:
            VertexBuffer();
        
            void InitWithSize(int flags, size_t capacity);
            // no Exit() for a little reference-counted class

            // never call, will throw up
            void DropResources();

            // memory mapping
            volatile uint8_t * MapForWriting();
            void Unmap();

            //
            void UploadRange(size_t offset, size_t count, const void* data);

            li_ReferencedClass_override(VertexBuffer)
    };
    
    extern BatchState batch;
    extern FrameStats frameStats;
    extern RendererStats rendererStats;

    extern GLuint currentArrayBuffer, currentElementBuffer, currentTexture2D;
    extern render::GLProgram* boundProgram;
    extern const VertexFormat* boundFormat;

    extern Int4 currentViewport;

    extern uint32_t nextPickingId;
    
    extern const GLenum gl_formats[], gl_primmode[];
    
    void FlushBatch();
    void SetUpRendering(const VertexFormat& format);
    
    inline void SetArrayBuffer( GLuint vbo )
    {
        if (vbo != currentArrayBuffer )
        {
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            currentArrayBuffer = vbo;
        }
    }
    
    inline void SetElementBuffer( GLuint ibo )
    {
        if (ibo != currentElementBuffer )
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
            currentElementBuffer = ibo;
        }
    }

    inline void SetTexture2D( GLuint tex )
    {
        if (tex != currentTexture2D )
        {
            zr::FlushBatch();

            glBindTexture( GL_TEXTURE_2D, tex );
            currentTexture2D = tex;
        }
    }
}
