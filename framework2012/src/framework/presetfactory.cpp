
#include "framework.hpp"

namespace zfw
{
    namespace media
    {
        inline unsigned addVertexPos( DIMesh* mesh, const Float3& pos )
        {
            mesh->coords[mesh->numVertices*3] = pos.x;
            mesh->coords[mesh->numVertices*3+1] = pos.y;
            mesh->coords[mesh->numVertices*3+2] = pos.z;
            return mesh->numVertices++;
        }

        inline void addVertexNormal( DIMesh* mesh, const Float3& normal )
        {
            mesh->normals[mesh->numVertices*3] = normal.x;
            mesh->normals[mesh->numVertices*3+1] = normal.y;
            mesh->normals[mesh->numVertices*3+2] = normal.z;
        }

        inline void addVertexUV( DIMesh* mesh, const Float2& uv )
        {
            mesh->uvs[mesh->numVertices*2] = uv.x;
            mesh->uvs[mesh->numVertices*2+1] = uv.y;
        }

    static void setUpCuboidSide( const Float3 corners[8], unsigned side, const Float3& normal, unsigned cornerA, unsigned cornerB, unsigned cornerC, unsigned cornerD,
            int flags, DIMesh* mesh )
    {
        Float3 pos[4], normals[4];
        Float2 uvs[4];

        pos[0] = corners[cornerA];
        pos[1] = corners[cornerB];
        pos[2] = corners[cornerC];
        pos[3] = corners[cornerD];

        if ( flags & WITH_NORMALS )
            for ( int i = 0; i < 4; i++ )
                normals[i] = ( !(flags & REVERSE_WINDING) ) ? normal : -normal;

        if ( flags & WITH_UVS )
        {
            uvs[0] = Float2( 0.0f, 0.0f );
            uvs[1] = Float2( 0.0f, 1.0f );
            uvs[2] = Float2( 1.0f, 1.0f );
            uvs[3] = Float2( 1.0f, 0.0f );
        }

        /*for ( unsigned i = 0; i < cuboid.vertexProperties.numTextures; i++ )
            vertices[0].uv[i] = cuboid.uv[side][0][i];

        for ( unsigned i = 0; i < cuboid.vertexProperties.numTextures; i++ )
            vertices[1].uv[i] = Vector2<>( cuboid.uv[side][0][i].x, cuboid.uv[side][1][i].y );

        for ( unsigned i = 0; i < cuboid.vertexProperties.numTextures; i++ )
            vertices[2].uv[i] = cuboid.uv[side][1][i];

        for ( unsigned i = 0; i < cuboid.vertexProperties.numTextures; i++ )
            vertices[3].uv[i] = Vector2<>( cuboid.uv[side][1][i].x, cuboid.uv[side][0][i].y );

        if ( cuboid.vertexProperties.hasLightUvs )
        {
            vertices[0].lightUv = cuboid.lightUv[side][0];
            vertices[1].lightUv = Vector2<>( cuboid.lightUv[side][0].x, cuboid.lightUv[side][1].y );
            vertices[2].lightUv = cuboid.lightUv[side][1];
            vertices[3].lightUv = Vector2<>( cuboid.lightUv[side][1].x, cuboid.lightUv[side][0].y );
        }*/

        unsigned index[4];

        if ( !(flags & REVERSE_WINDING) )
        {
            for ( int i = 0; i < 4; i++ )
            {
                if ( flags & WITH_NORMALS )
                    addVertexNormal( mesh, normals[i] );

                if ( flags & WITH_UVS )
                    addVertexUV( mesh, uvs[i] );

                index[i] = addVertexPos( mesh, pos[i] );
            }
        }
        else
        {
            for ( int i = 0; i < 4; i++ )
            {
                if ( flags & WITH_NORMALS )
                    addVertexNormal( mesh, normals[3-i] );

                if ( flags & WITH_UVS )
                    addVertexUV( mesh, uvs[3-i] );

                index[i] = addVertexPos( mesh, pos[3-i] );
            }
        }

        mesh->indices[mesh->numIndices++]=index[0];
        mesh->indices[mesh->numIndices++]=index[1];
        mesh->indices[mesh->numIndices++]=index[3];
        mesh->indices[mesh->numIndices++]=index[3];
        mesh->indices[mesh->numIndices++]=index[1];
        mesh->indices[mesh->numIndices++]=index[2];
    }

        static void setUpPlane( const Float3 corners[4], const Float3& normal, int flags, DIMesh* mesh )
        {
            Float3 normals;

            if ( flags & WITH_NORMALS )
                normals = ( !(flags & REVERSE_WINDING) ) ? normal : -normal;

            if ( !(flags & REVERSE_WINDING) )
            {
                addVertexNormal( mesh, normals );
                addVertexPos( mesh, corners[0] );
                addVertexNormal( mesh, normals );
                addVertexPos( mesh, corners[1] );
                addVertexNormal( mesh, normals );
                addVertexPos( mesh, corners[3] );

                addVertexNormal( mesh, normals );
                addVertexPos( mesh, corners[3] );
                addVertexNormal( mesh, normals );
                addVertexPos( mesh, corners[1] );
                addVertexNormal( mesh, normals );
                addVertexPos( mesh, corners[2] );
            }
            else
            {
                addVertexNormal( mesh, normals );
                addVertexPos( mesh, corners[2] );
                addVertexNormal( mesh, normals );
                addVertexPos( mesh, corners[1] );
                addVertexNormal( mesh, normals );
                addVertexPos( mesh, corners[3] );

                addVertexNormal( mesh, normals );
                addVertexPos( mesh, corners[3] );
                addVertexNormal( mesh, normals );
                addVertexPos( mesh, corners[1] );
                addVertexNormal( mesh, normals );
                addVertexPos( mesh, corners[0] );
            }
        }

        void PresetFactory::CreateBitmap2x2( const glm::vec4& colour, TexFormat format, DIBitmap* bmp )
        {
            bmp->format = format;
            bmp->size = glm::ivec2(2, 2);

            switch ( format )
            {
                case TEX_RGB8:
                    for ( int i = 0; i < 4; i++ )
                    {
                        bmp->data[0] = bmp->data[3] = bmp->data[6] = bmp->data[9] = (uint8_t) (glm::clamp(colour.r, 0.0f, 1.0f) * 255.0f);
                        bmp->data[1] = bmp->data[4] = bmp->data[7] = bmp->data[10] = (uint8_t) (glm::clamp(colour.g, 0.0f, 1.0f) * 255.0f);
                        bmp->data[2] = bmp->data[5] = bmp->data[8] = bmp->data[11] = (uint8_t) (glm::clamp(colour.b, 0.0f, 1.0f) * 255.0f);
                    }
                    break;

                case TEX_RGBA8:
                    for ( int i = 0; i < 4; i++ )
                    {
                        bmp->data[0] = bmp->data[4] = bmp->data[8] = bmp->data[12] = (uint8_t) (glm::clamp(colour.r, 0.0f, 1.0f) * 255.0f);
                        bmp->data[1] = bmp->data[5] = bmp->data[9] = bmp->data[13] = (uint8_t) (glm::clamp(colour.g, 0.0f, 1.0f) * 255.0f);
                        bmp->data[2] = bmp->data[6] = bmp->data[10] = bmp->data[14] = (uint8_t) (glm::clamp(colour.b, 0.0f, 1.0f) * 255.0f);
                        bmp->data[3] = bmp->data[7] = bmp->data[11] = bmp->data[15] = (uint8_t) (glm::clamp(colour.a, 0.0f, 1.0f) * 255.0f);
                    }
                    break;
            }
        }

        void PresetFactory::CreateCube( const Float3& size, const Float3& origin, int flags, DIMesh* mesh )
        {
            Float3 corners[8];

            corners[0] =  - origin + Float3( 0.0f, 0.0f, 0.0f );
            corners[1] =  - origin + Float3( size.x, 0.0f, 0.0f );
            corners[2] =  - origin + Float3( size.x, size.y, 0.0f );
            corners[3] =  - origin + Float3( 0.0f, size.y, 0.0f );
            corners[4] =  - origin + Float3( 0.0f, 0.0f, size.z );
            corners[5] =  - origin + Float3( size.x, 0.0f, size.z );
            corners[6] =  - origin + Float3( size.x, size.y, size.z );
            corners[7] =  - origin + Float3( 0.0f, size.y, size.z );

            mesh->flags = render::R_3D;

            if (!(flags & WIREFRAME))
            {
                if ( flags & WITH_NORMALS )
                    mesh->flags |= render::R_NORMALS;

                if ( flags & WITH_UVS )
                    mesh->flags |= render::R_UVS;

                mesh->format = MPRIM_TRIANGLES;
                mesh->layout = MESH_INDEXED;
                mesh->numVertices = 0;
                mesh->numIndices = 0;

                if ( flags & FRONT )
                    setUpCuboidSide( corners, 0, Float3( 0.0f, 1.0f, 0.0f ), 7, 3, 2, 6, flags, mesh );

                if ( flags & BACK )
                    setUpCuboidSide( corners, 1, Float3( 0.0f, -1.0f, 0.0f ), 5, 1, 0, 4, flags, mesh );

                if ( flags & LEFT )
                    setUpCuboidSide( corners, 2, Float3( -1.0f, 0.0f, 0.0f ), 4, 0, 3, 7, flags, mesh );

                if ( flags & RIGHT )
                    setUpCuboidSide( corners, 3, Float3( 1.0f, 0.0f, 0.0f ), 6, 2, 1, 5, flags, mesh );

                if ( flags & TOP )
                    setUpCuboidSide( corners, 4, Float3( 0.0f, 0.0f, 1.0f ), 4, 7, 6, 5, flags, mesh );

                if ( flags & BOTTOM )
                    setUpCuboidSide( corners, 5, Float3( 0.0f, 0.0f, -1.0f ), 3, 0, 1, 2, flags, mesh );
            }
            else
            {
                static const VertexIndex_t indices[24] = {
                    0, 1, 1, 2, 2, 3, 3, 0,     // bottom
                    0, 4, 1, 5, 2, 6, 3, 7,     // middle
                    4, 5, 5, 6, 6, 7, 7, 4      // top
                };

                mesh->format = MPRIM_LINES;
                mesh->layout = MESH_INDEXED;
                mesh->numVertices = 8;
                mesh->numIndices = 24;

                mesh->coords.load((float *) corners, 3 * 8);
                mesh->indices.load(indices, 24);
            }
        }

        void PresetFactory::CreatePlane( const Float2& size, const Float3& origin, int flags, DIMesh* mesh )
        {
            Float3 corners[4];

            corners[0] =  - origin + Float3( 0.0f, 0.0f, 0.0f );
            corners[1] =  - origin + Float3( 0.0f, size.y, 0.0f );
            corners[2] =  - origin + Float3( size.x, size.y, 0.0f );
            corners[3] =  - origin + Float3( size.x, 0.0f, 0.0f );

            mesh->flags = render::R_3D;
            mesh->format = MPRIM_TRIANGLES;
            mesh->layout = MESH_LINEAR;
            mesh->numVertices = 0;

            setUpPlane( corners, Float3(0.0f, 0.0f, 1.0f), flags, mesh );
        }
    }
}
