
#include "rendering.hpp"

#include <littl/Stack.hpp>

namespace zr
{
    // Private types
    enum BatchMode { BATCH_IDLE, BATCH_2D, BATCH_3D_SIMPLE };

    // Mapping tables
    const GLenum gl_formats[] = { 0, GL_SHORT, GL_UNSIGNED_BYTE, GL_FLOAT };
    const GLenum gl_primmode[] = { GL_LINES, GL_TRIANGLES };

    // Shared structs
    FrameStats frameStats;
    RendererStats rendererStats;

    GLuint currentArrayBuffer, currentElementBuffer, currentTexture2D;
    render::GLProgram* boundProgram;
    const VertexFormat* boundFormat;

    uint32_t nextPickingId;

    // Private
    static BatchMode batchMode;

    static Mesh* boxMesh;
    
    Int4 currentViewport;
    static Stack<Int4> scissorStack;
    static Stack<Int4> viewportStack;

    // TODO: review
    static RenderBuffer * currentRenderBuffer = nullptr;
    static Stack<RenderBuffer *> renderBuffers;

    // Vars
    static Var_t *r_batchallocsize = nullptr;

    struct Vertex2D
    {
        short x, y;
        float u, v;
        uint32_t rgba;
    };

    struct Vertex3DSimple
    {
        float x, y, z;
        float u, v;
        uint32_t rgba;
    };

#define VERTEX2D_1(i_, x_, y_, rgba_)\
    vertices[i_].x = x_;\
    vertices[i_].y = y_;\
    vertices[i_].rgba = rgba_

#define VERTEX2D_UV_RGBA(i_, x_, y_, u_, v_, rgba_)\
    vertices[i_].x = x_;\
    vertices[i_].y = y_;\
    vertices[i_].u = u_;\
    vertices[i_].v = v_;\
    vertices[i_].rgba = rgba_
    
#define VERTEX3DSIMPLE_1(i_, x_, y_, z_, rgba_)\
    vertices[i_].x = x_;\
    vertices[i_].y = y_;\
    vertices[i_].z = z_;\
    vertices[i_].rgba = rgba_

#define VERTEX3DSIMPLE_2(i_, x_, y_, z_, u_, v_, rgba_)\
    vertices[i_].x = x_;\
    vertices[i_].y = y_;\
    vertices[i_].z = z_;\
    vertices[i_].u = u_;\
    vertices[i_].v = v_;\
    vertices[i_].rgba = rgba_

    inline void BatchAlloc(BatchMode mode, size_t amount)
    {
        if (batchMode != mode)
        {
            FlushBatch();

            ZFW_ASSERT(VAR_INT(r_batchallocsize) > 0)

            if (mode == BATCH_2D)
                Batch::Begin(VAR_INT(r_batchallocsize) * sizeof(Vertex2D));
            else if (mode == BATCH_3D_SIMPLE)
                Batch::Begin(VAR_INT(r_batchallocsize) * sizeof(Vertex3DSimple));

            batchMode = mode;
        }

        if (batch.bufferUsed + amount > batch.bufferCapacity)
        {
            if (batch.flags & BATCH_MMAPPED)
                zfw::Sys::RaiseException(zfw::EX_BACKEND_ERR, "rendering/BatchAlloc", "Memory-mapped vertex buffer overflow");
            else
            {
                // FIXME: realloc()
                ZFW_ASSERT(false)
            }
        }
    }

    /*inline void CheckBatchBuffer(const size_t amount)
    {
        if (batch.bufferUsed + amount > batch.bufferCapacity)
        {
            if (batch.flags & BATCH_MMAPPED)
                zfw::Sys::RaiseException(zfw::EX_BACKEND_ERR, "R::_CheckBatchBuffer", "Memory-mapped vertex buffer overflow");
            else
            {
                // FIXME
                ZFW_ASSERT(false)
            }
        }
    }*/

    static zr::Mesh* GetBoxMesh()
    {
        if(boxMesh == nullptr)
        {
            media::DIMesh mesh;
            media::PresetFactory::CreateCube(Float3(1.0f, 1.0f, 1.0f), Float3(), media::ALL_SIDES | media::WITH_NORMALS | media::WITH_UVS, &mesh);

            boxMesh = zr::Mesh::CreateFromMemory(&mesh);
        }

        return boxMesh;
    }
    
    static void SetRenderBuffer(RenderBuffer* renderBuffer)
    {
        // FIXME: review

        if ( currentRenderBuffer == renderBuffer )
            return;

        if ( renderBuffer != nullptr )
        {
            glBindFramebuffer( GL_FRAMEBUFFER_EXT, renderBuffer->buffer );

            if ( renderBuffer->isSetUp() )
                //glViewport( 0, 0, renderBuffer->getWidth(), renderBuffer->getHeight() );
            {
                currentViewport = Int4( 0, 0, renderBuffer->getWidth(), renderBuffer->getHeight() );
                glViewport(currentViewport.x, currentViewport.y, currentViewport.z, currentViewport.w);
            }
        }
        else
        {
            glBindFramebuffer( GL_FRAMEBUFFER_EXT, 0 );
            //glViewport( viewportPos.x, windowSize.y - viewportPos.y - viewport.y, viewport.x, viewport.y );
            glViewport(currentViewport.x, currentViewport.y, currentViewport.z, currentViewport.w);
        }

        currentRenderBuffer = renderBuffer;
    }

    static void Perspective( float fov, float aspect, float zNear, float zFar )
    {
        float fH = ( float )( zNear * tan(fov / 2.0f) );
        float fW = fH * aspect;

        glFrustum( -fW, fW, -fH, fH, zNear, zFar );
    }

    void Renderer::Init()
    {
        rendererStats.batchesLost = 0;

        currentArrayBuffer = 0;
        currentElementBuffer = 0;
        currentTexture2D = 0;
        boundProgram = nullptr;
        boundFormat = nullptr;

        boxMesh = nullptr;

        r_batchallocsize = Var::LockInt("r_batchallocsize", false);

        ZFW_ASSERT(sizeof(Vertex2D) == 16)
        ZFW_ASSERT(sizeof(Vertex3DSimple) == 24)
        zr::Batch::Init();
    }

    void Renderer::Exit()
    {
        Var::Unlock(r_batchallocsize);

        zr::Batch::Exit();

        li::destroy(boxMesh);
    }
    
    void Renderer::BeginFrame()
    {
        if (glGetError() != 0)
            zfw::Sys::RaiseException(zfw::EX_BACKEND_ERR, "Renderer::BeginFrame", zfw::Sys::tmpsprintf(50, "OpenGL error $%04X"));

        frameStats.bytesStreamed = 0;
        frameStats.batchesDrawn = 0;
        frameStats.polysDrawn = 0;
        frameStats.textureSwitches = 0;
        frameStats.shaderSwitches = 0;
        
        batchMode = BATCH_IDLE;
    }

    void Renderer::BeginPicking()
    {
        glDisable( GL_BLEND );
        glDisable( GL_DITHER );
        glDisable( GL_FOG );
        glDisable( GL_TEXTURE_1D );
        glDisable( GL_TEXTURE_2D );
        glDisable( GL_TEXTURE_3D );
        glShadeModel( GL_FLAT );

        nextPickingId = 1;

        /*if ( driverShared.useShaders )
            getPickingShaderProgram()->select();
        else
            glDisable( GL_LIGHTING );*/

        render::R::SelectShadingProgram(nullptr);
        glDisable( GL_LIGHTING );

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    }

    const FrameStats* Renderer::EndFrame()
    {
        return &frameStats;
    }

    uint32_t Renderer::EndPicking(CInt2& samplePos)
    {
        if (samplePos.x < 0 || samplePos.y < 0
                || samplePos.x >= render::r_windoww->basictypes.i
                || samplePos.y >= render::r_windowh->basictypes.i)
            return 0;

        uint32_t id = 0;

        glReadPixels(samplePos.x, render::r_windowh->basictypes.i - samplePos.y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &id);

        glEnable( GL_BLEND );
        glShadeModel( GL_SMOOTH );

        printf( "%u,%u->%08X\n", samplePos.x, samplePos.y, id );
        return id & 0xFFFFFF;
    }

    uint32_t Renderer::GetPickingId()
    {
        return nextPickingId++;
    }

    void Renderer::SetPickingId(uint32_t id)
    {
        glColor4ubv((const GLubyte *) &id);
    }

    /*void R::Begin2D(size_t numVertices)
    {
        ZFW_ASSERT(sizeof(Vertex2D) == 16)
        
        Batch::Begin(numVertices * sizeof(Vertex2D));
        
        batchMode = BATCH_2D;
    }*/
    
    void R::Clear()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void R::DrawBox(CFloat3& pos, CFloat3& size)
    {
        GetBoxMesh()->Render(glm::scale(glm::translate(glm::mat4(), pos), size));
    }

    void R::DrawBox(CFloat3& pos, CFloat3& size, Byte4 colour)
    {
        SetBlendColour(colour);
        GetBoxMesh()->Render(glm::scale(glm::translate(glm::mat4(), pos), size));
    }

    void R::DrawBox(CFloat3& pos, CFloat3& size, CFloat4& colour)
    {
        SetBlendColour(colour);
        GetBoxMesh()->Render(glm::scale(glm::translate(glm::mat4(), pos), size));
    }

    void R::DrawFilledQuad(CFloat2 pos[4], CFloat2 uv[4], CByte4 colour[4])
    {
        BatchAlloc(BATCH_2D, 6 * sizeof(Vertex2D));
        
        Vertex2D * vertices = (Vertex2D *) ((uint8_t *) batch.buffer + batch.bufferUsed);
        register const uint32_t *rgba = (uint32_t *)&colour[0];
        
        VERTEX2D_UV_RGBA(0, pos[0].x, pos[0].y, uv[0].x, uv[0].y, rgba[0]);
        VERTEX2D_UV_RGBA(1, pos[1].x, pos[1].y, uv[1].x, uv[1].y, rgba[1]);
        VERTEX2D_UV_RGBA(2, pos[3].x, pos[3].y, uv[3].x, uv[3].y, rgba[3]);
        
        VERTEX2D_UV_RGBA(3, pos[3].x, pos[3].y, uv[3].x, uv[3].y, rgba[1]);
        VERTEX2D_UV_RGBA(4, pos[1].x, pos[1].y, uv[1].x, uv[1].y, rgba[3]);
        VERTEX2D_UV_RGBA(5, pos[2].x, pos[2].y, uv[2].x, uv[2].y, rgba[2]);
        
        batch.bufferUsed += 6 * sizeof(Vertex2D);
        batch.numVertices += 6;
    }
    
    void R::DrawFilledRect(CShort2& a, CShort2& b, CByte4 colour)
    {
        BatchAlloc(BATCH_2D, 6 * sizeof(Vertex2D));
        
        Vertex2D * vertices = (Vertex2D *) ((uint8_t *) batch.buffer + batch.bufferUsed);
        register const uint32_t rgba = *(uint32_t *)&colour[0];
        
        VERTEX2D_1(0, a.x, a.y, rgba);
        VERTEX2D_1(1, a.x, b.y, rgba);
        VERTEX2D_1(2, b.x, a.y, rgba);

        VERTEX2D_1(3, b.x, a.y, rgba);
        VERTEX2D_1(4, a.x, b.y, rgba);
        VERTEX2D_1(5, b.x, b.y, rgba);

        batch.bufferUsed += 6 * sizeof(Vertex2D);
        batch.numVertices += 6;
    }
    
    void R::DrawFilledRect(CShort2& a, CShort2& b, CFloat2& uva, CFloat2& uvb, CByte4 colour)
    {
        BatchAlloc(BATCH_2D, 6 * sizeof(Vertex2D));
        
        Vertex2D * vertices = (Vertex2D *) ((uint8_t *) batch.buffer + batch.bufferUsed);
        register const uint32_t rgba = *(uint32_t *)&colour[0];
        
        vertices[0].x = a.x;
        vertices[0].y = a.y;
        vertices[0].u = uva.x;
        vertices[0].v = uva.y;
        vertices[0].rgba = rgba;
        
        vertices[1].x = a.x;
        vertices[1].y = b.y;
        vertices[1].u = uva.x;
        vertices[1].v = uvb.y;
        vertices[1].rgba = rgba;
        
        vertices[2].x = b.x;
        vertices[2].y = a.y;
        vertices[2].u = uvb.x;
        vertices[2].v = uva.y;
        vertices[2].rgba = rgba;
        
        vertices[3].x = b.x;
        vertices[3].y = a.y;
        vertices[3].u = uvb.x;
        vertices[3].v = uva.y;
        vertices[3].rgba = rgba;
        
        vertices[4].x = a.x;
        vertices[4].y = b.y;
        vertices[4].u = uva.x;
        vertices[4].v = uvb.y;
        vertices[4].rgba = rgba;
        
        vertices[5].x = b.x;
        vertices[5].y = b.y;
        vertices[5].u = uvb.x;
        vertices[5].v = uvb.y;
        vertices[5].rgba = rgba;
        
        batch.bufferUsed += 6 * sizeof(Vertex2D);
        batch.numVertices += 6;
    }

    void R::DrawFilledRect(CFloat3& a, CFloat3& b, CByte4 colour)
    {
        BatchAlloc(BATCH_3D_SIMPLE, 6 * sizeof(Vertex3DSimple));
        
        Vertex3DSimple * vertices = (Vertex3DSimple *) ((uint8_t *) batch.buffer + batch.bufferUsed);
        register const uint32_t rgba = *(uint32_t *)&colour[0];
        
        VERTEX3DSIMPLE_1(0, a.x, a.y, a.z, rgba);
        VERTEX3DSIMPLE_1(1, a.x, b.y, b.z, rgba);
        VERTEX3DSIMPLE_1(2, b.x, a.y, a.z, rgba);
        
        VERTEX3DSIMPLE_1(3, b.x, a.y, a.z, rgba);
        VERTEX3DSIMPLE_1(4, a.x, b.y, b.z, rgba);
        VERTEX3DSIMPLE_1(5, b.x, b.y, b.z, rgba);
        
        batch.bufferUsed += 6 * sizeof(Vertex3DSimple);
        batch.numVertices += 6;
    }
    
    void R::DrawFilledTri(CShort2& a, CShort2& b, CShort2& c, CByte4 colour)
    {
        BatchAlloc(BATCH_2D, 3 * sizeof(Vertex2D));
        
        Vertex2D * vertices = (Vertex2D *) ((uint8_t *) batch.buffer + batch.bufferUsed);
        register const uint32_t rgba = *(uint32_t *)&colour[0];
        
        VERTEX2D_1(0, a.x, a.y, rgba);
        VERTEX2D_1(1, b.x, b.y, rgba);
        VERTEX2D_1(2, c.x, c.y, rgba);

        batch.bufferUsed += 3 * sizeof(Vertex2D);
        batch.numVertices += 3;
    }

    void R::DrawRectBillboard(CFloat3& pos, CFloat2& size)
    {
        BatchAlloc(BATCH_3D_SIMPLE, 6 * sizeof(Vertex3DSimple));

        // FIXMEEEEE I'M UGLY
        glm::mat4x4 modelView;
        glGetFloatv(GL_MODELVIEW_MATRIX, &modelView[0][0]);

        CFloat3 right = glm::normalize(Float3(modelView[0][0], modelView[1][0], modelView[2][0])) * size.x / 2.0f;
        CFloat3 up = glm::normalize(Float3(modelView[0][1], modelView[1][1], modelView[2][1])) * size.y / 2.0f;

        Vertex3DSimple * vertices = (Vertex3DSimple *) ((uint8_t *) batch.buffer + batch.bufferUsed);
        register const uint32_t rgba = /**(uint32_t *)&colour[0]*/ 0xFFFFFFFF;

        VERTEX3DSIMPLE_1(0, pos.x + right.x - up.x, pos.y + right.y - up.y, pos.z + right.z - up.z, rgba);
        VERTEX3DSIMPLE_1(1, pos.x + right.x + up.x, pos.y + right.y + up.y, pos.z + right.z + up.z, rgba);
        VERTEX3DSIMPLE_1(2, pos.x - right.x - up.x, pos.y - right.y - up.y, pos.z - right.z - up.z, rgba);
        VERTEX3DSIMPLE_1(3, pos.x - right.x - up.x, pos.y - right.y - up.y, pos.z - right.z - up.z, rgba);
        VERTEX3DSIMPLE_1(4, pos.x + right.x + up.x, pos.y + right.y + up.y, pos.z + right.z + up.z, rgba);
        VERTEX3DSIMPLE_1(5, pos.x - right.x + up.x, pos.y - right.y + up.y, pos.z - right.z + up.z, rgba);

        batch.bufferUsed += 6 * sizeof(Vertex3DSimple);
        batch.numVertices += 6;
    }

    void R::DrawRectBillboard(CFloat3& pos, CFloat2& size, CFloat2& uva, CFloat2& uvb, CByte4 colour)
    {
        BatchAlloc(BATCH_3D_SIMPLE, 6 * sizeof(Vertex3DSimple));

        glm::mat4x4 modelView;
        glGetFloatv(GL_MODELVIEW_MATRIX, &modelView[0][0]);

        CFloat3 right = glm::normalize(Float3(modelView[0][0], modelView[1][0], modelView[2][0])) * size.x / 2.0f;
        CFloat3 up = glm::normalize(Float3(modelView[0][1], modelView[1][1], modelView[2][1])) * size.y / 2.0f;

        Vertex3DSimple * vertices = (Vertex3DSimple *) ((uint8_t *) batch.buffer + batch.bufferUsed);
        register const uint32_t rgba = *(uint32_t *)&colour[0];

        VERTEX3DSIMPLE_2(0, pos.x + right.x - up.x, pos.y + right.y - up.y, pos.z + right.z - up.z, uva.x, 1.0f - uva.y, rgba);
        VERTEX3DSIMPLE_2(1, pos.x + right.x + up.x, pos.y + right.y + up.y, pos.z + right.z + up.z, uva.x, 1.0f - uvb.y, rgba);
        VERTEX3DSIMPLE_2(2, pos.x - right.x - up.x, pos.y - right.y - up.y, pos.z - right.z - up.z, uvb.x, 1.0f - uva.y, rgba);
        VERTEX3DSIMPLE_2(3, pos.x - right.x - up.x, pos.y - right.y - up.y, pos.z - right.z - up.z, uvb.x, 1.0f - uva.y, rgba);
        VERTEX3DSIMPLE_2(4, pos.x + right.x + up.x, pos.y + right.y + up.y, pos.z + right.z + up.z, uva.x, 1.0f - uvb.y, rgba);
        VERTEX3DSIMPLE_2(5, pos.x - right.x + up.x, pos.y - right.y + up.y, pos.z - right.z + up.z, uvb.x, 1.0f - uvb.y, rgba);

        batch.bufferUsed += 6 * sizeof(Vertex3DSimple);
        batch.numVertices += 6;
    }
    
    void R::DrawRectRotated(CFloat3& pos, CFloat2& size, CFloat2& origin, CByte4 colour, float angle)
    {
        BatchAlloc(BATCH_3D_SIMPLE, 6 * sizeof(Vertex3DSimple));

        Vertex3DSimple * vertices = (Vertex3DSimple *) ((uint8_t *) batch.buffer + batch.bufferUsed);
        register const uint32_t rgba = *(uint32_t *)&colour[0];
        
        float sin_a = sin(angle);
        float cos_a = cos(angle);
        
        CFloat2 a0(-origin.x, -origin.y);
        CFloat2 b0(-origin.x, size.y - origin.y);
        CFloat2 c0(size.x - origin.x, size.y - origin.y);
        CFloat2 d0(size.x - origin.x, -origin.y);

        CFloat2 a(pos.x + a0.x * cos_a + a0.y * sin_a, pos.y - a0.x * sin_a + a0.y * cos_a);
        CFloat2 b(pos.x + b0.x * cos_a + b0.y * sin_a, pos.y - b0.x * sin_a + b0.y * cos_a);
        CFloat2 c(pos.x + c0.x * cos_a + c0.y * sin_a, pos.y - c0.x * sin_a + c0.y * cos_a);
        CFloat2 d(pos.x + d0.x * cos_a + d0.y * sin_a, pos.y - d0.x * sin_a + d0.y * cos_a);

        VERTEX3DSIMPLE_1(0, a.x, a.y, pos.z, rgba);
        VERTEX3DSIMPLE_1(1, b.x, b.y, pos.z, rgba);
        VERTEX3DSIMPLE_1(2, d.x, d.y, pos.z, rgba);
        
        VERTEX3DSIMPLE_1(3, d.x, d.y, pos.z, rgba);
        VERTEX3DSIMPLE_1(4, b.x, b.y, pos.z, rgba);
        VERTEX3DSIMPLE_1(5, c.x, c.y, pos.z, rgba);

        batch.bufferUsed += 6 * sizeof(Vertex3DSimple);
        batch.numVertices += 6;
    }

    void R::DrawTexture(render::ITexture* tex, CShort2& pos, CByte4 colour)
    {
        SetTexture(tex);
        DrawFilledRect(pos, pos + Short2(tex->GetSize()), Float2(0.0f, 0.0f), Float2(1.0f, 1.0f), colour);
    }

    void R::DrawTexture(render::ITexture* tex, CShort2& pos, CFloat2& uva, CFloat2& uvb, CByte4 colour)
    {
        SetTexture(tex);
        DrawFilledRect(pos, pos + Short2(tex->GetSize()), uva, uvb, colour);
    }

    void R::EnableDepthTest( bool enable )
    {
        GLSet( DEPTH_TEST, enable);
    }

    static void R_Flush2D()
    {
        ZFW_ASSERT(batch.numVertices < INT_MAX)
        
        GLSetClientState(VERTEX_ARRAY, true)
        glVertexPointer(2, GL_SHORT, 16, (void*) 0);
        
        GLSetClientState(NORMAL_ARRAY, false)
        
        GLSetClientState(TEXTURE_COORD_ARRAY, true)
        glTexCoordPointer(2, GL_FLOAT, 16, (void*) 4);
        
        GLSetClientState(COLOR_ARRAY, true)
        glColorPointer(4, GL_UNSIGNED_BYTE, 16, (void*) 12);
        
        glDrawArrays(GL_TRIANGLES, 0, (int) batch.numVertices);
        frameStats.bytesStreamed += batch.bufferUsed;
        frameStats.polysDrawn += batch.numVertices;
        frameStats.batchesDrawn++;
        
        batch.bufferUsed = 0;
        batch.numVertices = 0;
    }

    static void R_Flush3DS()
    {
        ZFW_ASSERT(batch.numVertices < INT_MAX)
        
        GLSetClientState(VERTEX_ARRAY, true)
        glVertexPointer(3, GL_FLOAT, 24, (void*) 0);
        
        GLSetClientState(NORMAL_ARRAY, false)
        
        GLSetClientState(TEXTURE_COORD_ARRAY, true)
        glTexCoordPointer(2, GL_FLOAT, 24, (void*) 12);
        
        GLSetClientState(COLOR_ARRAY, true)
        glColorPointer(4, GL_UNSIGNED_BYTE, 24, (void*) 20);
        
        glDrawArrays(GL_TRIANGLES, 0, (int) batch.numVertices);
        frameStats.bytesStreamed += batch.bufferUsed;
        frameStats.polysDrawn += batch.numVertices;
        frameStats.batchesDrawn++;
        
        batch.bufferUsed = 0;
        batch.numVertices = 0;
    }
    
    void R::PopClippingRect()
    {
        scissorStack.pop();
        
        if (!scissorStack.isEmpty())
        {
            CInt4& rect = scissorStack.top();
        
            glScissor(rect.x, rect.y, rect.z, rect.w);
        }
        else
            GLSet(SCISSOR_TEST, false);
    }

    void R::PopRenderBuffer()
    {
        FlushBatch();

        currentViewport = viewportStack.pop();
        SetRenderBuffer( renderBuffers.pop() );
    }
    
    void R::PushClippingRect(int x, int y, int w, int h)
    {
        y = render::r_windowh->basictypes.i - y - h;
        
        GLSet(SCISSOR_TEST, true);
        glScissor(x, y, w, h);
        
        scissorStack.push(Int4(x, y, w, h));
    }

    void R::PushRenderBuffer(RenderBuffer* renderBuffer)
    {
        FlushBatch();

        renderBuffers.push( currentRenderBuffer );
        viewportStack.push(currentViewport);

        SetRenderBuffer( (RenderBuffer *) renderBuffer );
    }

    bool R::ReadDepth(int x, int y, float& depth)
    {
        if (x < 0 || y < 0 || x >= render::r_windoww->basictypes.i || y >= render::r_windowh->basictypes.i )
            return false;
        
        glReadPixels(x, render::r_windowh->basictypes.i - y - 1, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
        return true;
    }
    
    void R::ResetView()
    {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }

    void R::SelectShadingProgram( IProgram* program )
    {
        if ( boundProgram != static_cast<render::GLProgram*>(program) )
        {
            FlushBatch();

            boundProgram = static_cast<render::GLProgram*>(program);

            glUseProgram( boundProgram != nullptr ? boundProgram->prog : 0 );
            frameStats.shaderSwitches++;

            // FIXME: render::R sets currentTexture2D to zero here
        }
    }

    void R::Set2DView()
    {
        FlushBatch();

        glMatrixMode( GL_PROJECTION );
        glLoadIdentity();

        glOrtho( 0.0, currentViewport.z, currentViewport.w, 0.0, 1.0f, -1.0f );

        glMatrixMode( GL_MODELVIEW );
        glLoadIdentity();
    }

    void R::SetBlendColour(Byte4 colour)
    {
        glColor4ubv(&colour[0]);
    }

    void R::SetBlendColour(const Float4& colour)
    {
        //zfw::render::currentColour = colour;
        glColor4fv(&colour[0]);

            /*if ( colour != currentColour )
            {
                Renderer::Flush();

                //if ( boundProgram != nullptr )
                //    boundProgram->SetBlendColour( colour );
                //else
                    glColor4fv( &colour[0] );

                currentColour = colour;
            }*/
    }

    void R::SetCamera( const world::Camera& cam )
    {
        R::SetCamera( cam.eye, cam.center, cam.up );
    }

    void R::SetCamera( const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up )
    {
        glm::mat4 modelView;
        
        modelView = glm::lookAt( glm::vec3( eye.x, -eye.y, eye.z ), glm::vec3( center.x, -center.y, center.z ), glm::vec3( up.x, -up.y, up.z ) );
        modelView = glm::scale( modelView, glm::vec3( 1.0f, -1.0f, 1.0f ) );
        
        glLoadMatrixf( &modelView[0][0] );
    }

    void R::SetClearColour(float r, float g, float b, float a)
    {
        glClearColor(r, g, b, a);
    }
    
    void R::SetPerspectiveProjection( float nearz, float farz, float fov )
    {
        FlushBatch();
        
        glMatrixMode( GL_PROJECTION );
        glLoadIdentity();
        
        const GLfloat viewportRatio = (GLfloat)VAR_INT(render::r_viewportw) / (GLfloat)VAR_INT(render::r_viewporth);
        
        Perspective( fov, viewportRatio, nearz, farz );
        
        glMatrixMode( GL_MODELVIEW );
        glLoadIdentity();
    }
    
    void R::SetTexture(render::ITexture* tex)
    {
        FlushBatch();
        render::R::SetTexture(tex);
    }
    
    bool R::WorldToViewport( const Float4& world, Int2& viewport )
    {
        glm::mat4x4 proj, mv;
        glGetFloatv(GL_PROJECTION_MATRIX, &proj[0][0]);
        glGetFloatv(GL_MODELVIEW_MATRIX, &mv[0][0]);
        const auto proj_mat = proj * mv;

        CFloat2 wiewport_space(Util::TransformVec(world, proj_mat));
        viewport = Int2(Float2(wiewport_space.x + 1.0f, 1.0f - wiewport_space.y)
                * Float2(render::r_viewportw->basictypes.i / 2, render::r_viewporth->basictypes.i / 2));

        return true;
    }

    // zr shared functions
    void FlushBatch()
    {
        Batch::Finish();
        
        boundFormat = nullptr;

        if (batch.numVertices == 0)
            return;

        if (batchMode == BATCH_2D)
        {
            R_Flush2D();
        }
        else if (batchMode == BATCH_3D_SIMPLE)
        {
            R_Flush3DS();
        }
        else 
            ZFW_ASSERT(batchMode == BATCH_IDLE)
    }

    void SetUpRendering(const VertexFormat& format)
    {
        if (boundFormat == &format)
            return;

        GLSetClientState( VERTEX_ARRAY, true );
        glVertexPointer( format.pos.components, gl_formats[format.pos.format], format.vertexSize, (void*) format.pos.offset );
        
        if (format.normal.format != 0)
        {
            GLSetClientState( NORMAL_ARRAY, true );
            glNormalPointer( gl_formats[format.normal.format], format.vertexSize, (void*) format.normal.offset );
        }
        else
            GLSetClientState( NORMAL_ARRAY, false );
        
        if (format.uv[0].format != 0)
        {
            GLSetClientState( TEXTURE_COORD_ARRAY, true );
            glTexCoordPointer( format.uv[0].components, gl_formats[format.uv[0].format], format.vertexSize, (void*) format.uv[0].offset );
        }
        else
            GLSetClientState( TEXTURE_COORD_ARRAY, false );
        
        if (format.colour.format != 0)
        {
            GLSetClientState( COLOR_ARRAY, true );
            glColorPointer( format.colour.components, gl_formats[format.colour.format], format.vertexSize, (void*) format.colour.offset );
        }
        else
            GLSetClientState( COLOR_ARRAY, false );

        boundFormat = &format;
    }
}
