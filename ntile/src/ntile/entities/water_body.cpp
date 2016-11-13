
#include "entities.hpp"
#include "../ntile.hpp"

#include <framework/colorconstants.hpp>
#include <framework/resourcemanager2.hpp>

//#include <glm/gtx/rotate_vector.hpp>

namespace ntile
{
namespace entities
{
    static const Normal_t normal_up[4] = { 0, 0, INT16_MAX, 32767 };

    static const int step_max = 128;
    static const int min = -4 * 256;
    static const int max = 4 * 256;

    static bool waveExists = true;
    static float wavePhase = 0.0f;
    static float waveLength = 60.0f;
    static float waveAmplitude = 15.0f * 256.0f;

    static float waveDistToHalf = 4 * f_pi;
    static Float2 waveDirection(0.707f, 0.707f);

    water_body::water_body() : tiles(32, 32)
    {
    }

    bool water_body::Init()
    {
        numVertices = tiles.x * tiles.y * 6;

        randomHeightmap.resize((tiles.x + 1) * (tiles.y + 1));
        waveHeightmap.resize((tiles.x + 1) * (tiles.y + 1));
        RandomizeHeightmap();

        vertexBuf.reset(ir->CreateVertexBuffer());
        vertexBuf->Alloc(numVertices * sizeof(WorldVertex));
        ResetGeometry();

        return true;
    }

    water_body::~water_body()
    {
    }

    void water_body::Draw(const UUID_t* uuidOrNull)
    {
        ir->DrawPrimitives(vertexBuf.get(), PRIMITIVE_TRIANGLES, g_worldVertexFormat.get(), 0, numVertices);
    }

    void water_body::InitializeVertices(WorldVertex* p_vertices)
    {
        // size of one subdivision
        static const float tile_x = 16.0f;
        static const float tile_y = 16.0f;

        //static const Byte4 rgba = rgbCSS(0x1166CC);         // pretty, but too saturated
        static const Byte4 rgba = rgbCSS(0x3366CC);

        // TODO: type
        float xx = TILE_SIZE_H / 2 + pos.x;
        float yy = TILE_SIZE_V / 2 + pos.y;
        float zz = pos.z;
        float u = 0.5f;
        float v = 0.5f;

        for (int y = 0; y < tiles.y; y++)
        {
            for (int x = 0; x < tiles.x; x++)
            {
                p_vertices[0].x = xx + x * tile_x;
                p_vertices[0].y = yy + y * tile_y;
                p_vertices[0].z = zz;
                memcpy(&p_vertices[0].n[0], normal_up, sizeof(normal_up));
                p_vertices[0].u = u;
                p_vertices[0].v = v;
                memcpy(&p_vertices[0].rgba[0], &rgba[0], sizeof(rgba));

                p_vertices[1].x = xx + x * tile_x;
                p_vertices[1].y = yy + (y + 1) * tile_y;
                p_vertices[1].z = zz;
                memcpy(&p_vertices[1].n[0], normal_up, sizeof(normal_up));
                p_vertices[1].u = u;
                p_vertices[1].v = 1.0f - v;
                memcpy(&p_vertices[1].rgba[0], &rgba[0], sizeof(rgba));

                p_vertices[2].x = xx + (x + 1) * tile_x;
                p_vertices[2].y = yy + (y + 1) * tile_y;
                p_vertices[2].z = zz;
                memcpy(&p_vertices[2].n[0], normal_up, sizeof(normal_up));
                p_vertices[2].u = 1.0f - u;
                p_vertices[2].v = 1.0f - v;
                memcpy(&p_vertices[2].rgba[0], &rgba[0], sizeof(rgba));

                p_vertices[3].x = xx + x * tile_x;
                p_vertices[3].y = yy + y * tile_y;
                p_vertices[3].z = zz;
                memcpy(&p_vertices[3].n[0], normal_up, sizeof(normal_up));
                p_vertices[3].u = u;
                p_vertices[3].v = v;
                memcpy(&p_vertices[3].rgba[0], &rgba[0], sizeof(rgba));

                p_vertices[4].x = xx + (x + 1) * tile_x;
                p_vertices[4].y = yy + (y + 1) * tile_y;
                p_vertices[4].z = zz;
                memcpy(&p_vertices[4].n[0], normal_up, sizeof(normal_up));
                p_vertices[4].u = 1.0f - u;
                p_vertices[4].v = 1.0f - v;
                memcpy(&p_vertices[4].rgba[0], &rgba[0], sizeof(rgba));

                p_vertices[5].x = xx + (x + 1) * tile_x;
                p_vertices[5].y = yy + y * tile_y;
                p_vertices[5].z = zz;
                memcpy(&p_vertices[5].n[0], normal_up, sizeof(normal_up));
                p_vertices[5].u = 1.0f - u;
                p_vertices[5].v = v;
                memcpy(&p_vertices[5].rgba[0], &rgba[0], sizeof(rgba));

                p_vertices += 6;
                u = 1.0f - u;
            }

            v = 1.0f - v;
        }
    }

    void water_body::OnTick()
    {
        UpdateHeightmap();
        UpdateGeometry();       // FIXME FIXME FIXME this mustn't be called all over
    }

    void water_body::RandomizeHeightmap()
    {
        int16_t* p_heightmap = &randomHeightmap[0];

        for (int i = 0; i < (tiles.x + 1) * (tiles.y + 1); i++) {
            int16_t& value = *p_heightmap++;
            value = min + (rand() % (max - min));
        }
    }

    void water_body::ResetGeometry()
    {
        auto vertices = static_cast<WorldVertex*>(vertexBuf->Map(false, true));

        InitializeVertices(vertices);

        vertexBuf->Unmap();

        UpdateGeometry();// GOD PLEASE FIXME
    }

    void water_body::UpdateGeometry()
    {
        auto p_vertices = static_cast<WorldVertex*>(vertexBuf->Map(true, true));

        unsigned xx = 0;
        const float zz = pos.z;

        for (unsigned y = 0; y < tiles.y; y++)
        {
            for (unsigned x = 0; x < tiles.x; x++)
            {
                int heightLT = randomHeightmap[y * (tiles.x + 1) + x] + waveHeightmap[y * (tiles.x + 1) + x];
                int heightLB = randomHeightmap[(y + 1) * (tiles.x + 1) + x] + waveHeightmap[(y + 1) * (tiles.x + 1) + x];
                int heightRB = randomHeightmap[(y + 1) * (tiles.x + 1) + (x + 1)] + waveHeightmap[(y + 1) * (tiles.x + 1) + (x + 1)];
                int heightRT = randomHeightmap[y * (tiles.x + 1) + (x + 1)] + waveHeightmap[y * (tiles.x + 1) + (x + 1)];

                p_vertices[0].z = zz + heightLT * (1 / 256.0f);
                p_vertices[1].z = zz + heightLB * (1 / 256.0f);
                p_vertices[2].z = zz + heightRB * (1 / 256.0f);

                Float3 a1 = Float3(*(Int3*)(&p_vertices[0].x));
                Float3 a2 = Float3(*(Int3*)(&p_vertices[1].x));
                Float3 a3 = Float3(*(Int3*)(&p_vertices[2].x));
                Short3& na1 = *(Short3*)(p_vertices[0].n);
                Short3& na2 = *(Short3*)(p_vertices[1].n);
                Short3& na3 = *(Short3*)(p_vertices[2].n);
                Short3 normal = Short3(glm::normalize(glm::cross(a1 - a2, a3 - a1)) * 32767.0f);
                na1 = normal;
                na2 = normal;
                na3 = normal;

                p_vertices[3].z = zz + heightLT * (1 / 256.0f);
                p_vertices[4].z = zz + heightRB * (1 / 256.0f);
                p_vertices[5].z = zz + heightRT * (1 / 256.0f);

                Float3 b1 = Float3(*(Int3*)(&p_vertices[3].x));
                Float3 b2 = Float3(*(Int3*)(&p_vertices[4].x));
                Float3 b3 = Float3(*(Int3*)(&p_vertices[5].x)) + Float3(0.0f, 0.0f, 2.0f);  // make stuff less uniform*/
                Short3& nb1 = *(Short3*)(p_vertices[3].n);
                Short3& nb2 = *(Short3*)(p_vertices[4].n);
                Short3& nb3 = *(Short3*)(p_vertices[5].n);
                Short3 normal2 = Short3(glm::normalize(glm::cross(b1 - b2, b3 - b1)) * 32767.0f);
                nb1 = normal2;
                nb2 = normal2;
                nb3 = normal2;

                p_vertices += 6;
            }
        }

        vertexBuf->Unmap();
    }

    void water_body::UpdateHeightmap()
    {
        for (int i = 0; i < tiles.x * tiles.y / 256; i++) {
            unsigned xx = rand() % (tiles.x + 1);
            unsigned yy = rand() % (tiles.y + 1);
            int16_t& value = randomHeightmap[yy * (tiles.x + 1) + xx];
            value = std::min(max, std::max(min, value + (rand() % (2 * step_max)) - step_max));
        }

        int16_t* p_heightmap = &waveHeightmap[0];

        if (waveExists)
        {
            for (unsigned y = 0; y < tiles.y + 1; y++)
            {
                for (unsigned x = 0; x < tiles.x + 1; x++)
                {
                    float t = glm::dot(Float2(x, y), waveDirection) * 16.0f;
                    float omega = 2 * f_pi / waveLength;
                    float phase = omega * t - wavePhase;
                    float a = cos(phase) / (1.0f + abs(phase) / waveDistToHalf);
                    *p_heightmap++ = waveAmplitude * a;
                }
            }
        }

        wavePhase += 0.05f;
        //waveDirection = Float2(glm::rotateZ(Float3(waveDirection, 0.0f), 0.05f));
    }
}
}
