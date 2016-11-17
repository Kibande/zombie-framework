
#include <StudioKit/blenderimporter.hpp>

#include <framework/errorcheck.hpp>
#include <framework/system.hpp>
#include <framework/utility/errorbuffer.hpp>
#include <framework/utility/essentials.hpp>

#include <littl/cfx2.hpp>
#include <littl/File.hpp>
#include <littl/FileName.hpp>

#include <array>
#include <functional>

namespace std
{
    template <typename Type, glm::precision P>
    Type* begin(glm::tvec3<Type, P>& value)
    {
        return &value[0];
    }

    template <typename Type, glm::precision P>
    Type* end(glm::tvec3<Type, P>& value)
    {
        return &value[0] + 3;
    }
}

namespace StudioKit
{
    using namespace li;

    using std::array;
    using std::string;
    using std::vector;
    using zfw::Float2;
    using zfw::Float3;

    static Float2 toFloat2(const char* x, const char* y)
    {
        return Float2(x ? (float)strtod(x, nullptr) : 0.0f, y ? (float)strtod(y, nullptr) : 0.0f);
    }

    static int toInt(const char* x)
    {
        return x ? strtol(x, nullptr, 0) : 0;
    }

    template <typename OutputIterator, typename UnaryOperation>
    void fromTokens(const char* str, char delim, OutputIterator first, OutputIterator last, UnaryOperation convert)
    {
        if (!str)
            return;

        for (; first < last; first++)
        {
            const char* end = strchr(str, delim);

            if (end != nullptr)
            {
                *first = convert(std::string(str, end - str));
                str = end + 1;
            }
            else
            {
                *first = convert(str);
                break;
            }
        }
    }

    template <size_t size>
    size_t vertexCount(const std::array<int, size>& indices)
    {
        for (size_t i = 0; i < size; i++)
            if (indices[i] == -1)
                return i;

        return size;
    }

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    namespace blender
    {
        struct Material
        {
            string active_texture_image_filepath;
            string active_texture_type;
        };

        struct MeshTessFace
        {
            int material_index;
            array<int, 7> vertices;
        };

        struct MeshTextureFace
        {
            string image_filepath;
            array<Float2, 4> uv;
        };

        struct MeshVertex
        {
            Float3 co;
            Float3 normal;
        };

        struct Mesh
        {
            vector<Material> materials;
            vector<MeshTextureFace> tessface_uv_textures;
            vector<MeshTessFace> tessfaces;
            vector<MeshVertex> vertices;
        };
    }

    struct Triangle_t
    {
        Float3 pos[3];
        Float3 normal[3];
        Float2 uv[3];

        size_t materialId;
    };

    class BlenderImporter : public IBlenderImporter
    {
        public:
            virtual void Init(zfw::ISystem* sys, float globalScale) override
            {
                this->sys = sys;
                this->globalScale = globalScale;
            }

            virtual bool ImportScene(const char* fileName, Scene_t* scene_out) override;

        private:
            size_t GetSceneMaterialIndex();
            size_t GetSceneMaterialIndex(const blender::Material& mat);
            size_t GetSceneMaterialIndex(blender::MeshTextureFace& meshTextureFace);
            MaterialGroup_t& GetMaterialGroupForMesh(Mesh_t& mesh, size_t sceneMaterialIndex);
            
            bool LampObject(cfx2::Node object);
            bool MeshObject(cfx2::Node object);
            bool Object(cfx2::Node object);

            zfw::ISystem* sys;
            float globalScale;

            string fileName;
            IBlenderImporter::Scene_t* scene;
    };

    // ====================================================================== //
    //  class BlenderImporter
    // ====================================================================== //

    IBlenderImporter* CreateBlenderImporter()
    {
        return new BlenderImporter();
    }

    IBlenderImporter::MaterialGroup_t& BlenderImporter::GetMaterialGroupForMesh(Mesh_t& mesh, size_t sceneMaterialIndex)
    {
        for (auto& materialGroup : mesh.materialGroups)
        {
            if (materialGroup.sceneMaterialIndex == sceneMaterialIndex)
                return materialGroup;
        }

        mesh.materialGroups.emplace_back();
        auto& materialGroup = mesh.materialGroups.back();

        materialGroup.sceneMaterialIndex = sceneMaterialIndex;

        return materialGroup;
    }

    size_t BlenderImporter::GetSceneMaterialIndex()
    {
        size_t index = 0;

        for (auto& material : scene->materials)
        {
            if (material.texture.empty())
                return index;

            index++;
        }

        scene->materials.emplace_back();
        auto& material = scene->materials.back();

        return index;
    }

    size_t BlenderImporter::GetSceneMaterialIndex(const blender::Material& mat)
    {
        size_t index = 0;

        for (auto& material : scene->materials)
        {
            if (mat.active_texture_type == "IMAGE")
            {
                if (material.texture == mat.active_texture_image_filepath)
                    return index;
            }
            else if (mat.active_texture_type == "NONE")
            {
                if (material.texture.empty())
                    return index;
            }

            index++;
        }

        scene->materials.emplace_back();
        auto& material = scene->materials.back();

        material.texture = mat.active_texture_image_filepath;

        return index;
    }

    size_t BlenderImporter::GetSceneMaterialIndex(blender::MeshTextureFace& meshTextureFace)
    {
        size_t index = 0;

        for (auto& material : scene->materials)
        {
            if (material.texture == meshTextureFace.image_filepath)
                return index;

            index++;
        }

        scene->materials.emplace_back();
        auto& material = scene->materials.back();

        material.texture = meshTextureFace.image_filepath;

        return index;
    }

    bool BlenderImporter::ImportScene(const char* fileName, IBlenderImporter::Scene_t* scene_out)
    {
        this->fileName = fileName;
        this->scene = scene_out;

        File input(fileName);

        if (!input)
            return zfw::ErrorBuffer::SetError3(zfw::EX_ASSET_OPEN_ERR, 1,
                    "desc", sprintf_255("Failed to open input file %s.", fileName)
                    ), false;

        cfx2::Document doc;
        auto x = sys->GetGlobalMicros();
        zombie_assert(doc.loadFrom(&input));
        auto y = sys->GetGlobalMicros();
        input.close();

        sys->Printf(zfw::kLogAlways, "Loading '%s': %u ms", fileName, (unsigned int)(y - x) / 1000);

        auto scene = doc.findChild("scene");
        zombie_assert(scene);

        auto objects = scene.findChild("objects");
        zombie_assert(objects);

        for (auto i : objects)
        {
            ErrorPassthru(Object(i));
        }

        return true;
    }

    bool BlenderImporter::LampObject(cfx2::Node object)
    {
        sys->Printf(zfw::kLogInfo, "Processing lamp object `%s`", object.getText());

        cfx2::Node data = object.findChild("data");
        zombie_assert(data);

        if (strcmp(data.queryValue("type"), "POINT") != 0)
        {
            sys->Printf(zfw::kLogWarning, "Skipping unknown lamp type: '%s'", data.queryValue("type"));
            return true;
        }

        scene->pointLights.emplace_back();
        auto& pointLight = scene->pointLights.back();

        pointLight.name = object.getText();

        fromTokens(object.queryValue("location"), ' ', std::begin(pointLight.pos), std::end(pointLight.pos),
                [&](const string& s) { return std::stof(s); });

        pointLight.dist = std::stof(data.queryValue("distance"));

        fromTokens(data.queryValue("color"), ' ', std::begin(pointLight.color), std::end(pointLight.color),
                [&](const string& s) { return std::stof(s); });

        pointLight.dist *= globalScale;
        pointLight.pos.y = -pointLight.pos.y;
        pointLight.pos *= globalScale;

        return true;
    }

    bool BlenderImporter::MeshObject(cfx2::Node object)
    {
        sys->Printf(zfw::kLogInfo, "Processing mesh object `%s`", object.getText());

        cfx2::Node mesh = object.findChild("mesh");
        zombie_assert(mesh);

        cfx2::Node materials = mesh.findChild("materials");
        cfx2::Node tessface_uv_textures = mesh.findChild("tessface_uv_textures");
        cfx2::Node tessfaces = mesh.findChild("tessfaces");
        cfx2::Node vertices = mesh.findChild("vertices");

        blender::Mesh bmesh;

        if (tessface_uv_textures)
            zombie_assert(tessface_uv_textures.getNumChildren() <= 1);

        for (auto i : materials)
        {
            bmesh.materials.emplace_back();
            auto& material = bmesh.materials.back();

            auto active_texture = i.findChild("active_texture");

            if (active_texture)
            {
                const char* filepath = active_texture.queryValue("image/filepath");
                if (filepath)
                    material.active_texture_image_filepath = li::FileName(filepath).getFileName();

                material.active_texture_type = active_texture.queryValue("type", "NONE");
            }
        }

        for (auto meshTextureFaceLayer : tessface_uv_textures)
        {
            auto data = meshTextureFaceLayer.findChild("data");

            for (auto i : data)
            {
                bmesh.tessface_uv_textures.emplace_back();
                auto& meshTextureFace = bmesh.tessface_uv_textures.back();

                const char* filepath = i.queryValue("image/filepath");
                if (filepath)
                    meshTextureFace.image_filepath = li::FileName(filepath).getFileName();

                cfx2::Node uv = i.findChild("uv");

                int n = 0;
                for (auto j : uv)
                    meshTextureFace.uv[n++] = toFloat2(j[0].getText(), j[1].getText());
            }
        }

        for (auto i : tessfaces)
        {
            bmesh.tessfaces.emplace_back();
            auto& meshTessFace = bmesh.tessfaces.back();

            meshTessFace.material_index = toInt(i.queryValue("material_index"));
            meshTessFace.vertices.fill(-1);

            fromTokens(i.queryValue("vertices"), ' ', meshTessFace.vertices.begin(), meshTessFace.vertices.end(),
                    [&](const string& s) { return std::stoi(s); });
        }

        for (auto i : vertices)
        {
            bmesh.vertices.emplace_back();
            auto& meshVertex = bmesh.vertices.back();

            fromTokens(i.queryValue("co"), ' ', std::begin(meshVertex.co), std::end(meshVertex.co),
                    [&](const string& s) { return std::stof(s); });
            fromTokens(i.queryValue("normal"), ' ', std::begin(meshVertex.normal), std::end(meshVertex.normal),
                    [&](const string& s) { return std::stof(s); });

            meshVertex.co.y = -meshVertex.co.y;
            meshVertex.co *= globalScale;
        }

        // Build the output Mesh_t

        scene->meshes.emplace_back();
        auto& smesh = scene->meshes.back();

        size_t index = 0;
        for (auto& meshTessFace : bmesh.tessfaces)
        {
            //printf("meshTessFace %u<%u\n", index, bmesh.tessface_uv_textures.size());
            bool hasUvTexture = (index < bmesh.tessface_uv_textures.size());

            size_t sceneMaterialIndex;

            if (hasUvTexture)
            {
                auto& meshTextureFace = bmesh.tessface_uv_textures[index];

                if (!meshTextureFace.image_filepath.empty())
                    sceneMaterialIndex = GetSceneMaterialIndex(meshTextureFace);
                else if (meshTessFace.material_index < bmesh.materials.size())
                    sceneMaterialIndex = GetSceneMaterialIndex(bmesh.materials[meshTessFace.material_index]);
                else
                    sceneMaterialIndex = GetSceneMaterialIndex();
            }
            else if (meshTessFace.material_index < bmesh.materials.size())
                sceneMaterialIndex = GetSceneMaterialIndex(bmesh.materials[meshTessFace.material_index]);
            else
                sceneMaterialIndex = GetSceneMaterialIndex();

            auto& materialGroup = GetMaterialGroupForMesh(smesh, sceneMaterialIndex);

            if (vertexCount(meshTessFace.vertices) == 3)
            {
                auto& v0_in = bmesh.vertices[meshTessFace.vertices[0]];
                auto& v1_in = bmesh.vertices[meshTessFace.vertices[1]];
                auto& v2_in = bmesh.vertices[meshTessFace.vertices[2]];

                materialGroup.vertices.resize(materialGroup.vertices.size() + 3);
                auto& v0 = materialGroup.vertices[materialGroup.vertices.size() - 3];
                auto& v1 = materialGroup.vertices[materialGroup.vertices.size() - 2];
                auto& v2 = materialGroup.vertices[materialGroup.vertices.size() - 1];

                v0.pos = v0_in.co;
                v1.pos = v1_in.co;
                v2.pos = v2_in.co;

                Float3 normal = glm::normalize(glm::cross(v2_in.co - v0_in.co, v1_in.co - v0_in.co));

                v0.normal = normal;
                v1.normal = normal;
                v2.normal = normal;

                if (hasUvTexture)
                {
                    auto& meshTextureFace = bmesh.tessface_uv_textures[index];

                    v0.uv = meshTextureFace.uv[0];
                    v1.uv = meshTextureFace.uv[1];
                    v2.uv = meshTextureFace.uv[2];
                }
            }
            else if (vertexCount(meshTessFace.vertices) == 4)
            {
                auto& v0_in = bmesh.vertices[meshTessFace.vertices[0]];
                auto& v1_in = bmesh.vertices[meshTessFace.vertices[1]];
                auto& v2_in = bmesh.vertices[meshTessFace.vertices[2]];
                auto& v3_in = bmesh.vertices[meshTessFace.vertices[3]];

                materialGroup.vertices.resize(materialGroup.vertices.size() + 6);
                auto& v0 = materialGroup.vertices[materialGroup.vertices.size() - 6];
                auto& v1 = materialGroup.vertices[materialGroup.vertices.size() - 5];
                auto& v2 = materialGroup.vertices[materialGroup.vertices.size() - 4];
                auto& v3 = materialGroup.vertices[materialGroup.vertices.size() - 3];
                auto& v4 = materialGroup.vertices[materialGroup.vertices.size() - 2];
                auto& v5 = materialGroup.vertices[materialGroup.vertices.size() - 1];

                v0.pos = v0_in.co;
                v1.pos = v1_in.co;
                v2.pos = v2_in.co;
                v3.pos = v0_in.co;
                v4.pos = v2_in.co;
                v5.pos = v3_in.co;

                Float3 normal = glm::normalize(glm::cross(v2_in.co - v0_in.co, v1_in.co - v0_in.co));

                v0.normal = normal;
                v1.normal = normal;
                v2.normal = normal;
                v3.normal = normal;
                v4.normal = normal;
                v5.normal = normal;

                if (hasUvTexture)
                {
                    auto& meshTextureFace = bmesh.tessface_uv_textures[index];

                    v0.uv = meshTextureFace.uv[0];
                    v1.uv = meshTextureFace.uv[1];
                    v2.uv = meshTextureFace.uv[2];
                    v3.uv = meshTextureFace.uv[0];
                    v4.uv = meshTextureFace.uv[2];
                    v5.uv = meshTextureFace.uv[3];
                }
            }
            else
            {
                zombie_assert(false);
            }

            index++;
        }

        return true;
    }

    bool BlenderImporter::Object(cfx2::Node object)
    {
        //auto mesh = object.findChild("mesh");
        auto type = object.findChild("type").getText();

        if (!type)
            printf("Hidden object %s\n", object.getText());
        else if (strcmp(type, "EMPTY") == 0)
            printf("NOT IMPLEMENTED: Proxy object %s\n", object.getText());
        else if (strcmp(type, "LAMP") == 0)
        {
            ErrorPassthru(LampObject(object));
        }
        else if (strcmp(type, "MESH") == 0)
        {
            ErrorPassthru(MeshObject(object));
        }
        else
            printf("Unknown type of object %s (claims %s)\n", object.getText(), type);

        return true;
    }
}
