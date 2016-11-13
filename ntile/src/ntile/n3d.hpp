#pragma once

#include "nbase.hpp"

#include <framework/datamodel.hpp>
#include <framework/resource.hpp>

#undef DrawText

// TODO: zfw::PrimitiveType will be removed. Replace it with our own enum.

namespace n3d
{
    using namespace zfw;

    class ITexture;
    class IRenderer;
    class IVertexBuffer;
    class IVertexFormat;

    enum AttribDataType
    {
        ATTRIB_UBYTE,
        ATTRIB_UBYTE_2,
        ATTRIB_UBYTE_3,
        ATTRIB_UBYTE_4,
        
        ATTRIB_SHORT,
        ATTRIB_SHORT_2,
        ATTRIB_SHORT_3,
        ATTRIB_SHORT_4,
        
        ATTRIB_INT,
        ATTRIB_INT_2,
        ATTRIB_INT_3,
        ATTRIB_INT_4,
        
        ATTRIB_FLOAT,
        ATTRIB_FLOAT_2,
        ATTRIB_FLOAT_3,
        ATTRIB_FLOAT_4,
    };

    enum
    {
        TEXTURE_BILINEAR =      1,
        TEXTURE_MIPMAPPED =     2,
        TEXTURE_AUTO_MIPMAPS =  4,
        TEXTURE_DEFAULT_FLAGS = TEXTURE_BILINEAR | TEXTURE_MIPMAPPED | TEXTURE_AUTO_MIPMAPS
    };

    enum
    {
        VIDEO_FULLSCREEN =      1
    };
    
    enum PrimitiveType
    {
        PRIMITIVE_LINES,
        PRIMITIVE_TRIANGLES,
        PRIMITIVE_TRIANGLE_STRIP,
        PRIMITIVE_QUADS,
    };

    struct Mesh
    {
        unique_ptr<IVertexBuffer> vb;
        PrimitiveType primitiveType;
        IVertexFormat* format;
        uint32_t offset, count;
        
        glm::mat4x4 transform;

        Mesh() {}

        Mesh(Mesh&& other) : vb(move(other.vb)), primitiveType(other.primitiveType),
                format(other.format), offset(other.offset), count(other.count), transform(other.transform)
        {
            // ayy lmao MSVC bugs
        }
    };

    struct VertexAttrib
    {
        unsigned int offset;
        const char* name;
        AttribDataType datatype;
    };

    class IPlatform
    {
        public:
            virtual ~IPlatform() {}

            virtual void Init() = 0;
            virtual void Shutdown() = 0;

            /*virtual IRenderer* SetGLVideoMode(int w, int h, int flags) = 0;
            virtual void SetMultisampleLevel(int level) = 0;
            virtual void SetSwapControl(int swapControl) = 0;*/
    };

    class IFont : public IResource2
    {
        public:
            virtual ~IFont() {}

            virtual void DrawText(const char* utf8, Int3 pos, Byte4 colour, int flags) = 0;
            virtual void DrawText(const char* utf8, size_t length, Int3 pos, Byte4 colour, int flags) = 0;
            virtual Int2 MeasureText(const char* text, int flags) = 0;
    };

    class IFrameBuffer
    {
        public:
            virtual ~IFrameBuffer() {}

            virtual ITexture* GetTexture() = 0;
    };

    class IModel : public IResource2
    {
        public:
            virtual ~IModel() {}
        
            virtual void Draw() = 0;
            virtual Mesh* GetMeshByIndex(unsigned int index) = 0;
    };
    
    class IProjectionBuffer
    {
        public:
            enum Projection { PROJ_ORTHO, PROJ_ORTHO_SS, PROJ_PERSPECTIVE };

            virtual ~IProjectionBuffer() {}

            //virtual ProjectionBuffer* Clone() = 0;

            virtual float CameraGetDistance() = 0;
            virtual void CameraMove(const Float3& vec) = 0;
            virtual void CameraRotateXY(float alpha, bool absolute) = 0;
            virtual void CameraRotateZ(float alpha, bool absolute) = 0;
            virtual void CameraZoom(float amount, bool absolute) = 0;

            virtual void GetModelViewMatrix(glm::mat4* output) = 0;
            //virtual void GetProjectionMatrix(glm::mat4* output) = 0;
            virtual void GetProjectionModelViewMatrix(glm::mat4* output) = 0;

            virtual void SetOrtho(float left, float right, float top, float bottom, float nearZ, float farZ) = 0;
            virtual void SetOrthoScreenSpace(float nearZ, float farZ) = 0;
            virtual void SetPerspective(float nearClip, float farClip) = 0;
            virtual void SetVFov(float vfov) = 0;
            virtual void SetView(const Float3& eye, const Float3& center, const Float3& up) = 0;
    };

    class ITexture : public IResource2
    {
        public:
            virtual ~ITexture() {}

            virtual Int2 GetSize() = 0;
    };

    class IShaderProgram : public IResource2
    {
        public:
            virtual ~IShaderProgram() {}

            virtual int GetUniform(const char* name) = 0;
            virtual void SetUniformVec3(int id, const Float3& value) = 0;
            virtual void SetUniformVec4(int id, const Float4& value) = 0;
    };

    class IVertexBuffer
    {
        public:
            virtual ~IVertexBuffer() {}

            virtual bool Alloc(size_t size) = 0;

            virtual void* Map(bool read, bool write) = 0;
            virtual void Unmap() = 0;

            virtual bool Upload(const uint8_t* data, size_t length) = 0;
            virtual bool UploadRange(size_t offset, size_t length, const uint8_t* data) = 0;
    };

    class IVertexFormat
    {
        public:
            virtual ~IVertexFormat() {}

            virtual uint32_t GetVertexSize() = 0;
    };

    class IRenderer
    {
        protected:
            ~IRenderer() {}

        public:
            //virtual bool Init() = 0;
            //virtual void Shutdown() = 0;

            virtual void RegisterResourceProviders(IResourceManager2* resMgr) = 0;

            virtual IVertexFormat*  CompileVertexFormat(IShaderProgram* program, unsigned int vertexSize, const VertexAttrib* attributes) = 0;
            virtual IFrameBuffer*   CreateFrameBuffer(Int2 size, bool withDepth) = 0;
            virtual IModel*         CreateModelFromMeshes(Mesh** meshes, size_t count) = 0;
            //virtual IProjectionBuffer* CreateProjectionBuffer() = 0;
            virtual IVertexBuffer*  CreateVertexBuffer() = 0;
            //virtual IFont*          LoadFont(const char* path, int scale, int textureFlags, int loadFlags = 0) = 0;
            //virtual IShaderProgram* LoadShaderProgram(const char* path, int loadFlags = 0) = 0;
            //virtual shared_ptr<ITexture>    LoadTexture(const char* path, int textureFlags, int loadFlags = 0) = 0;
            //virtual ITexture*       LoadTextureMipmaps(const char* path, unsigned count, float lodBias, int loadFlags = 0) = 0;

            virtual void Begin3DScene() = 0;
            virtual void End3DScene() = 0;

            virtual void BeginPicking() = 0;
            virtual uint32_t EndPicking(Int2 samplePos) = 0;

            virtual float SampleDepth(Int2 samplePos) = 0;

            virtual void Clear(Byte4 colour) = 0;
            virtual void DrawPrimitives(IVertexBuffer* vb, PrimitiveType type, IVertexFormat* format, uint32_t offset, uint32_t count) = 0;
            virtual void DrawQuad(const Int3 abcd[4]) = 0;
            virtual void DrawRect(Int3 pos, Int2 size) = 0;
            virtual void DrawTexture(ITexture* tex, Int2 pos) = 0;
            virtual void DrawTexture(ITexture* tex, Int2 pos, Int2 size, Int2 areaPos, Int2 areaSize) = 0;
            virtual void DrawTextureBillboard(ITexture* tex, Float3 pos, Float2 size) = 0;
            virtual void GetModelView(glm::mat4x4& modelView) = 0;
            virtual void GetProjectionModelView(glm::mat4x4& output) = 0;
            virtual void PopTransform() = 0;
            virtual void PushTransform(const glm::mat4x4& transform) = 0;
            virtual void SetColour(const Byte4 colour) = 0;
            virtual void SetColourv(const uint8_t* rgba) = 0;
            virtual void SetFrameBuffer(IFrameBuffer* fb) = 0;
            virtual void SetOrthoScreenSpace(float nearZ, float farZ) = 0;
            virtual void SetPerspective(float nearClip, float farClip) = 0;
            virtual void SetShaderProgram(IShaderProgram* program) = 0;
            virtual void SetTextureUnit(int unit, ITexture* tex) = 0;
            virtual void SetVFov(float vfov) = 0;
            virtual void SetView(const Float3& eye, const Float3& center, const Float3& up) = 0;
    };
}