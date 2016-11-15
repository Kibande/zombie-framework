#pragma once

#include "../RenderingKit.hpp"
#include "../TypeReflection.hpp"

#include <framework/errorcheck.hpp>
#include <framework/resourcemanager.hpp>

#include <cstddef>

// FIXME: rm->VertexCacheFlush();

namespace RenderingKit
{
    template <typename PosType, typename UVType, typename ColourType>
    struct TexturedVertex_t
    {
        typedef TexturedVertex_t<PosType, UVType, ColourType> ThisType;

        PosType     pos;
        UVType      uv;
        ColourType  colour;

        static const VertexAttrib_t* GetVertexAttribs()
        {
            static const VertexAttrib_t vertexAttribs[] =
            {
                {"in_Position", offsetof(ThisType, pos),    TypeReflection<PosType>::attribDataType,
                        RK_ATTRIB_NOT_NORMALIZED},
                {"in_UV",       offsetof(ThisType, uv),     TypeReflection<UVType>::attribDataType},
                {"in_Color",    offsetof(ThisType, colour), TypeReflection<ColourType>::attribDataType},
                {}
            };

            return vertexAttribs;
        }
    };

    template <typename PosType, typename UVType, typename ColourType>
    class TexturedPainter
    {
        public:
            typedef TexturedVertex_t<PosType, UVType, ColourType> VertexType;

            TexturedPainter();

            bool Init(IRenderingManager* rm);
            void Shutdown();

        protected:
            IRenderingManager* rm;

            shared_ptr<IVertexFormat> vertexFormat;

            shared_ptr<IMaterial> material;

            void DropResources();

            inline MaterialSetupOptions SetTexture(ITexture* texture)
            {
                MaterialSetupOptions options;
                options.type = MaterialSetupOptions::kSingleTextureOverride;
                options.data.singleTex.tex = texture;
                return options;
            }
    };

    template <typename PosType = zfw::Short2, typename UVType = zfw::Float2, typename ColourType = zfw::Byte4>
    class TexturedPainter2D : public TexturedPainter<PosType, UVType,  ColourType>
    {
        public:
            typedef typename TexturedPainter<PosType, UVType,  ColourType>::VertexType VertexType;

            void DrawFilledRectangle(ITexture* texture, const PosType& pos, const PosType& size, const ColourType& colour);
            void DrawFilledRectangleUV(ITexture* texture, const PosType& pos, const PosType& size, const UVType uv[2], const ColourType& colour);
    };

    template <typename PosType = zfw::Float3, typename UVType = zfw::Float2, typename ColourType = zfw::Byte4>
    class TexturedPainter3D : public TexturedPainter<PosType, UVType,  ColourType>
    {
        public:
            typedef typename TexturedPainter<PosType, UVType,  ColourType>::VertexType VertexType;

            void DrawFilledCuboid(ITexture* texture, const PosType& pos, const PosType& size, const ColourType& colour);
            void DrawFilledPlane(ITexture* texture, const PosType& pos, const PosType& size, const ColourType& colour);
            void DrawFilledTriangleUV(ITexture* texture, const PosType abc[3], const UVType uv[3], const ColourType colours[3]);
    };

    // ====================================================================== //
    //  class TexturedPainter
    // ====================================================================== //

    template <typename PosType, typename UVType, typename ColourType>
    TexturedPainter<PosType, UVType, ColourType>::TexturedPainter()
    {
        rm = nullptr;
    }

    template <typename PosType, typename UVType, typename ColourType>
    bool TexturedPainter<PosType, UVType, ColourType>::Init(IRenderingManager* rm)
    {
        this->rm = rm;

        auto shader = rm->GetSharedResourceManager()->GetResource<IShader>("path=RenderingKit/basicTextured", zfw::RESOURCE_REQUIRED, 0);
        zombie_ErrorCheck(shader);

        vertexFormat = rm->CompileVertexFormat(shader.get(), sizeof(VertexType), VertexType::GetVertexAttribs(), false);

        material = rm->CreateMaterial("TexturedPainter/material", shader);
        material->SetTexture("tex", nullptr);

        return true;
    }

    template <typename PosType, typename UVType, typename ColourType>
    void TexturedPainter<PosType, UVType, ColourType>::Shutdown()
    {
        DropResources();
    }

    template <typename PosType, typename UVType, typename ColourType>
    void TexturedPainter<PosType, UVType, ColourType>::DropResources()
    {
        material.reset();
        vertexFormat.reset();
    }

    // ====================================================================== //
    //  class TexturedPainter2D
    // ====================================================================== //

#define vert(x_, y_, u_, v_, colour_) do {\
        p_vertices->pos.x = x_;\
        p_vertices->pos.y = y_;\
        p_vertices->uv.x = u_;\
        p_vertices->uv.y = v_;\
        p_vertices->colour = colour_;\
        p_vertices++;\
    } while (false)

    template <typename PosType, typename UVType, typename ColourType>
    void TexturedPainter2D<PosType, UVType, ColourType>::DrawFilledRectangle(ITexture* texture,
            const PosType& pos, const PosType& size, const ColourType& colour)
    {
        static const auto u0 = TypeReflection<decltype(std::declval<UVType>().x)>::zero;
        static const auto v0 = TypeReflection<decltype(std::declval<UVType>().y)>::zero;
        static const auto u1 = TypeReflection<decltype(std::declval<UVType>().x)>::one;
        static const auto v1 = TypeReflection<decltype(std::declval<UVType>().y)>::one;

        VertexType* p_vertices = this->rm->template VertexCacheAllocAs<VertexType>(this->vertexFormat.get(),
                this->material.get(), this->SetTexture(texture), RK_TRIANGLES, 6);

        vert(pos.x,             pos.y,          u0, v0, colour);
        vert(pos.x,             pos.y + size.y, u0, v1, colour);
        vert(pos.x + size.x,    pos.y,          u1, v0, colour);
        vert(pos.x + size.x,    pos.y,          u1, v0, colour);
        vert(pos.x,             pos.y + size.y, u0, v1, colour);
        vert(pos.x + size.x,    pos.y + size.y, u1, v1, colour);
    }

    template <typename PosType, typename UVType, typename ColourType>
    void TexturedPainter2D<PosType, UVType, ColourType>::DrawFilledRectangleUV(ITexture* texture, const PosType& pos, const PosType& size, const UVType uv[2], const ColourType& colour)
    {
        VertexType* p_vertices = this->rm->template VertexCacheAllocAs<VertexType>(this->vertexFormat.get(),
                this->material.get(), this->SetTexture(texture), RK_TRIANGLES, 6);

        vert(pos.x,             pos.y,          uv[0].x,    uv[0].y,    colour);
        vert(pos.x,             pos.y + size.y, uv[0].x,    uv[1].y,    colour);
        vert(pos.x + size.x,    pos.y,          uv[1].x,    uv[0].y,    colour);
        vert(pos.x + size.x,    pos.y,          uv[1].x,    uv[0].y,    colour);
        vert(pos.x,             pos.y + size.y, uv[0].x,    uv[1].y,    colour);
        vert(pos.x + size.x,    pos.y + size.y, uv[1].x,    uv[1].y,    colour);
    }

#undef vert

    // ====================================================================== //
    //  class TexturedPainter3D
    // ====================================================================== //

#define vert(x_, y_, z_, u_, v_, colour_) do {\
        p_vertices->pos.x = x_;\
        p_vertices->pos.y = y_;\
        p_vertices->pos.z = z_;\
        p_vertices->uv.x = u_;\
        p_vertices->uv.y = v_;\
        p_vertices->colour = colour_;\
        p_vertices++;\
    } while (false)

    template <typename PosType, typename UVType, typename ColourType>
    void TexturedPainter3D<PosType, UVType, ColourType>::DrawFilledCuboid(ITexture* texture, const PosType& pos, const PosType& size, const ColourType& colour)
    {
        static const auto u0 = TypeReflection<decltype(std::declval<UVType>().x)>::zero;
        static const auto v0 = TypeReflection<decltype(std::declval<UVType>().y)>::zero;
        static const auto u1 = TypeReflection<decltype(std::declval<UVType>().x)>::one;
        static const auto v1 = TypeReflection<decltype(std::declval<UVType>().y)>::one;

        VertexType* p_vertices = this->rm->template VertexCacheAllocAs<VertexType>(this->vertexFormat.get(),
                this->material.get(), this->SetTexture(texture), RK_TRIANGLES, 6 * 6);

        // front
        vert(pos.x,             pos.y + size.y, pos.z + size.z, u0, v0, colour);
        vert(pos.x,             pos.y + size.y, pos.z,          u0, v1, colour);
        vert(pos.x + size.x,    pos.y + size.y, pos.z + size.z, u1, v0, colour);

        vert(pos.x + size.x,    pos.y + size.y, pos.z + size.z, u1, v0, colour);
        vert(pos.x,             pos.y + size.y, pos.z,          u0, v1, colour);
        vert(pos.x + size.x,    pos.y + size.y, pos.z,          u1, v1, colour);

        // back
        vert(pos.x + size.x,    pos.y,          pos.z + size.z, u0, v0, colour);
        vert(pos.x + size.x,    pos.y,          pos.z,          u0, v1, colour);
        vert(pos.x,             pos.y,          pos.z + size.z, u1, v0, colour);

        vert(pos.x,             pos.y,          pos.z + size.z, u1, v0, colour);
        vert(pos.x + size.x,    pos.y,          pos.z,          u0, v1, colour);
        vert(pos.x,             pos.y,          pos.z,          u1, v1, colour);

        // left
        vert(pos.x,             pos.y,          pos.z + size.z, u0, v0, colour);
        vert(pos.x,             pos.y,          pos.z,          u0, v1, colour);
        vert(pos.x,             pos.y + size.y, pos.z + size.z, u1, v0, colour);

        vert(pos.x,             pos.y + size.y, pos.z + size.z, u1, v0, colour);
        vert(pos.x,             pos.y,          pos.z,          u0, v1, colour);
        vert(pos.x,             pos.y + size.y, pos.z,          u1, v1, colour);

        // right
        vert(pos.x + size.x,    pos.y + size.y, pos.z + size.z, u0, v0, colour);
        vert(pos.x + size.x,    pos.y + size.y, pos.z,          u0, v1, colour);
        vert(pos.x + size.x,    pos.y,          pos.z + size.z, u1, v0, colour);

        vert(pos.x + size.x,    pos.y,          pos.z + size.z, u1, v0, colour);
        vert(pos.x + size.x,    pos.y + size.y, pos.z,          u0, v1, colour);
        vert(pos.x + size.x,    pos.y,          pos.z,          u1, v1, colour);

        // top
        vert(pos.x,             pos.y,          pos.z + size.z, u0, v0, colour);
        vert(pos.x,             pos.y + size.y, pos.z + size.z, u0, v1, colour);
        vert(pos.x + size.x,    pos.y,          pos.z + size.z, u1, v0, colour);

        vert(pos.x + size.x,    pos.y,          pos.z + size.z, u1, v0, colour);
        vert(pos.x,             pos.y + size.y, pos.z + size.z, u0, v1, colour);
        vert(pos.x + size.x,    pos.y + size.y, pos.z + size.z, u1, v1, colour);

        // bottom
        vert(pos.x,             pos.y + size.y, pos.z,         u0, v0, colour);
        vert(pos.x,             pos.y,          pos.z,         u0, v1, colour);
        vert(pos.x + size.x,    pos.y + size.y, pos.z,         u1, v0, colour);

        vert(pos.x + size.x,    pos.y + size.y, pos.z,         u1, v0, colour);
        vert(pos.x,             pos.y,          pos.z,         u0, v1, colour);
        vert(pos.x + size.x,    pos.y,          pos.z,         u1, v1, colour);
    }

    template <typename PosType, typename UVType, typename ColourType>
    void TexturedPainter3D<PosType, UVType, ColourType>::DrawFilledPlane(ITexture* texture, const PosType& pos, const PosType& size, const ColourType& colour)
    {
        static const auto u0 = TypeReflection<decltype(std::declval<UVType>().x)>::zero;
        static const auto v0 = TypeReflection<decltype(std::declval<UVType>().y)>::zero;
        static const auto u1 = TypeReflection<decltype(std::declval<UVType>().x)>::one;
        static const auto v1 = TypeReflection<decltype(std::declval<UVType>().y)>::one;

        VertexType* p_vertices = this->rm->template VertexCacheAllocAs<VertexType>(this->vertexFormat.get(),
                                                                                   this->material.get(), this->SetTexture(texture), RK_TRIANGLES, 2 * 6);

        // top
        vert(pos.x,             pos.y,          pos.z, u0, v0, colour);
        vert(pos.x,             pos.y + size.y, pos.z, u0, v1, colour);
        vert(pos.x + size.x,    pos.y,          pos.z, u1, v0, colour);

        vert(pos.x + size.x,    pos.y,          pos.z, u1, v0, colour);
        vert(pos.x,             pos.y + size.y, pos.z, u0, v1, colour);
        vert(pos.x + size.x,    pos.y + size.y, pos.z, u1, v1, colour);
    }

    template <typename PosType, typename UVType, typename ColourType>
    void TexturedPainter3D<PosType, UVType, ColourType>::DrawFilledTriangleUV(ITexture* texture, const PosType abc[3], const UVType uv[3], const ColourType colours[3])
    {
        VertexType* p_vertices = this->rm->template VertexCacheAllocAs<VertexType>(this->vertexFormat.get(),
                this->material.get(), this->SetTexture(texture), RK_TRIANGLES, 3);

        vert(abc[0].x,  abc[0].y,   abc[0].z,   uv[0].x,    uv[0].y,    colours[0]);
        vert(abc[1].x,  abc[1].y,   abc[1].z,   uv[1].x,    uv[1].y,    colours[1]);
        vert(abc[2].x,  abc[2].y,   abc[2].z,   uv[2].x,    uv[2].y,    colours[2]);
    }

#undef vert
}
