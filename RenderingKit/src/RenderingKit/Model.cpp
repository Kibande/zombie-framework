#include "RenderingKitImpl.hpp"

#include <framework/engine.hpp>
#include <framework/mediacodechandler.hpp>
#include <framework/pixmap.hpp>
#include <framework/resourcemanager2.hpp>
#include <framework/utility/pixmap.hpp>

#include <littl/Stream.hpp>

#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

#include <glm/gtc/quaternion.hpp>

namespace RenderingKit
{
    using namespace zfw;

    static GLenum ps_AccessorComponentTypeToGLType(int componentType) {
        switch (componentType) {
            case TINYGLTF_COMPONENT_TYPE_BYTE:              return GL_BYTE;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:     return GL_UNSIGNED_BYTE;
            case TINYGLTF_COMPONENT_TYPE_SHORT:             return GL_SHORT;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:    return GL_UNSIGNED_SHORT;
            case TINYGLTF_COMPONENT_TYPE_INT:               return GL_INT;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:      return GL_UNSIGNED_INT;
            case TINYGLTF_COMPONENT_TYPE_FLOAT:             return GL_FLOAT;
            case TINYGLTF_COMPONENT_TYPE_DOUBLE:            return GL_DOUBLE;

            default:
                zombie_assert(componentType != componentType);
                return 0;
        }
    }

    static GLint ps_GetNumComponentes(int type) {
        switch (type) {
            case TINYGLTF_TYPE_SCALAR:  return 1;
            case TINYGLTF_TYPE_VEC2:    return 2;
            case TINYGLTF_TYPE_VEC3:    return 3;
            case TINYGLTF_TYPE_VEC4:    return 4;

            default:
                zombie_assert(type != type);
                return 0;
        }
    }

    static const char* ps_MapAttributeName(const std::string& name) {
        if (name == "JOINTS_0") {
            return "in_Joints0";
        }
        else if (name == "NORMAL") {
            return "in_Normal";
        }
        else if (name == "POSITION") {
            return "in_Position";
        }
        else if (name == "WEIGHTS_0") {
            return "in_Weights0";
        }
        else {
            zombie_assert(name != name);
            return nullptr;
        }
    }

    static GLenum ps_PrimitiveModeToGLMode(int primitiveMode) {
        switch (primitiveMode) {
            case TINYGLTF_MODE_POINTS:          return GL_POINTS;
            case TINYGLTF_MODE_LINE:            return GL_LINES;
            case TINYGLTF_MODE_LINE_LOOP:       return GL_LINE_LOOP;
            case TINYGLTF_MODE_TRIANGLES:       return GL_TRIANGLES;
            case TINYGLTF_MODE_TRIANGLE_STRIP:  return GL_TRIANGLE_STRIP;
            case TINYGLTF_MODE_TRIANGLE_FAN:    return GL_TRIANGLE_FAN;

            default:
                zombie_assert(primitiveMode != primitiveMode);
                return 0;
        }
    }

    // ====================================================================== //
    //  class declaration(s)
    // ====================================================================== //

    class ImageLoader {
    public:
        IEngine* engine;

        static bool LoadImageData(tinygltf::Image* image, std::string* err, std::string* warn,
                                  int req_width, int req_height, const unsigned char *bytes,
                                  int size, void* userdata);
    };

    class GLModel : public IGLModel {
        public:
            GLModel(RenderingKit* rk, const char* path) : rk(rk), path(path) {}

            // IModel
            void Draw(const glm::mat4x4& transform) final;

            // IResource2
            bool BindDependencies(IResourceManager2* resMgr) { return true; }
            bool Preload(IResourceManager2* resMgr);
            void Unload() {}
            bool Realize(IResourceManager2* resMgr);
            void Unrealize() {}

            virtual void* Cast(const TypeID& resourceClass) final override { return DefaultCast(this, resourceClass); }

            virtual State_t GetState() const final override { return state; }

            virtual bool StateTransitionTo(State_t targetState, IResourceManager2* resMgr) final override
            {
                return DefaultStateTransitionTo(this, targetState, resMgr);
            }

        private:
            // Private methods
            void p_DrawNode(tinygltf::Node& node, const glm::mat4x4& projection, const glm::mat4x4& modelView);

            RenderingKit* rk;
            std::string path;

            State_t state = CREATED;

            // preloaded
            unique_ptr<tinygltf::Model> model;

            // realized
            GLuint geometryBuffer;
            std::vector<IGLMaterial*> materials;            // TODO: how to resource-manage?

            friend class IResource2;
    };

    // ====================================================================== //
    //  class GLModel
    // ====================================================================== //

    unique_ptr<IGLModel> IGLModel::Create(RenderingKit* rk, const char* fileName) {
        return make_unique<GLModel>(rk, fileName);
    }

    void GLModel::Draw(const glm::mat4x4& transform) {
        zombie_assert(this->model);

        auto rm = rk->GetRenderingManagerBackend();
        rm->VertexCacheFlush();

        glm::mat4x4* projection, * modelView;
        rm->GetModelViewProjectionMatrices(&projection, &modelView);
        glm::mat4x4 myModelView = *modelView * transform * glm::scale({}, Float3(-16, -16, -16));

        for (auto& scene : this->model->scenes) {
            for (auto node_index : scene.nodes) {
                p_DrawNode(this->model->nodes[node_index], *projection, myModelView);
            }
        }
    }

    void GLModel::p_DrawNode(tinygltf::Node& node, const glm::mat4x4& projection, const glm::mat4x4& modelView_in) {
        auto rm = rk->GetRenderingManagerBackend();

        glm::mat4x4 modelView;

        if (!node.matrix.empty()) {
            zombie_assert(node.matrix.size() == 16);
            zombie_assert(node.translation.empty());
            zombie_assert(node.rotation.empty());
            zombie_assert(node.scale.empty());

            modelView = modelView_in * glm::mat4x4(node.matrix[0], node.matrix[1], node.matrix[2], node.matrix[3],
                                                   node.matrix[4], node.matrix[5], node.matrix[6], node.matrix[7],
                                                   node.matrix[8], node.matrix[9], node.matrix[10], node.matrix[11],
                                                   node.matrix[12], node.matrix[13], node.matrix[14], node.matrix[15]);
        }
        else {
            auto T = (node.translation.size() == 3) ? glm::translate({}, Float3(node.translation[0], node.translation[1], node.translation[2])) : glm::mat4x4();
            auto R = (node.rotation.size() == 4) ? glm::fquat(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]) : glm::fquat();
            auto S = (node.scale.size() == 3) ? glm::scale({}, Float3(node.scale[0], node.scale[1], node.scale[2])) : glm::mat4x4();

            modelView = modelView_in * T * glm::mat4_cast(R) * S;
        }

        if (node.mesh >= 0) {
            auto& mesh = this->model->meshes[node.mesh];

            for (auto& primitive : mesh.primitives) {
                GLuint vao;
                glGenVertexArrays(1, &vao);
                glBindVertexArray(vao);

                // Bind vertex attributes
                GLStateTracker::BindArrayBuffer(geometryBuffer);

                int index = 0;
                for (const auto& [name, accessor_index] : primitive.attributes) {
                    int location = rm->GetGlobalCache().GetAttribLocationByName(ps_MapAttributeName(name));

                    auto& accessor = this->model->accessors[accessor_index];
                    auto& bufferView = this->model->bufferViews[accessor.bufferView];

                    auto componentType = ps_AccessorComponentTypeToGLType(accessor.componentType);
                    auto numComponents = ps_GetNumComponentes(accessor.type);
                    //struct vec3f {float x,y,z;};
                    //auto vertexData = reinterpret_cast<vec3f*>(&this->model->buffers[0].data[0] + bufferView.byteOffset + accessor.byteOffset);

                    glEnableVertexAttribArray(location);
                    glVertexAttribPointer(location,
                                          numComponents,
                                          componentType,
                                          accessor.normalized ? GL_TRUE : GL_FALSE,
                                          accessor.ByteStride(this->model->bufferViews[accessor.bufferView]),
                                          reinterpret_cast<GLvoid*>(bufferView.byteOffset + accessor.byteOffset));
                    rm->CheckErrors("GLModel::p_DrawNode");
                }

                // Now set up indices
                GLStateTracker::BindElementArrayBuffer(0);
                GLStateTracker::BindElementArrayBuffer(geometryBuffer);
                auto& accessor = this->model->accessors[primitive.indices];
                auto& bufferView = this->model->bufferViews[accessor.bufferView];

                // Bind a material and render it!
                auto material = (primitive.material >= 0) ? this->materials[primitive.material] : this->materials[0];
                MaterialSetupOptions options {};
                options.type = MaterialSetupOptions::kNone;

                material->GLSetup(options, projection, modelView);

                auto componentType = ps_AccessorComponentTypeToGLType(accessor.componentType);
                //auto elementData = reinterpret_cast<unsigned short*>(&this->model->buffers[0].data[0] + bufferView.byteOffset + accessor.byteOffset);

                glDrawElements(ps_PrimitiveModeToGLMode(primitive.mode),
                               accessor.count,
                               componentType,
                               reinterpret_cast<GLvoid*>(bufferView.byteOffset + accessor.byteOffset));

                rm->CheckErrors("GLModel::p_DrawNode");

                glDeleteVertexArrays(1, &vao);
            }
        }

        for (auto node_index : node.children) {
            p_DrawNode( this->model->nodes[node_index], projection, modelView);
        }
    }

    bool GLModel::Preload(IResourceManager2* resMgr)
    {
        if (state == PRELOADED || state == REALIZED)        // TODO: necessary?
            return true;

        tinygltf::TinyGLTF loader;
        std::string err;
        std::string warn;

        auto engine = rk->GetEngine();
        ImageLoader iml { engine };

        unique_ptr<InputStream> stream(engine->OpenInput((this->path + ".glb").c_str()));

        if (!stream)
            return {};

        zombie_assert(stream->finite());
        auto size = stream->getSize();

        std::vector<uint8_t> bytes(size);
        if (stream->read(&bytes[0], size) != size) {
            // FIXME: must set error
            return false;
        }

        loader.SetImageLoader(&ImageLoader::LoadImageData, &iml);

        this->model = make_unique<tinygltf::Model>();
        bool ret = loader.LoadBinaryFromMemory(this->model.get(), &err, &warn, &bytes[0], size, "", tinygltf::REQUIRE_ALL);

        if (!warn.empty()) {
            printf("Warn: %s\n", warn.c_str());
        }

        if (!err.empty()) {
            printf("Err: %s\n", err.c_str());
        }

        if (!ret) {
            printf("Failed to parse glTF\n");
            this->model.reset();
            return false;
        }

        return true;
    }

    bool GLModel::Realize(IResourceManager2* resMgr) {
        if (state == REALIZED)              // TODO: necessary?
            return true;

        zombie_assert(model->buffers.size() == 1);

        // TODO: would be cool to factor all glCalls() out into specific functions
        glGenBuffers(1, &geometryBuffer);
        GLStateTracker::BindArrayBuffer(geometryBuffer);
        glBufferData(GL_ARRAY_BUFFER, model->buffers[0].data.size(), &model->buffers[0].data[0], GL_STATIC_DRAW);

        // Initialize materials
        for (auto& material : model->materials) {
            auto rkMaterial = rk->GetRenderingManagerBackend()->GetSharedResourceManager2()->GetResource<IGLMaterial>("shader=path=ntile/shaders/world", 0);
            zombie_assert(rkMaterial);
            this->materials.push_back(rkMaterial);
        }

        return true;
    }

    // ====================================================================== //
    //  class ImageLoader
    // ====================================================================== //

    bool ImageLoader::LoadImageData(tinygltf::Image* image, std::string* err, std::string* warn,
                                    int req_width, int req_height, const unsigned char *bytes,
                                    int size, void* userdata) {
        auto self = *static_cast<ImageLoader*>(userdata);

        Pixmap_t pm;
        ArrayIOStream ios(bytes, size);         // TODO: SpanIOStream!

        if (!Pixmap::LoadFromStream(self.engine, &pm, &ios, nullptr)) {
            // TODO: propagate error
            if (err) {
                (*err) += "Unknown image format.\n";
            }
            return false;
        }

        if (req_width > 0) {
            if (req_width != pm.info.size.x) {
                if (err) {
                    (*err) += "Image width mismatch.\n";
                }
                return false;
            }
        }

        if (req_height > 0) {
            if (req_height != pm.info.size.y) {
                if (err) {
                    (*err) += "Image height mismatch.\n";
                }
                return false;
            }
        }

        image->width = pm.info.size.x;
        image->height = pm.info.size.y;
        if (pm.info.format == PixmapFormat_t::RGB8) {
            image->component = 3;
        }
        else if (pm.info.format == PixmapFormat_t::RGBA8) {
            image->component = 4;
        }
        else {
            if (err) {
                (*err) += "Unsupported image format.\n";
            }
            return false;
        }

        image->image.resize(static_cast<size_t>(image->width * image->height * image->component));
        std::copy(&pm.pixelData[0], &pm.pixelData[0] + image->width * image->height * image->component, image->image.begin());

        return true;
    }
}
