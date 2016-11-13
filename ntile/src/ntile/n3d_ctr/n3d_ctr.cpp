
#include "n3d_ctr_internal.hpp"

#include <framework/colorconstants.hpp>
#include <framework/errorbuffer.hpp>
#include <framework/utility/pixmap.hpp>

#include "math.h"

//will be moved into ctrulib at some point
#define CONFIG_3D_SLIDERSTATE (*(float*)0x1FF81080)

extern "C" u64 osGetMicros();

namespace n3d
{
    using zfw::typeID;

    ISystem* g_sys;
    CTRRenderer* ctrr;

    unique_ptr<CTRVertexCache> vertexCache;

    static void s_CopyLine(uint8_t* line, u8* fb, unsigned int y, unsigned int fbWidth)
    {
        for (unsigned int x = 0; x < fbWidth; x++)
        {
            *line++ = fb[(x * 240 + 239 - y) * 3];
            *line++ = fb[(x * 240 + 239 - y) * 3 + 1];
            *line++ = fb[(x * 240 + 239 - y) * 3 + 2];
        }
    }

    static void onCtrglTimeout(CTRGLtimeoutType type)
    {
        g_sys->Printf(kLogError, "ctrGL timeout 0x%x!", type);
    }

    /*static void GetOpenGLDataType(AttribDataType datatype, GLenum& type, int& count)
    {
        switch (datatype)
        {
            case ATTRIB_UBYTE: case ATTRIB_UBYTE_2: case ATTRIB_UBYTE_3: case ATTRIB_UBYTE_4:
                type = GL_UNSIGNED_BYTE;
                count = datatype - ATTRIB_UBYTE + 1;
                break;
            
            case ATTRIB_SHORT: case ATTRIB_SHORT_2: case ATTRIB_SHORT_3: case ATTRIB_SHORT_4:
                type = GL_SHORT;
                count = datatype - ATTRIB_SHORT + 1;
                break;
                
            case ATTRIB_INT: case ATTRIB_INT_2: case ATTRIB_INT_3: case ATTRIB_INT_4:
                type = GL_INT;
                count = datatype - ATTRIB_INT + 1;
                break;
                
            case ATTRIB_FLOAT: case ATTRIB_FLOAT_2: case ATTRIB_FLOAT_3: case ATTRIB_FLOAT_4:
                type = GL_FLOAT;
                count = datatype - ATTRIB_FLOAT + 1;
                break;
            
            default:
                ZFW_ASSERT(false)
        }
    }*/
    
    /*static void SetUpFormat(const GLVertexFormat* format, bool picking)
    {
        gl.SetClientState(CL_GL_VERTEX_ARRAY, format->pos.location != -1);
        gl.SetClientState(CL_GL_NORMAL_ARRAY, format->normal.location != -1);
        gl.SetClientState(CL_GL_TEXTURE_COORD_ARRAY, format->uv0.location != -1);
        gl.SetClientState(CL_GL_COLOR_ARRAY, !picking && format->colour.location != -1);
        
        if (format->pos.location != -1)
            glVertexPointer(format->pos.components, format->pos.type, format->vertexSize,
                            reinterpret_cast<void*>(format->pos.offset));
        
        if (format->normal.location != -1)
            glNormalPointer(format->normal.type, format->vertexSize,
                            reinterpret_cast<void*>(format->normal.offset));
        
        if (format->uv0.location != -1)
            glTexCoordPointer(format->uv0.components, format->uv0.type, format->vertexSize,
                            reinterpret_cast<void*>(format->uv0.offset));
        
        if (!picking && format->colour.location != -1)
            glColorPointer(format->colour.components, format->colour.type, format->vertexSize,
                            reinterpret_cast<void*>(format->colour.offset));
    }*/

    CTRRenderer::CTRRenderer()
    {
        ctrr = this;

        drawColor = RGBA_WHITE;
    }

    bool CTRRenderer::Init()
    {
        GPU_Init(NULL);
        gfxSet3D(true);

        ctrglInit();
        ctrglAllocateCommandBuffers(0x4000, 2);
        ctrglSetTimeout(CTRGL_TIMEOUT_P3D, 300 * 1000 * 1000);          // 300ms for PICA
        ctrglSetTimeout(CTRGL_TIMEOUT_PPF, 100 * 1000 * 1000);          // 100ms for PPF
        ctrglSetTimeout(CTRGL_TIMEOUT_PSC0, 100 * 1000 * 1000);         // 100ms for PSC0
        ctrglSetTimeoutHandler(onCtrglTimeout);
        ctrglSetVsyncWait(GL_FALSE);

        ctrglResetGPU();

        // basic OpenGL settings
        glEnable(GL_CULL_FACE);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        glEnable(GL_BLEND);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        vertexCache.reset(new CTRVertexCache(16 * 1024));

        return true;
    }

    void CTRRenderer::Shutdown()
    {
        vertexCache.reset();

        ctrglExit();
    }

    void CTRRenderer::Begin3DScene()
    {
        //gl.BindTexture0(0);
        glEnable(GL_DEPTH_TEST);

        //SetFrameBuffer(renderBuffer);
    }

    void CTRRenderer::BeginFrame()
    {
        ctrglBeginRendering();

        vertexCache->Reset();
    }

    void CTRRenderer::BeginPicking()
    {/*
        gl.SetState(ST_GL_DEPTH_TEST, true);
        gl.SetState(ST_GL_TEXTURE_2D, false);

        SetFrameBuffer(nullptr);

        picking = true;*/
    }

    bool CTRRenderer::CaptureFrame(Pixmap_t* pm_out)
    {
        float slider = CONFIG_3D_SLIDERSTATE;

        static const unsigned int bottomOffset = (400 - 320) / 2;

        if (slider > 0.0f)
        {
            Pixmap::Initialize(pm_out, Int2(800, 480), PixmapFormat_t::BGR8);

            u8* fb1 = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
            u8* fb2 = gfxGetFramebuffer(GFX_TOP, GFX_RIGHT, NULL, NULL);

            for (unsigned int y = 0; y < 240; y++)
            {
                s_CopyLine(Pixmap::GetPixelDataForWriting(pm_out) + y * Pixmap::GetBytesPerLine(pm_out->info), fb1, y, 400);
                s_CopyLine(Pixmap::GetPixelDataForWriting(pm_out) + y * Pixmap::GetBytesPerLine(pm_out->info) + 400 * 3, fb2, y, 400);
            }

            u8* fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);

            for (unsigned int y = 0; y < 240; y++)
            {
                s_CopyLine(Pixmap::GetPixelDataForWriting(pm_out) + (y + 240) * Pixmap::GetBytesPerLine(pm_out->info) + bottomOffset * 3, fb, y, 320);
                s_CopyLine(Pixmap::GetPixelDataForWriting(pm_out) + (y + 240) * Pixmap::GetBytesPerLine(pm_out->info) + (400 + bottomOffset) * 3, fb, y, 320);
            }
        }
        else
        {
            Pixmap::Initialize(pm_out, Int2(400, 480), PixmapFormat_t::BGR8);

            u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);

            for (unsigned int y = 0; y < 240; y++)
                s_CopyLine(Pixmap::GetPixelDataForWriting(pm_out) + y * Pixmap::GetBytesPerLine(pm_out->info), fb, y, 400);

            fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);

            for (unsigned int y = 0; y < 240; y++)
                s_CopyLine(Pixmap::GetPixelDataForWriting(pm_out) + (y + 240) * Pixmap::GetBytesPerLine(pm_out->info) + bottomOffset * 3, fb, y, 320);
        }

        return true;
    }

    void CTRRenderer::Clear(Byte4 colour)
    {
        glClearColorRgba8CTR(glMakeRgba8CTR(colour.r, colour.g, colour.b, 0xFF));
    }
    
    IVertexFormat* CTRRenderer::CompileVertexFormat(IShaderProgram* program, unsigned int vertexSize, const VertexAttrib* attributes)
    {
        /*GLVertexFormat* fmt = new GLVertexFormat;
        fmt->vertexSize = vertexSize;
        
        fmt->pos.location =     -1;
        fmt->normal.location =  -1;
        fmt->uv0.location =     -1;
        fmt->colour.location =  -1;
        
        //fmt->numAttribs = 0;
        
        for ( ; attributes->name != nullptr; attributes++)
        {
            GLVertexAttrib attrib;
            
            if (strcmp(attributes->name, "pos") == 0)
                attrib.location = BUILTIN_POS;
            else if (strcmp(attributes->name, "normal") == 0)
                attrib.location = BUILTIN_NORMAL;
            else if (strcmp(attributes->name, "uv0") == 0)
                attrib.location = BUILTIN_UV0;
            else if (strcmp(attributes->name, "colour") == 0)
                attrib.location = BUILTIN_COLOUR;
            else
                ZFW_ASSERT(false)
            
            attrib.offset = attributes->offset;
            GetOpenGLDataType(attributes->datatype, attrib.type, attrib.components);
            
            if (attrib.location == BUILTIN_POS)
                fmt->pos = attrib;
            else if (attrib.location == BUILTIN_NORMAL)
                fmt->normal = attrib;
            else if (attrib.location == BUILTIN_UV0)
                fmt->uv0 = attrib;
            else if (attrib.location == BUILTIN_COLOUR)
                fmt->colour = attrib;
            //else
            //    fmt->attribs[fmt->numAttribs++] = attrib;
        }
        
        return fmt;*/
    }

    IModel* CTRRenderer::CreateModelFromMeshes(Mesh** meshes, size_t count)
    {
        unique_ptr<CTRModel> model(new CTRModel);

        for (size_t i = 0; i < count; i++)
            model->meshes.add(meshes[i]);

        return model.release();
    }

    IResource2* CTRRenderer::CreateResource(IResourceManager2* res, const TypeID& resourceClass, const char* recipe, int flags)
    {
        if (resourceClass == typeID<IFont>())
        {
            String path;
            unsigned int size = 0;

            const char *key, *value;
            int textureFlags = 0;

            while (Params::Next(recipe, key, value))
            {
                if (strcmp(key, "path") == 0)
                    path = value;
                else if (strcmp(key, "size") == 0)
                    size = strtoul(value, nullptr, 0);
            }

            zombie_assert(!path.isEmpty());
            zombie_assert(size > 0);

            return new CTRFont(std::move(path), size, textureFlags, 0);
        }
        else if (resourceClass == typeID<IShaderProgram>())
        {
            String path;

            const char *key, *value;

            while (Params::Next(recipe, key, value))
            {
                if (strcmp(key, "path") == 0)
                    path = value;
            }

            zombie_assert(!path.isEmpty());

            return new CTRShaderProgram(std::move(path));
        }

        /*if (resourceClass == typeid(IGraphics))
        {
            auto g = p_CreateGLGraphics();

            if (!g->Init(res, normparams, flags))
                g.reset();

            return g;
        }
        */
            /*case IShaderProgram::UNIQ:
            {
                String path;

                const char *key, *value;

                while (Params::Next(normparams, key, value))
                {
                    if (strcmp(key, "path") == 0)
                        path = value;
                }

                // FIXME: Replace with a nice check
                ZFW_ASSERT(!path.isEmpty())

                IGLShaderProgram* program = p_CreateShaderProgram(eb, rk, this, path);

                if (!program->GLCompile(path))
                {
                    zfw::Release(program);
                    return nullptr;
                }

                return program->GetResource();
            }*/
        /*else if (resourceClass == typeID<ITexture>())
        {
            String path;

            const char *key, *value;
            int textureFlags = TEXTURE_DEFAULT_FLAGS;

            while (Params::Next(normparams, key, value))
            {
                if (strcmp(key, "path") == 0)
                    path = value;
            }

            // FIXME: Replace with a nice check
            ZFW_ASSERT(!path.isEmpty())

            return LoadTexture(path, textureFlags, 0);
        }*/
        else
        {
            ZFW_ASSERT(resourceClass != resourceClass)
            return nullptr;
        }
    }

    void CTRRenderer::DrawPrimitives(IVertexBuffer* vb, PrimitiveType type, IVertexFormat* format, uint32_t offset, uint32_t count)
    {
        auto ctrvb = static_cast<CTRVertexBuffer*>(vb);

        glBindBuffer(GL_ARRAY_BUFFER, (GLuint) &ctrvb->vbo);

        switch (type)
        {
            //case PRIMITIVE_LINES:       glDrawArrays(GL_LINES, offset, count); break;
            case PRIMITIVE_TRIANGLES:   glDrawArrays(GL_TRIANGLES, offset, count); break;
            //case PRIMITIVE_QUADS:       glDrawArrays(GL_QUADS, offset, count); break;
        }

        // FIXME: How to control texture enable/disable and others?

        //vertexCache->Flush();

        /*auto glvb = static_cast<GLVertexBuffer*>(vb);

        gl.BindVBO(glvb->vbo);
        SetUpFormat(static_cast<GLVertexFormat*>(format), picking);

        switch (type)
        {
            case PRIMITIVE_LINES:       glDrawArrays(GL_LINES, offset, count); break;
            case PRIMITIVE_TRIANGLES:   glDrawArrays(GL_TRIANGLES, offset, count); break;
            case PRIMITIVE_QUADS:       glDrawArrays(GL_QUADS, offset, count); break;
        }*/
    }

    void CTRRenderer::DrawQuad(const Int3 abcd[4])
    {
        //vertexCache->Flush();

        /*gl.SetState(ST_GL_TEXTURE_2D, false);

        glBegin(GL_QUADS);
            glVertex3iv(&abcd[0].x);
            glVertex3iv(&abcd[1].x);
            glVertex3iv(&abcd[2].x);
            glVertex3iv(&abcd[3].x);
        glEnd();*/
    }

    void CTRRenderer::DrawRect(Int3 pos, Int2 size) 
    {
        UIVertex* vertices = static_cast<UIVertex*>(vertexCache->Alloc(this, 6 * sizeof(UIVertex)));

        glDisable(GL_TEXTURE_2D);

        vertices[0].x = pos.x;
        vertices[0].y = pos.y;
        vertices[0].z = pos.z;
        memcpy(&vertices[0].rgba[0], &drawColor[0], 4);
        vertices[0].u = 0.0f;
        vertices[0].v = 0.0f;
                
        vertices[1].x = pos.x;
        vertices[1].y = pos.y + size.y;
        vertices[1].z = pos.z;
        memcpy(&vertices[1].rgba[0], &drawColor[0], 4);
        vertices[1].u = 0.0f;
        vertices[1].v = 1.0f;
                
        vertices[2].x = pos.x + size.x;
        vertices[2].y = pos.y + size.y;
        vertices[2].z = pos.z;
        memcpy(&vertices[2].rgba[0], &drawColor[0], 4);
        vertices[2].u = 1.0f;
        vertices[2].v = 1.0f;

        vertices[3].x = pos.x;
        vertices[3].y = pos.y;
        vertices[3].z = pos.z;
        memcpy(&vertices[3].rgba[0], &drawColor[0], 4);
        vertices[3].u = 0.0f;
        vertices[3].v = 0.0f;

        vertices[4].x = pos.x + size.x;
        vertices[4].y = pos.y + size.y;
        vertices[4].z = pos.z;
        memcpy(&vertices[4].rgba[0], &drawColor[0], 4);
        vertices[4].u = 1.0f;
        vertices[4].v = 1.0f;

        vertices[5].x = pos.x + size.x;
        vertices[5].y = pos.y;
        vertices[5].z = pos.z;
        memcpy(&vertices[5].rgba[0], &drawColor[0], 4);
        vertices[5].u = 1.0f;
        vertices[5].v = 0.0f;

        vertexCache->Flush();
    }

    void CTRRenderer::DrawTexture(ITexture* tex, Int2 pos)
    {
        //vertexCache->Flush();
        /*
        auto gltex = static_cast<GLTexture*>(tex);
        gltex->Bind();

        gl.SetState(ST_GL_TEXTURE_2D, true);

        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f);
            glVertex2i(pos.x, pos.y);

            glTexCoord2f(0.0f, 1.0f);
            glVertex2i(pos.x, pos.y + gltex->size.y);

            glTexCoord2f(1.0f, 1.0f);
            glVertex2i(pos.x + gltex->size.x, pos.y + gltex->size.y);
            
            glTexCoord2f(1.0f, 0.0f);
            glVertex2i(pos.x + gltex->size.x, pos.y);
        glEnd();*/
    }

    void CTRRenderer::DrawTexture(ITexture* tex, Int2 pos, Int2 size, Int2 areaPos, Int2 areaSize)
    {
        //vertexCache->Flush();
        
        /*auto gltex = static_cast<GLTexture*>(tex);
        gltex->Bind();
        
        gl.SetState(ST_GL_TEXTURE_2D, true);

        const Float2 uv0 = Float2(areaPos) / Float2(gltex->size);
        const Float2 uv1 = Float2(areaPos + areaSize) / Float2(gltex->size);

        glBegin(GL_QUADS);
            glTexCoord2f(uv0.x, uv0.y);
            glVertex2i(pos.x, pos.y);

            glTexCoord2f(uv0.x, uv1.y);
            glVertex2i(pos.x, pos.y + size.y);

            glTexCoord2f(uv1.x, uv1.y);
            glVertex2i(pos.x + size.x, pos.y + size.y);

            glTexCoord2f(uv1.x, uv0.y);
            glVertex2i(pos.x + size.x, pos.y);
        glEnd();*/
    }

    void CTRRenderer::DrawTextureBillboard(ITexture* tex, Float3 pos, Float2 size)
    {
        //vertexCache->Flush();

        /*auto gltex = static_cast<GLTexture*>(tex);
        gltex->Bind();

        gl.SetState(ST_GL_TEXTURE_2D, true);

        glm::mat4x4 modelView;
        proj.GetModelViewMatrix(&modelView);

        const Float3 right = glm::normalize(Float3(modelView[0][0], modelView[1][0], modelView[2][0])) * (size.x / 2.0f);
        const Float3 up = glm::normalize(Float3(modelView[0][1], modelView[1][1], modelView[2][1])) * (size.y / 2.0f);

        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f);
            glVertex3f(pos.x + right.x - up.x, pos.y + right.y - up.y, pos.z + right.z - up.z);

            glTexCoord2f(0.0f, 1.0f);
            glVertex3f(pos.x + right.x + up.x, pos.y + right.y + up.y, pos.z + right.z + up.z);

            glTexCoord2f(1.0f, 1.0f);
            glVertex3f(pos.x - right.x + up.x, pos.y - right.y + up.y, pos.z - right.z + up.z);
            
            glTexCoord2f(1.0f, 0.0f);
            glVertex3f(pos.x - right.x - up.x, pos.y - right.y - up.y, pos.z - right.z - up.z);
        glEnd();*/
    }

    void CTRRenderer::End3DScene()
    {
        vertexCache->Flush();

        glDisable(GL_DEPTH_TEST);
        
        /*if (smaa != nullptr)
        {
            gl.SetState(ST_GL_BLEND, false);
            gl.SetState(ST_GL_DEPTH_TEST, false);

            smaa->Process(renderBuffer.get());

            gl.SetState(ST_GL_BLEND, true);
        }
        else
        {
            SetShaderProgram(0);
            SetFrameBuffer(nullptr);

            gl.SetState(ST_GL_DEPTH_TEST, false);

            // FIXME: Correctly flip texture (in SMAA as well)*/
            /*proj.SetOrtho(-1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f);
            proj.R_SetUpMatrices();
            DrawTexture(renderBuffer->texture, Int2(-1, -1), Int2(2, 2), Int2(0, 0), gl.renderViewport);*/
            
            //SetOrthoScreenSpace(-1.0f, 1.0f);
            //DrawTexture(renderBuffer->texture, Int2(0, 0));
        //}
    }

    void CTRRenderer::EndFrame(int ticksElapsed)
    {
        vertexCache->Flush();

        float slider = CONFIG_3D_SLIDERSTATE;

        if (slider > 0.0f)
            glStereoEnableCTR(std::min<float>(slider, 0.98f));
        else
            glStereoDisableCTR();

        auto t0 = osGetMicros();
        ctrglFinishRendering();
        renderingTime = osGetMicros() - t0;
        //gspWaitForEvent(GSPEVENT_VBlank0, true);
    }

    uint32_t CTRRenderer::EndPicking(Int2 samplePos)
    {
        // FIXME: check samplePos

        /*uint32_t colour = 0;
        glReadPixels(samplePos.x, gl.renderViewport.y - samplePos.y - 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &colour);

        picking = false;

        return colour & 0x00FFFFFF;*/
    }

    IVertexFormat* CTRRenderer::GetFontVertexFormat()
    {
        /*static const VertexAttrib fontVertexAttribs[] =
        {
            {0,     "pos",      ATTRIB_INT_3},
            {12,    "colour",   ATTRIB_UBYTE_4},
            {16,    "uv0",      ATTRIB_FLOAT_3},
            {}
        };

        if (fontVertexFormat == nullptr)
            fontVertexFormat.reset(glr->CompileVertexFormat(nullptr, 24, fontVertexAttribs));

        return fontVertexFormat.get();*/
    }

    void CTRRenderer::GetModelView(glm::mat4x4& modelView)
    {
         proj.GetModelViewMatrix(&modelView);
    }

    void CTRRenderer::GetProjectionModelView(glm::mat4x4& output)
    {
         proj.GetProjectionModelViewMatrix(&output);
    }

    void CTRRenderer::OnVertexCacheFlush(CTRVertexBuffer* vb, size_t offset, size_t bytesUsed)
    {
        //gl.SetState(ST_GL_TEXTURE_2D, true);
        //glr->SetTextureUnit(0, texture);
        DrawPrimitives(vb, PRIMITIVE_TRIANGLES, nullptr, (uint32_t)(offset / sizeof(UIVertex)), (uint32_t)(bytesUsed / sizeof(UIVertex)));
    }

    void CTRRenderer::PopTransform()
    {
        // TODO: optimize
        glm::mat4x4 in = matrixStack.back();
        matrixStack.pop_back();
        glModelviewMatrixfCTR(&in[0][0]);
    }

    void CTRRenderer::PushTransform(const glm::mat4x4& transform)
    {
        // TODO: optimize
        glm::mat4x4 in;
        glGetDirectMatrixfCTR(GL_MODELVIEW, &in[0][0]);

        matrixStack.push_back(in);
        in = glm::transpose(transform) * in;
        glModelviewMatrixfCTR(&in[0][0]);
    }

    void CTRRenderer::RegisterResourceProviders(IResourceManager2* res)
    {
        static const TypeID resourceClasses[] = {
            typeID<IFont>(), /*typeID<IGraphics>(),*/ typeID<IShaderProgram>(), typeID<ITexture>()
        };

        res->RegisterResourceProvider(resourceClasses, lengthof(resourceClasses), this);
    }


    /*const char* CTRRenderer::TryGetResourceClassName(const std::type_index& resourceClass)
    {
        if (resourceClass == typeid(::n3d::IGraphics))
            return "n3d::IGraphics";
        else if (resourceClass == typeID<::n3d::ITexture>())
            return "n3d::ITexture";
        else
            return nullptr;
    }*/

    float CTRRenderer::SampleDepth(Int2 samplePos)
    {
        // FIXME: check samplePos
        /*
        float depth = 0.0f;
        glReadPixels(samplePos.x, gl.renderViewport.y - samplePos.y - 1, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

        return depth;*/
    }

    void CTRRenderer::SetColour(const Byte4 colour)
    {
        drawColor = colour;
    }

    void CTRRenderer::SetColourv(const uint8_t* rgba)
    {
        memcpy(&drawColor[0], rgba, 4);
    }

    void CTRRenderer::SetFrameBuffer(IFrameBuffer* fb_in)
    {/*
        GLFrameBuffer* fb = static_cast<GLFrameBuffer*>(fb_in);

        if (fb == currentFB)
            return;

        if (fb != nullptr)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, fb->fbo);

            Int2 size;

            if (fb->GetSize(size))
                glViewport(0, 0, size.x, size.y);
        }
        else
        {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, gl.renderViewport.x, gl.renderViewport.y);
        }

        currentFB = fb;*/
    }

    void CTRRenderer::SetOrthoScreenSpace(float nearZ, float farZ)
    {
        proj.SetOrthoScreenSpace(nearZ, farZ);
        proj.R_SetUpMatrices();
    }

    void CTRRenderer::SetPerspective(float nearClip, float farClip)
    {
        proj.SetPerspective(nearClip, farClip);
        proj.R_SetUpMatrices();
    }

    void CTRRenderer::SetShaderProgram(IShaderProgram* program)
    {
        glUseProgram((GLuint) &static_cast<CTRShaderProgram*>(program)->shader);
    }

    void CTRRenderer::SetTextureUnit(int unit, ITexture* tex)
    {
        static_cast<CTRTexture*>(tex)->Bind();
    }

    void CTRRenderer::SetVFov(float vfov)
    {
        proj.SetVFov(vfov);
        proj.R_SetUpMatrices();
    }

    void CTRRenderer::SetView(const Float3& eye, const Float3& center, const Float3& up)
    {
        proj.SetView(eye, center, up);
        proj.R_SetUpMatrices();
    }
}
