#pragma once

#include "../RenderingKit.hpp"
#include "../TypeReflection.hpp"

#include <framework/errorcheck.hpp>
#include <framework/resourcemanager.hpp>

#include <cstddef>

namespace RenderingKit
{
    template <typename PosType, typename ColourType>
    struct BasicVertex_t
    {
        typedef BasicVertex_t<PosType, ColourType> ThisType;

        PosType     pos;
        ColourType  colour;

        static const VertexAttrib_t* GetVertexAttribs()
        {
            static const VertexAttrib_t vertexAttribs[] =
            {
                {"in_Position",     offsetof(ThisType, pos),    TypeReflection<PosType>::attribDataType,
                        RK_ATTRIB_NOT_NORMALIZED},
                {"in_Color",        offsetof(ThisType, colour), TypeReflection<ColourType>::attribDataType},
                {}
            };

            return vertexAttribs;
        }
    };

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    template <typename PosType, typename ColourType>
    class BasicPainter
    {
        public:
            typedef BasicVertex_t<PosType, ColourType> VertexType;

            BasicPainter();

            bool Init(IRenderingManager* rm);
            bool InitWithShader(IRenderingManager* rm, shared_ptr<IShader> program);
            void Shutdown();

        protected:
            IRenderingManager* rm;

            shared_ptr<IVertexFormat> vertexFormat;
            shared_ptr<IMaterial> material;

            void DropResources();
    };

    template <typename PosType = zfw::Short2, typename ColourType = zfw::Byte4>
    class BasicPainter2D : public BasicPainter<PosType, ColourType>
    {
        public:
            typedef typename BasicPainter<PosType, ColourType>::VertexType VertexType;

            void DrawFilledRectangle(const PosType& pos, const PosType& size, const ColourType& colour);
            void DrawFilledRectangle(const PosType& pos, const PosType& size, const ColourType colours[4]);
            void DrawFilledTriangle(const PosType abc[3], const ColourType colours[3]);
    };

    template <typename PosType = zfw::Float3, typename ColourType = zfw::Byte4>
    class BasicPainter3D : public BasicPainter<PosType, ColourType>
    {
        public:
            typedef typename BasicPainter<PosType, ColourType>::VertexType VertexType;

            void DrawCuboidWireframe(const PosType& pos, const PosType& size, const ColourType& colour);
            void DrawFilledCuboid(const PosType& pos, const PosType& size, const ColourType& colour);
            void DrawFilledRectangle(const PosType& pos, const PosType& size, const ColourType& colour);
            void DrawFilledTriangle(const PosType abc[3], const ColourType& colour);
            void DrawGrid(const PosType& pos, const PosType& cellSize, const Int2& numCells, const ColourType& colour);
            void DrawGridAround(const PosType& center, const PosType& cellSize, const Int2& numCells, const ColourType& colour);
    };

    // ====================================================================== //
    //  class BasicPainter
    // ====================================================================== //

    template <typename PosType, typename ColourType>
    BasicPainter<PosType, ColourType>::BasicPainter()
    {
        rm = nullptr;
    }

    template <typename PosType, typename ColourType>
    bool BasicPainter<PosType, ColourType>::Init(IRenderingManager* rm)
    {
        this->rm = rm;

        auto shader = rm->GetSharedResourceManager()->GetResource<IShader>("path=RenderingKit/basic", zfw::RESOURCE_REQUIRED, 0);
        zombie_ErrorCheck(shader);

        vertexFormat = rm->CompileVertexFormat(shader.get(), sizeof(VertexType), VertexType::GetVertexAttribs(), false);

        material = rm->CreateMaterial("BasicPainter/material", shader);

        return true;
    }

    template <typename PosType, typename ColourType>
    bool BasicPainter<PosType, ColourType>::InitWithShader(IRenderingManager* rm, shared_ptr<IShader> program)
    {
        this->rm = rm;

        vertexFormat = rm->CompileVertexFormat(program.get(), sizeof(VertexType), VertexType::GetVertexAttribs(), false);
        material = rm->CreateMaterial("BasicPainter/material", program);

        return true;
    }

    template <typename PosType, typename ColourType>
    void BasicPainter<PosType, ColourType>::Shutdown()
    {
        DropResources();
    }

    template <typename PosType, typename ColourType>
    void BasicPainter<PosType, ColourType>::DropResources()
    {
        material.reset();
        vertexFormat.reset();
    }

    // ====================================================================== //
    //  class BasicPainter2D
    // ====================================================================== //

#define vert(x_, y_, colour_) do {\
        p_vertices->pos.x = x_;\
        p_vertices->pos.y = y_;\
        p_vertices->colour = colour_;\
        p_vertices++;\
    } while (false)

    template <typename PosType, typename ColourType>
    void BasicPainter2D<PosType, ColourType>::DrawFilledRectangle(const PosType& pos, const PosType& size, const ColourType& colour)
    {
        VertexType* p_vertices = reinterpret_cast<VertexType*>(
                this->rm->VertexCacheAlloc(this->vertexFormat.get(), this->material.get(), RK_TRIANGLES, 6)
        );

        vert(pos.x,             pos.y,          colour);
        vert(pos.x,             pos.y + size.y, colour);
        vert(pos.x + size.x,    pos.y,          colour);
        vert(pos.x + size.x,    pos.y,          colour);
        vert(pos.x,             pos.y + size.y, colour);
        vert(pos.x + size.x,    pos.y + size.y, colour);
    }

    template <typename PosType, typename ColourType>
    void BasicPainter2D<PosType, ColourType>::DrawFilledRectangle(const PosType& pos, const PosType& size, const ColourType colours[4])
    {
        VertexType* p_vertices = reinterpret_cast<VertexType*>(
                this->rm->VertexCacheAlloc(this->vertexFormat.get(), this->material.get(), RK_TRIANGLES, 6)
        );

        vert(pos.x,             pos.y,          colours[0]);
        vert(pos.x,             pos.y + size.y, colours[2]);
        vert(pos.x + size.x,    pos.y,          colours[1]);
        vert(pos.x + size.x,    pos.y,          colours[1]);
        vert(pos.x,             pos.y + size.y, colours[2]);
        vert(pos.x + size.x,    pos.y + size.y, colours[3]);
    }

    template <typename PosType, typename ColourType>
    void BasicPainter2D<PosType, ColourType>::DrawFilledTriangle(const PosType abc[3], const ColourType colours[3])
    {
        VertexType* p_vertices = reinterpret_cast<VertexType*>(
                this->rm->VertexCacheAlloc(this->vertexFormat.get(), this->material.get(), RK_TRIANGLES, 3)
        );

        vert(abc[0].x,  abc[0].y,   colours[0]);
        vert(abc[1].x,  abc[1].y,   colours[2]);
        vert(abc[2].x,  abc[2].y,   colours[1]);
    }

#undef vert

    // ====================================================================== //
    //  class BasicPainter3D
    // ====================================================================== //

#define vert(x_, y_, z_, colour_) do {\
        p_vertices->pos.x = x_;\
        p_vertices->pos.y = y_;\
        p_vertices->pos.z = z_;\
        p_vertices->colour = colour_;\
        p_vertices++;\
    } while (false)

    template <typename PosType, typename ColourType>
    void BasicPainter3D<PosType, ColourType>::DrawCuboidWireframe(const PosType& pos, const PosType& size, const ColourType& colour)
    {
        VertexType* p_vertices = reinterpret_cast<VertexType*>(
                this->rm->VertexCacheAlloc(this->vertexFormat.get(), this->material.get(), RK_LINES, 24)
        );

        // front
        vert(pos.x,             pos.y + size.y, pos.z + size.z, colour);
        vert(pos.x,             pos.y + size.y, pos.z,          colour);

        vert(pos.x,             pos.y + size.y, pos.z,          colour);
        vert(pos.x + size.x,    pos.y + size.y, pos.z,          colour);

        vert(pos.x + size.x,    pos.y + size.y, pos.z,          colour);
        vert(pos.x + size.x,    pos.y + size.y, pos.z + size.z, colour);

        vert(pos.x + size.x,    pos.y + size.y, pos.z + size.z, colour);
        vert(pos.x,             pos.y + size.y, pos.z + size.z, colour);

        // back
        vert(pos.x,             pos.y,          pos.z + size.z, colour);
        vert(pos.x,             pos.y,          pos.z,          colour);

        vert(pos.x,             pos.y,          pos.z,          colour);
        vert(pos.x + size.x,    pos.y,          pos.z,          colour);

        vert(pos.x + size.x,    pos.y,          pos.z,          colour);
        vert(pos.x + size.x,    pos.y,          pos.z + size.z, colour);

        vert(pos.x + size.x,    pos.y,          pos.z + size.z, colour);
        vert(pos.x,             pos.y,          pos.z + size.z, colour);

        // middle
        vert(pos.x,             pos.y,          pos.z + size.z, colour);
        vert(pos.x,             pos.y + size.y, pos.z + size.z, colour);

        vert(pos.x,             pos.y,          pos.z,          colour);
        vert(pos.x,             pos.y + size.y, pos.z,          colour);

        vert(pos.x + size.x,    pos.y,          pos.z,          colour);
        vert(pos.x + size.x,    pos.y + size.y, pos.z,          colour);

        vert(pos.x + size.x,    pos.y,          pos.z + size.z, colour);
        vert(pos.x + size.x,    pos.y + size.y, pos.z + size.z, colour);
    }

    template <typename PosType, typename ColourType>
    void BasicPainter3D<PosType, ColourType>::DrawFilledCuboid(const PosType& pos, const PosType& size, const ColourType& colour)
    {
        VertexType* p_vertices = reinterpret_cast<VertexType*>(
                this->rm->VertexCacheAlloc(this->vertexFormat.get(), this->material.get(), RK_TRIANGLES, 6 * 6)
        );

        // front
        vert(pos.x,             pos.y + size.y, pos.z + size.z, colour);
        vert(pos.x,             pos.y + size.y, pos.z,          colour);
        vert(pos.x + size.x,    pos.y + size.y, pos.z + size.z, colour);

        vert(pos.x + size.x,    pos.y + size.y, pos.z + size.z, colour);
        vert(pos.x,             pos.y + size.y, pos.z,          colour);
        vert(pos.x + size.x,    pos.y + size.y, pos.z,          colour);

        // back
        vert(pos.x + size.x,    pos.y,          pos.z + size.z, colour);
        vert(pos.x + size.x,    pos.y,          pos.z,          colour);
        vert(pos.x,             pos.y,          pos.z + size.z, colour);

        vert(pos.x,             pos.y,          pos.z + size.z, colour);
        vert(pos.x + size.x,    pos.y,          pos.z,          colour);
        vert(pos.x,             pos.y,          pos.z,          colour);

        // left
        vert(pos.x,             pos.y,          pos.z + size.z, colour);
        vert(pos.x,             pos.y,          pos.z,          colour);
        vert(pos.x,             pos.y + size.y, pos.z + size.z, colour);

        vert(pos.x,             pos.y + size.y, pos.z + size.z, colour);
        vert(pos.x,             pos.y,          pos.z,          colour);
        vert(pos.x,             pos.y + size.y, pos.z,          colour);

        // right
        vert(pos.x + size.x,    pos.y + size.y, pos.z + size.z, colour);
        vert(pos.x + size.x,    pos.y + size.y, pos.z,          colour);
        vert(pos.x + size.x,    pos.y,          pos.z + size.z, colour);

        vert(pos.x + size.x,    pos.y,          pos.z + size.z, colour);
        vert(pos.x + size.x,    pos.y + size.y, pos.z,          colour);
        vert(pos.x + size.x,    pos.y,          pos.z,          colour);

        // top
        vert(pos.x,             pos.y,          pos.z + size.z, colour);
        vert(pos.x,             pos.y + size.y, pos.z + size.z, colour);
        vert(pos.x + size.x,    pos.y,          pos.z + size.z, colour);

        vert(pos.x + size.x,    pos.y,          pos.z + size.z, colour);
        vert(pos.x,             pos.y + size.y, pos.z + size.z, colour);
        vert(pos.x + size.x,    pos.y + size.y, pos.z + size.z, colour);

        // bottom
        vert(pos.x,             pos.y + size.y, pos.z,         colour);
        vert(pos.x,             pos.y,          pos.z,         colour);
        vert(pos.x + size.x,    pos.y + size.y, pos.z,         colour);

        vert(pos.x + size.x,    pos.y + size.y, pos.z,         colour);
        vert(pos.x,             pos.y,          pos.z,         colour);
        vert(pos.x + size.x,    pos.y,          pos.z,         colour);
    }

    template <typename PosType, typename ColourType>
    void BasicPainter3D<PosType, ColourType>::DrawFilledRectangle(const PosType& pos, const PosType& size, const ColourType& colour)
    {
        VertexType* p_vertices = reinterpret_cast<VertexType*>(
                this->rm->VertexCacheAlloc(this->vertexFormat.get(), this->material.get(), RK_TRIANGLES, 6)
        );

        vert(pos.x,             pos.y,          pos.z,  colour);
        vert(pos.x,             pos.y + size.y, pos.z,  colour);
        vert(pos.x + size.x,    pos.y,          pos.z,  colour);
        vert(pos.x + size.x,    pos.y,          pos.z,  colour);
        vert(pos.x,             pos.y + size.y, pos.z,  colour);
        vert(pos.x + size.x,    pos.y + size.y, pos.z,  colour);
    }

    template <typename PosType, typename ColourType>
    void BasicPainter3D<PosType, ColourType>::DrawFilledTriangle(const PosType abc[3], const ColourType& colour)
    {
        VertexType* p_vertices = reinterpret_cast<VertexType*>(
                this->rm->VertexCacheAlloc(this->vertexFormat.get(), this->material.get(), RK_TRIANGLES, 3)
        );

        vert(abc[0].x,  abc[0].y,   abc[0].z,   colour);
        vert(abc[1].x,  abc[1].y,   abc[1].z,   colour);
        vert(abc[2].x,  abc[2].y,   abc[2].z,   colour);
    }

    template <typename PosType, typename ColourType>
    void BasicPainter3D<PosType, ColourType>::DrawGrid(const PosType& pos, const PosType& cellSize, const Int2& numCells, const ColourType& colour)
    {
        if (numCells.x <= 0 || numCells.y <= 0)
            return;

        const size_t numVertices = (numCells.x + numCells.y + 2) * 2;

        const auto x0 = pos.x;
        const auto x1 = x0 + numCells.x * cellSize.x;

        const auto y0 = pos.y;
        const auto y1 = y0 + numCells.y * cellSize.y;

        const auto z = pos.z;

        VertexType* p_vertices = reinterpret_cast<VertexType*>(
                this->rm->VertexCacheAlloc(this->vertexFormat.get(), this->material.get(), RK_LINES, numVertices)
        );

        auto x = x0;

        for (int i = 0; i <= numCells.x; i++)
        {
            vert(x, y0, z, colour);
            vert(x, y1, z, colour);

            x += cellSize.x;
        }

        auto y = y0;

        for (int i = 0; i <= numCells.y; i++)
        {
            vert(x0, y, z, colour);
            vert(x1, y, z, colour);

            y += cellSize.y;
        }
    }

    template <typename PosType, typename ColourType>
    void BasicPainter3D<PosType, ColourType>::DrawGridAround(const PosType& center, const PosType& cellSize, const Int2& numCells, const ColourType& colour)
    {
        DrawGrid(PosType(center.x - numCells.x * cellSize.x / 2, center.y - numCells.y * cellSize.y / 2, center.z),
                cellSize,
                numCells,
                colour);
    }

#undef vert
}
