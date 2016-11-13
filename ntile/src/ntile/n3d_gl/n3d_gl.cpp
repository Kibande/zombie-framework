
#include "n3d_gl_internal.hpp"

#include "SMAA/AreaTex.h"
#include "SMAA/SearchTex.h"

#include <framework/errorbuffer.hpp>
#include <framework/system.hpp>

namespace n3d
{
    ISystem* g_sys;
    GLRenderer* glr;
    OpenGLState gl;
    unique_ptr<GLVertexCache> vertexCache;

    // FIXME: Replace all ZFW_ASSERT(false)
    
    static void GetOpenGLDataType(AttribDataType datatype, GLenum& type, int& count)
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
    }
    
    static void SetUpFormat(const GLVertexFormat* format, bool picking)
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
    }

    GLRenderer::GLRenderer()
    {
        glr = this;

        shaderPreprocessor = nullptr;
    }

    bool GLRenderer::Init()
    {
        g_sys->Printf(kLogInfo, "Starting n3D OpenGL renderer");
        g_sys->Printf(kLogInfo, "OpenGL renderer: %s", (const char*) glGetString(GL_RENDERER));
        g_sys->Printf(kLogInfo, "OpenGL vendor:   %s", (const char*) glGetString(GL_VENDOR));
        g_sys->Printf(kLogInfo, "OpenGL version:  %s", (const char*) glGetString(GL_VERSION));
        g_sys->Printf(kLogInfo, "OpenGL shading language version: %s", (const char*) glGetString(GL_SHADING_LANGUAGE_VERSION));

        gl.SetState(ST_GL_BLEND, true);
        gl.SetState(ST_GL_DEPTH_TEST, true);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        picking = false;

        currentFB = nullptr;
        smaa = nullptr;
        fontVertexFormat = nullptr;

        // FIXME: Resizing etc.
        renderBuffer.reset(new GLFrameBuffer(gl.renderViewport, true));
        
        vertexCache.reset(new GLVertexCache(16 * 1024));

        return true;
    }

    void GLRenderer::Shutdown()
    {
        renderBuffer.reset();
        shaderPreprocessor.reset();
        vertexCache.reset();
        fontVertexFormat.reset();
    }

    void GLRenderer::Begin3DScene()
    {
        gl.BindTexture0(0);
        gl.SetState(ST_GL_DEPTH_TEST, true);

        //SetFrameBuffer(renderBuffer);
    }

    void GLRenderer::BeginPicking()
    {
        gl.SetState(ST_GL_DEPTH_TEST, true);
        gl.SetState(ST_GL_TEXTURE_2D, false);

        SetFrameBuffer(nullptr);

        picking = true;
    }

    void GLRenderer::Clear(Byte4 colour)
    {
        glClearColor(colour.r / 255.0f, colour.g / 255.0f, colour.b / 255.0f, colour.a / 255.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
    
    IVertexFormat* GLRenderer::CompileVertexFormat(IShaderProgram* program, unsigned int vertexSize, const VertexAttrib* attributes)
    {
        GLVertexFormat* fmt = new GLVertexFormat;
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
        
        return fmt;
    }

    IModel* GLRenderer::CreateModelFromMeshes(Mesh** meshes, size_t count)
    {
        unique_ptr<GLModel> model(new GLModel("kek"));

        //for (size_t i = 0; i < count; i++)
        //    model->meshes.add(meshes[i]);

        return model.release();
    }

    IResource2* GLRenderer::CreateResource(IResourceManager2* res, const TypeID& resourceClass,
            const char* recipe, int flags)
    {
        if (resourceClass == typeID<IGraphics2>())
        {
            return p_CreateGLGraphics2(recipe);
        }
        else if (resourceClass == typeID<IFont>())
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

            return new GLFont(std::move(path), size, textureFlags, 0);
        }
        else if (resourceClass == typeid(n3d::IModel))
        {
            String path;

            const char *key, *value;

            while (Params::Next(recipe, key, value))
            {
                if (strcmp(key, "path") == 0)
                    path = value;
            }

            zombie_assert(!path.isEmpty());

            auto mdl = new GLModel(std::move(path));
            zombie_assert(mdl);
            return mdl;
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

            return new GLShaderProgram(this, std::move(path));
        }
        else if (resourceClass == typeID<ITexture>())
        {
            String path;
            unsigned int numMipmaps = 1;
            float lodBias = 0.0f;

            const char *key, *value;
            int textureFlags = TEXTURE_DEFAULT_FLAGS;

            while (Params::Next(recipe, key, value))
            {
                if (strcmp(key, "path") == 0)
                    path = value;
                else if (strcmp(key, "lodBias") == 0)
                    lodBias = (float) strtod(value, nullptr);
                else if (strcmp(key, "numMipmaps") == 0)
                    numMipmaps = strtoul(value, nullptr, 0);
            }

            zombie_assert(!path.isEmpty());

            return new GLTexture(std::move(path), textureFlags, numMipmaps, lodBias);
        }
        else
        {
            zombie_assert(resourceClass != resourceClass);
            return nullptr;
        }
    }

    void GLRenderer::DrawPrimitives(IVertexBuffer* vb, PrimitiveType type, IVertexFormat* format, uint32_t offset, uint32_t count)
    {
        // FIXME: How to control texture enable/disable and others?

        //vertexCache->Flush();

        auto glvb = static_cast<GLVertexBuffer*>(vb);

        gl.BindVBO(glvb->vbo);
        SetUpFormat(static_cast<GLVertexFormat*>(format), picking);

        switch (type)
        {
            case PRIMITIVE_LINES:       glDrawArrays(GL_LINES, offset, count); break;
            case PRIMITIVE_TRIANGLES:   glDrawArrays(GL_TRIANGLES, offset, count); break;
            case PRIMITIVE_TRIANGLE_STRIP: glDrawArrays(GL_TRIANGLE_STRIP, offset, count); break;
            case PRIMITIVE_QUADS:       glDrawArrays(GL_QUADS, offset, count); break;
            default: zombie_assert(type != type);
        }
    }

    void GLRenderer::DrawQuad(const Int3 abcd[4])
    {
        //vertexCache->Flush();

        gl.SetState(ST_GL_TEXTURE_2D, false);

        glBegin(GL_QUADS);
            glVertex3iv(&abcd[0].x);
            glVertex3iv(&abcd[1].x);
            glVertex3iv(&abcd[2].x);
            glVertex3iv(&abcd[3].x);
        glEnd();
    }

    void GLRenderer::DrawRect(Int3 pos, Int2 size) 
    {
        gl.SetState(ST_GL_TEXTURE_2D, false);

        glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f);
            glVertex2i(pos.x, pos.y);

            glTexCoord2f(0.0f, 1.0f);
            glVertex2i(pos.x, pos.y + size.y);

            glTexCoord2f(1.0f, 1.0f);
            glVertex2i(pos.x + size.x, pos.y + size.y);

            glTexCoord2f(1.0f, 0.0f);
            glVertex2i(pos.x + size.x, pos.y);
        glEnd();
    }

    void GLRenderer::DrawTexture(ITexture* tex, Int2 pos)
    {
        //vertexCache->Flush();
        
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
        glEnd();
    }

    void GLRenderer::DrawTexture(ITexture* tex, Int2 pos, Int2 size, Int2 areaPos, Int2 areaSize)
    {
        //vertexCache->Flush();
        
        auto gltex = static_cast<GLTexture*>(tex);
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
        glEnd();
    }

    void GLRenderer::DrawTextureBillboard(ITexture* tex, Float3 pos, Float2 size)
    {
        //vertexCache->Flush();

        if (!picking)
        {
            auto gltex = static_cast<GLTexture*>(tex);
            gltex->Bind();

            gl.SetState(ST_GL_TEXTURE_2D, true);
        }
        else
            gl.SetState(ST_GL_TEXTURE_2D, false);

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
        glEnd();
    }

    void GLRenderer::End3DScene()
    {
        //vertexCache->Flush();
        
        if (smaa != nullptr)
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

            // FIXME: Correctly flip texture (in SMAA as well)
            /*proj.SetOrtho(-1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f);
            proj.R_SetUpMatrices();
            DrawTexture(renderBuffer->texture, Int2(-1, -1), Int2(2, 2), Int2(0, 0), gl.renderViewport);*/
            
            //SetOrthoScreenSpace(-1.0f, 1.0f);
            //DrawTexture(renderBuffer->texture, Int2(0, 0));
        }
    }

    uint32_t GLRenderer::EndPicking(Int2 samplePos)
    {
        // FIXME: check samplePos

        uint32_t colour = 0;
        glReadPixels(samplePos.x, gl.renderViewport.y - samplePos.y - 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &colour);

        picking = false;

        return colour & 0x00FFFFFF;
    }

    IVertexFormat* GLRenderer::GetFontVertexFormat()
    {
        static const VertexAttrib fontVertexAttribs[] =
        {
            {0,     "pos",      ATTRIB_INT_3},
            {12,    "colour",   ATTRIB_UBYTE_4},
            {16,    "uv0",      ATTRIB_FLOAT_3},
            {}
        };

        if (fontVertexFormat == nullptr)
            fontVertexFormat.reset(glr->CompileVertexFormat(nullptr, 24, fontVertexAttribs));

        return fontVertexFormat.get();
    }

    void GLRenderer::GetModelView(glm::mat4x4& modelView)
    {
         proj.GetModelViewMatrix(&modelView);
    }

    void GLRenderer::GetProjectionModelView(glm::mat4x4& output)
    {
         proj.GetProjectionModelViewMatrix(&output);
    }

    bool GLRenderer::LoadShader(const char* path, char** source_out)
    {
        if (shaderPreprocessor == nullptr)
            shaderPreprocessor.reset(g_sys->CreateShaderPreprocessor());

        return shaderPreprocessor->LoadShader(path, "", source_out);
    }

    void GLRenderer::PopTransform()
    {
        glPopMatrix();
    }

    void GLRenderer::PushTransform(const glm::mat4x4& transform)
    {
        glm::mat4x4 modelView;

        glPushMatrix();
        glGetFloatv(GL_MODELVIEW_MATRIX, &modelView[0][0]);

        modelView *= transform;
        glLoadMatrixf(&modelView[0][0]);
    }

    void GLRenderer::RegisterResourceProviders(IResourceManager2* res)
    {
        static const TypeID resourceClasses[] = {
            typeID<IFont>(), typeID<IGraphics2>(), typeID<IModel>(), typeID<IShaderProgram>(), typeID<ITexture>()
        };

        res->RegisterResourceProvider(resourceClasses, lengthof(resourceClasses), this);
    }

    /*const char* GLRenderer::TryGetResourceClassName(const TypeID& resourceClass)
    {
        if (resourceClass == typeID<::n3d::IGraphics>())
            return "n3d::IGraphics";
        else if (resourceClass == typeID<::n3d::ITexture>())
            return "n3d::ITexture";
        else
            return nullptr;
    }*/

    float GLRenderer::SampleDepth(Int2 samplePos)
    {
        // FIXME: check samplePos

        float depth = 0.0f;
        glReadPixels(samplePos.x, gl.renderViewport.y - samplePos.y - 1, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

        return depth;
    }

    void GLRenderer::SetColour(const Byte4 colour)
    {
        glColor4ubv(&colour[0]);
    }

    void GLRenderer::SetColourv(const uint8_t* rgba)
    {
        glColor4ubv(rgba);
    }

    void GLRenderer::SetFrameBuffer(IFrameBuffer* fb_in)
    {
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

        currentFB = fb;
    }

    void GLRenderer::SetOrthoScreenSpace(float nearZ, float farZ)
    {
        proj.SetOrthoScreenSpace(nearZ, farZ);
        proj.R_SetUpMatrices();
    }

    void GLRenderer::SetPerspective(float nearClip, float farClip)
    {
        proj.SetPerspective(nearClip, farClip);
        proj.R_SetUpMatrices();
    }

    void GLRenderer::SetShaderProgram(IShaderProgram* program)
    {
        if (program != nullptr)
            glUseProgram(static_cast<GLShaderProgram*>(program)->program);
        else
            glUseProgram(0);
    }

    void GLRenderer::SetTextureUnit(int unit, ITexture* tex)
    {
        // FIXME: Track this!!
        glActiveTexture(GL_TEXTURE0 + unit);
        static_cast<GLTexture*>(tex)->Bind();
        glActiveTexture(GL_TEXTURE0);
    }

    void GLRenderer::SetVFov(float vfov)
    {
        proj.SetVFov(vfov);
        proj.R_SetUpMatrices();
    }

    void GLRenderer::SetView(const Float3& eye, const Float3& center, const Float3& up)
    {
        proj.SetView(eye, center, up);
        proj.R_SetUpMatrices();
    }
}
