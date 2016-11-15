
#include "RenderingKitImpl.hpp"

namespace RenderingKit
{
    static void GetOpenGLDataType(RKAttribDataType_t datatype, GLenum& type, int& count, uint32_t& sizeInBytes)
    {
        switch (datatype)
        {
            case RK_ATTRIB_UBYTE: case RK_ATTRIB_UBYTE_2: case RK_ATTRIB_UBYTE_3: case RK_ATTRIB_UBYTE_4:
                type = GL_UNSIGNED_BYTE;
                count = datatype - RK_ATTRIB_UBYTE + 1;
                sizeInBytes = count;
                break;

            case RK_ATTRIB_SHORT: case RK_ATTRIB_SHORT_2: case RK_ATTRIB_SHORT_3: case RK_ATTRIB_SHORT_4:
                type = GL_SHORT;
                count = datatype - RK_ATTRIB_SHORT + 1;
                sizeInBytes = count * 2;
                break;

            case RK_ATTRIB_INT: case RK_ATTRIB_INT_2: case RK_ATTRIB_INT_3: case RK_ATTRIB_INT_4:
                type = GL_INT;
                count = datatype - RK_ATTRIB_INT + 1;
                sizeInBytes = count * 4;
                break;

            case RK_ATTRIB_FLOAT: case RK_ATTRIB_FLOAT_2: case RK_ATTRIB_FLOAT_3: case RK_ATTRIB_FLOAT_4:
                type = GL_FLOAT;
                count = datatype - RK_ATTRIB_FLOAT + 1;
                sizeInBytes = count * 4;
                break;

            default:
                ZFW_ASSERT(false)
        }
    }

    void GLVertexFormat::Cleanup()
    {
    }

    void GLVertexFormat::Compile(IShader* program, uint32_t vertexSize, const VertexAttrib_t* attributes, bool groupedByAttrib)
    {
        this->vertexSize = vertexSize;
        this->groupedByAttrib = groupedByAttrib;

        pos.location =     -1;
        normal.location =  -1;
        uv0.location =     -1;
        colour.location =  -1;

        numAttribs = 0;

        ZFW_ASSERT(program != nullptr)

        for ( ; attributes->name != nullptr; attributes++)
        {
            GLVertexAttrib_t attrib;

            attrib.location = static_cast<IGLShaderProgram*>(program)->GLGetAttribLocation(attributes->name);
            //printf("%s] %s -> %i\n", program->GetName(), attributes->name, (int) attrib.location);

            if (attrib.location == -1)
                continue;

            attrib.offset = attributes->offset;
            GetOpenGLDataType(attributes->datatype, attrib.type, attrib.components, attrib.sizeInBytes);
            attrib.flags = attributes->flags;

            attribs[numAttribs++] = attrib;
        }
    }

    /*bool GLVertexFormat::GetComponent(const char* componentName, Component<Float3>& component)
    {
        if (strcmp(componentName, "pos") == 0 && pos.type == GL_FLOAT && pos.components >= 3)
        {
            if (!groupedByAttrib)
                component = Component<Float3>(pos.offset, vertexSize);
            else
                component = Component<Float3>(0, pos.sizeInBytes);

            return true;
        }
        
        return false;
    }
    
    bool GLVertexFormat::GetComponent(const char* componentName, Component<Byte4>& component)
    {
        if (strcmp(componentName, "rgba") == 0 && colour.type == GL_UNSIGNED_BYTE && colour.components >= 4)
        {
            if (!groupedByAttrib)
                component = Component<Byte4>(colour.offset, vertexSize);
            else
                component = Component<Byte4>(0, colour.sizeInBytes);

            return true;
        }
        
        return false;
    }
    
    bool GLVertexFormat::GetComponent(const char* componentName, Component<Float2>& component)
    {
        if (strcmp(componentName, "uv0") == 0 && uv0.type == GL_FLOAT && uv0.components >= 2)
        {
            if (!groupedByAttrib)
                component = Component<Float2>(uv0.offset, vertexSize);
            else
                component = Component<Float2>(0, uv0.sizeInBytes);

            return true;
        }
        
        return false;
    }*/

    bool GLVertexFormat::InitializeVao(GLuint* vao_out)
    {
        glGenVertexArrays(1, vao_out);
        glBindVertexArray(*vao_out);

        for (size_t i = 0; i < numAttribs; i++)
        {
            const auto& attrib = attribs[i];

#ifdef RENDERING_KIT_USING_OPENGL_ES
			zombie_assert(attrib.type != GL_INT);
#endif

            glEnableVertexAttribArray(attrib.location);
            glVertexAttribPointer(attrib.location, attrib.components, attrib.type,
                    (attrib.flags & RK_ATTRIB_NOT_NORMALIZED) ? GL_FALSE : GL_TRUE,
                    vertexSize, reinterpret_cast<void*>(attrib.offset));
        }

        return true;
    }

    void GLVertexFormat::Setup(GLuint vbo, int fpMaterialFlags)
    {
        ZFW_ASSERT(!groupedByAttrib)

        if (vboBinding.vao == 0)
        {
            ZFW_ASSERT(InitializeVao(&vboBinding.vao));
            vboBinding.vbo = vbo;
        }
        else
        {
            // Yes, we know this is broken.
            ZFW_ASSERT(vboBinding.vbo == vbo);
        }

        glBindVertexArray(vboBinding.vao);

        ZFW_ASSERT(glGetError() == GL_NO_ERROR);
    }
}
