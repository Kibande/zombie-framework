#pragma once

#include <StudioKit/worldgeom.hpp>

#include <framework/datamodel.hpp>
#include <framework/modulehandler.hpp>

#include <string>
#include <vector>

namespace worldcraft
{
    class IBlenderImporter
    {
        public:
            using Vertex_t = StudioKit::WorldVertex_t;

            struct Material_t
            {
                std::string texture;
            };

            struct MaterialGroup_t
            {
                size_t sceneMaterialIndex;
                std::vector<Vertex_t> vertices;
            };

            struct Mesh_t
            {
                std::string name;
                std::vector<MaterialGroup_t> materialGroups;
            };

            struct PointLight_t
            {
                std::string name;
                zfw::Float3 pos;
                zfw::Float3 color;
                float dist;
                float attn_linear, attn_quad;
            };

            struct Scene_t
            {
                std::vector<Material_t> materials;
                std::vector<Mesh_t> meshes;
                std::vector<PointLight_t> pointLights;
            };

            virtual ~IBlenderImporter() {}

            virtual void Init(zfw::IEngine* sys, float globalScale) = 0;
            virtual bool ImportScene(const char* fileName, Scene_t* scene_out) = 0;
    };

    IBlenderImporter* CreateBlenderImporter();
}
