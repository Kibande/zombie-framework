
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

    GLVertexFormat::GLVertexFormat(const VertexFormatInfo& vf_in, GlobalCache& gc)
            : vertexSize(vf_in.vertexSizeInBytes)
    {
        numAttribs = 0;

        for (size_t i = 0; i < vf_in.numAttribs; i++)
        {
            const auto& attrib_in = vf_in.attribs[i];
            GLVertexAttrib_t attrib_baked;

            if (attrib_in.location >= 0) {
                attrib_baked.location = attrib_in.location;
            }
            else {
                zombie_assert(attrib_in.name != nullptr);
                attrib_baked.location = gc.GetAttribLocationByName(attrib_in.name);
            }

            attrib_baked.offset = attrib_in.offset;
            GetOpenGLDataType(attrib_in.datatype, attrib_baked.type, attrib_baked.components, attrib_baked.sizeInBytes);
            attrib_baked.flags = attrib_in.flags;

            this->attribs[this->numAttribs++] = attrib_baked;
        }
    }

    bool GLVertexFormat::Equals(const VertexFormatInfo& vf_in)
    {
        // TODO
        return false;
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

    void GLVertexFormat::Setup()
    {
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
    }
}
