
#include "RenderingKitImpl.hpp"

// Uncomment to enable debug messages in this unit
#define DEBUG_GEOMBUFFER

#ifdef DEBUG_GEOMBUFFER
#define Debug_Printk(text) printf text
#else
#define Debug_Printk(text)
#endif

namespace RenderingKit
{
    using namespace zfw;

    class GLGeomChunk;

    struct Region_t
    {
        uint32_t vboIndex, offset, length;
        enum { REGION_VERTEX, REGION_INDEX } type;
    };

    struct VertexRegion_t : public Region_t
    {
        GLVertexFormat* fmt;
        uint32_t vertexSize;
        uint32_t capacityInVertices;

        uint32_t linearAllocIndex;
    };

    struct IndexRegion_t : public Region_t
    {
        GLenum datatype;
        uint32_t capacityInIndices;

        uint32_t linearAllocIndex;
    };

    class GLGeomBuffer : public IGLGeomBuffer
    {
        struct BufferObject_t
        {
            enum { MAPPED_WHOLE };

            GLenum type;
            GLuint obj;
            uint32_t size;

            uint8_t* mapped;        // pointer from glMapBuffer
            int mapRefs;            // reference count (MapChunk++ / UnMapChunk--)
            int mapTimeout;

            // strategy = LINEAR
            uint32_t allocIndex;
        };

        zfw::ErrorBuffer_t* eb;
        RenderingKit* rk;
        IRenderingManagerBackend* rm;
        String name;
        int32_t numRefs;

        AllocStrategy strategy;

        size_t vbMinSize, vbMaxSize;
        List<BufferObject_t, size_t, Allocator<BufferObject_t>,
                ArrayOptions::noBoundsChecking> vbufs;

        List<Region_t*, size_t, Allocator<Region_t*>,
                ArrayOptions::noBoundsChecking> regions;

        size_t numLateMappedVBs;

        void p_DoUnmapVB(size_t i);
        VertexRegion_t* p_GetVertexRegion(GLVertexFormat* fmt, size_t numVerticesRequired);
        bool p_ProvideVertexBufferObject(size_t spaceNeeded, size_t& vbi);

        public:
            GLGeomBuffer(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name);
            ~GLGeomBuffer();

            virtual void AddRef() override { ++numRefs; }
            virtual const char* GetName() override { return name.c_str(); }
            virtual void Release() override { if (--numRefs == 0) delete this; }

            virtual void SetVBMinSize(size_t size) override { vbMinSize = size; }
            virtual void SetVBMaxSize(size_t size) override { vbMaxSize = size; }
            
            virtual IGeomChunk* CreateGeomChunk() override;

            virtual void GLBindChunk(IGeomChunk* gc) override;
            bool AllocVertices(GLGeomChunk* gc, IVertexFormat* fmt, size_t count, int flags);
            GLuint GetChunkVbo(GLGeomChunk* gc);
            void ReleaseChunk(GLGeomChunk* gc);
            void UpdateVertices(GLGeomChunk* gc, size_t first, const uint8_t* buffer, size_t sizeInBytes);
    };

    class GLGeomChunk : public IGeomChunk
    {
        public:
            GLGeomBuffer* owner;
            Region_t* region;
            size_t index, count;
            int32_t numRefs;
            GLuint vbo;         // for faster access

        public:
            GLGeomChunk(GLGeomBuffer* owner);
            ~GLGeomChunk() { owner->ReleaseChunk(this); }

            virtual bool AllocVertices(IVertexFormat* fmt, size_t count, int flags) override
            {
                return owner->AllocVertices(this, fmt, count, flags);
            }

            virtual void UpdateVertices(size_t first, const uint8_t* buffer, size_t sizeInBytes) override
            {
                owner->UpdateVertices(this, first, buffer, sizeInBytes);
            }
    };

    void p_DrawChunk(IRenderingManagerBackend* rm, IGeomChunk* gc_in, IGLMaterial* material, GLenum mode)
    {
        auto gc = static_cast<GLGeomChunk*>(gc_in);

        ZFW_ASSERT(gc->region->type == Region_t::REGION_VERTEX)

        gc->owner->GLBindChunk(gc_in);

        MaterialSetupOptions options;
        options.type = MaterialSetupOptions::kNone;

        rm->SetupMaterialAndVertexFormat(material, options, static_cast<VertexRegion_t*>(gc->region)->fmt, gc->vbo);

        glDrawArrays(mode, gc->index, gc->count);

        rm->CleanupMaterialAndVertexFormat();
    }

    shared_ptr<IGLGeomBuffer> p_CreateGeomBuffer(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name)
    {
        return std::make_shared<GLGeomBuffer>(eb, rk, rm, name);
    }

    GLGeomChunk::GLGeomChunk(GLGeomBuffer* owner)
    {
        this->owner = owner;
        region = nullptr;
        numRefs = 1;
    }

    GLGeomBuffer::GLGeomBuffer(zfw::ErrorBuffer_t* eb, RenderingKit* rk, IRenderingManagerBackend* rm, const char* name)
            : name(name)
    {
        this->eb = eb;
        this->rk = rk;
        this->rm = rm;
        numRefs = 1;

        strategy = ALLOC_LINEAR;

        vbMinSize = 64 * 1024;          // 64 KiB
        vbMaxSize = 64 * 1024 * 1024;   // 64 MiB

        numLateMappedVBs = 0;
    }

    GLGeomBuffer::~GLGeomBuffer()
    {
        iterate2 (i, regions)
            delete i;

        iterate2 (i, vbufs)
           glDeleteBuffers(1, &(*i).obj);
    }
    
    bool GLGeomBuffer::AllocVertices(GLGeomChunk* gc, IVertexFormat* fmt, size_t count, int flags)
    {
        if (gc->region != nullptr)
        {
            ZFW_ASSERT(count <= gc->count)
            return true;
        }
        
        ZFW_ASSERT(strategy == ALLOC_LINEAR)
        //DBG_GLSanityCheck(true, nullptr);

        VertexRegion_t* region = p_GetVertexRegion(static_cast<GLVertexFormat*>(fmt), count);

        ZFW_ASSERT(region != nullptr)
        ZFW_ASSERT(region->linearAllocIndex + count < region->capacityInVertices)

        gc->region = region;
        gc->index = region->linearAllocIndex;
        gc->count = count;
        gc->vbo = vbufs[region->vboIndex].obj;

        /*if ((flags & ALLOCCHUNK_MIRRORED) && !material->IsGroupedByComponent())
            gc->mirrors = AllocMirror(numVertices * region->vertexSize);
        else
            gc->mirrors = nullptr;*/

        region->linearAllocIndex += count;

        /*if (flags & ALLOCCHUNK_MAP)
        {
            ZFW_ASSERT(R_MapRegion(gc->region, ptr) == 0)
            *(uint8_t **) ptr += region->material->GetMappingOffset(gc->index);
        }*/

        return true;
    }

    IGeomChunk* GLGeomBuffer::CreateGeomChunk()
    {
        return new GLGeomChunk(this);
    }

    void GLGeomBuffer::GLBindChunk(IGeomChunk* gc_in)
    {
        auto gc = static_cast<GLGeomChunk*>(gc_in);

        const size_t i = gc->region->vboIndex;

        ZFW_ASSERT(vbufs[i].mapRefs == 0)

        if (vbufs[i].mapped != nullptr)
        {
            //if (logVBMapCalls) { host->Printk(false, "Rendering Kit: actual glUnmapBuffer() for rendering"); }
            p_DoUnmapVB(i);
            numLateMappedVBs--;
        }

        if (gc->region->type == Region_t::REGION_VERTEX)
            GLStateTracker::BindArrayBuffer(gc->vbo);
        else
            GLStateTracker::BindElementArrayBuffer(gc->vbo);
    }

    void GLGeomBuffer::p_DoUnmapVB(size_t i)
    {
        // FIXME: Might actually be an index buffer?
        GLStateTracker::BindArrayBuffer(vbufs[i].obj);
        glUnmapBuffer(GL_ARRAY_BUFFER);

        vbufs[i].mapped = nullptr;
    }

    bool GLGeomBuffer::p_ProvideVertexBufferObject(size_t spaceNeeded, size_t& vbi)
    {
        for (vbi = 0; vbi < vbufs.getLength(); vbi++)
            if (vbufs[vbi].allocIndex + spaceNeeded <= vbufs[vbi].size)
                return true;

        ZFW_ASSERT(spaceNeeded < vbMaxSize)
        ZFW_ASSERT(vbMinSize <= vbMaxSize)
        ZFW_ASSERT(vbMaxSize <= UINT32_MAX)
        
        auto& vbuf = vbufs.addEmpty();
            
        // Calculate VB size
        // First try to fit the allocated chunk
        vbuf.size = (uint32_t) vbMinSize;
            
        while (spaceNeeded > vbuf.size)
            vbuf.size <<= 1;
            
        if (vbuf.size > vbMaxSize)
        {
            vbufs.removeFromEnd(0);
            return false;
        }
            
        // Try to go as high as (lastVB.size * 2)
        if (!vbufs.isEmpty() && vbuf.size < vbufs.getFromEnd().size * 2)
            vbuf.size = (uint32_t) std::min<size_t>(vbufs.getFromEnd().size * 2, vbMaxSize);

        glGenBuffers(1, &vbuf.obj);
        //DBG_GLSanityCheck(false, "vertex buffer creation");
            
        GLStateTracker::BindArrayBuffer(vbuf.obj);
            
        // TODO: more options for `usage`
        glBufferData(GL_ARRAY_BUFFER, vbuf.size, nullptr, GL_DYNAMIC_DRAW);
        //DBG_GLSanityCheck(false, "vertex buffer allocation");
            
        vbuf.mapped = nullptr;
        vbuf.mapRefs = 0;
        vbuf.allocIndex = 0;

        return true;
    }

    VertexRegion_t* GLGeomBuffer::p_GetVertexRegion(GLVertexFormat* fmt, size_t numVerticesRequired)
    {
        const size_t size = numVerticesRequired * fmt->GetVertexSize();

        // Try to find a region with matching material and enough free space
        // TODO: Use binary search here

        iterate2 (region, regions)
        {
            if (region->type != Region_t::REGION_VERTEX)
                continue;

            auto vertexRegion = static_cast<VertexRegion_t*>(*region);

            if (vertexRegion->fmt == fmt && vertexRegion->linearAllocIndex + numVerticesRequired <= vertexRegion->capacityInVertices)
                return vertexRegion;
        }

        // No region found. Find a vertex buffer to create one in.

        size_t vbi;

        if (!p_ProvideVertexBufferObject(size, vbi))
            return nullptr;

        // For now, use the whole VBO

        auto vertexRegion = new VertexRegion_t;
        vertexRegion->vboIndex = (uint32_t) vbi;
        vertexRegion->offset = vbufs[vbi].allocIndex;
        vertexRegion->length = vbufs[vbi].size;
        vertexRegion->type = Region_t::REGION_VERTEX;

        vertexRegion->fmt = fmt;
        vertexRegion->vertexSize = fmt->GetVertexSize();
        vertexRegion->capacityInVertices = vertexRegion->length / vertexRegion->vertexSize;
        vertexRegion->linearAllocIndex = 0;
        regions.add(vertexRegion);

        vbufs[vbi].allocIndex += vertexRegion->length;

        //Debug_Printk(("[%s +region](%s, %u verts, %u bytes)", name.c_str(), material->GetName(),
        //        (unsigned) vertexRegion->capacityInVertices, (unsigned) vertexRegion->length));
        return vertexRegion;
    }

    void GLGeomBuffer::ReleaseChunk(GLGeomChunk* gc)
    {
        // FIXME: reclaim VBO space
    }

    void GLGeomBuffer::UpdateVertices(GLGeomChunk* gc, size_t first, const uint8_t* buffer, size_t sizeInBytes)
    {
        //ZFW_DBGASSERT(cmd->gc->region->type == VERTEX_REGION)
        auto region = static_cast<VertexRegion_t*>(gc->region);

        GLBindChunk(gc);
        glBufferSubData(GL_ARRAY_BUFFER, region->offset + (gc->index + first) * region->vertexSize, sizeInBytes, buffer);
    }
}
