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

    enum { kMaxJoints = 20 };
    auto magic_constant = 0.016f;
    auto magic_constant2 = 3.0f;
    auto magic_string = "";

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
        else if (name == "TEXCOORD_0") {
            return "in_UV";
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

            bool IsAnimationPlaying();
            void PlayAnimation(const char* name);
            void StopAnimation();
            void ResetPose();

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
            void p_DrawNode(int node_index, const glm::mat4x4& projection, const glm::mat4x4& modelView);
            void p_GetKeyframesForTime(const tinygltf::Accessor& timeAccessor,
                                       float animationTime,
                                       std::pair<size_t, size_t>* keyframes_out,
                                       float* t_out);
            void p_UpdateAnimation(float step);
            void p_UpdateNodeMatrixRecursive(int node_index, const glm::mat4x4& parent);
            void p_UpdateSkin(int skin_index, const glm::mat4x4& mesh_matrix, glm::mat4x4* jointMatrix);

            RenderingKit* rk;
            std::string path;

            State_t state = CREATED;

            // preloaded
            unique_ptr<tinygltf::Model> model;

            // realized
            GLuint geometryBuffer;
            std::vector<IGLMaterial*> materials;            // TODO: how to resource-manage?
            std::vector<glm::mat4x4> node_matrix;
            int animationIndex = -1;
            float animationTime;

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
        glm::mat4x4 myModelView = *modelView * transform;

        for (auto& scene : this->model->scenes) {
            for (auto node_index : scene.nodes) {
                p_UpdateNodeMatrixRecursive(node_index, {});
            }
            for (auto node_index : scene.nodes) {
                p_DrawNode(node_index, *projection, myModelView);
            }
        }

        if (this->animationIndex >= 0) {
            p_UpdateAnimation(magic_constant);
        }
    }

    void GLModel::p_DrawNode(int node_index, const glm::mat4x4& projection, const glm::mat4x4& modelView) {
        auto rm = rk->GetRenderingManagerBackend();
        auto& node = this->model->nodes[node_index];

        if (node.mesh >= 0) {
            glm::mat4x4 jointMatrices[kMaxJoints];

            if (node.skin >= 0) {
                p_UpdateSkin(node.skin, this->node_matrix[node_index], jointMatrices);
            }

            auto& mesh = this->model->meshes[node.mesh];

            for (auto& primitive : mesh.primitives) {
                GLuint vao;
                glGenVertexArrays(1, &vao);
                glBindVertexArray(vao);

                // Bind vertex attributes
                GLStateTracker::BindArrayBuffer(geometryBuffer);

                int index = 0;
                for (const auto& pair : primitive.attributes) {
                    // TODO[C++17]
                    const auto& name = pair.first;
                    const auto accessor_index = pair.second;

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

                material->GLSetup(options, projection, modelView * this->node_matrix[node_index]);

                // Upload joint matrices
                if (node.skin >= 0) {
                    auto location = static_cast<IGLShaderProgram*>(material->GetShader())->GetUniformLocation("jointMatrices");
                    glUniformMatrix4fv(location, kMaxJoints, false, reinterpret_cast<const float*>(jointMatrices));
                }

                auto componentType = ps_AccessorComponentTypeToGLType(accessor.componentType);
                //auto elementData = reinterpret_cast<unsigned short*>(&this->model->buffers[0].data[0] + bufferView.byteOffset + accessor.byteOffset);

                glDrawElements(ps_PrimitiveModeToGLMode(primitive.mode),
                               accessor.count,
                               componentType,
                               reinterpret_cast<GLvoid*>(bufferView.byteOffset + accessor.byteOffset));
                GLStateTracker::IncreaseDrawCallCounter(accessor.count / 3);

                rm->CheckErrors("GLModel::p_DrawNode");

                glDeleteVertexArrays(1, &vao);
            }
        }

        for (auto child_index : node.children) {
            p_DrawNode(child_index, projection, modelView);
        }
    }

    void GLModel::p_GetKeyframesForTime(const tinygltf::Accessor& timeAccessor,
                                        float animationTime,
                                        std::pair<size_t, size_t>* keyframes_out,
                                        float* t_out) {
        zombie_assert(timeAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
        zombie_assert(timeAccessor.type == TINYGLTF_TYPE_SCALAR);

        auto& bufferView = this->model->bufferViews[timeAccessor.bufferView];
        auto& buffer = this->model->buffers[bufferView.buffer];

        auto p_floats = reinterpret_cast<float*>(
                &buffer.data[bufferView.byteOffset + timeAccessor.byteOffset]);

        // Binary search
        size_t min = 0;
        size_t max = timeAccessor.count;

        if (animationTime < p_floats[min]) {
            *keyframes_out = {min, min};
            *t_out = 0;
            return;
        }
        else if (animationTime >= p_floats[max-1]) {
            *keyframes_out = {max-1, max-1};
            *t_out = 0;
            return;
        }

        while (max - min > 1) {
            size_t middle = (min + max) / 2;
            if (animationTime >= p_floats[middle]) {
                min = middle;
            }
            else {
                max = middle;
            }
        }

        zombie_assert(p_floats[min] <= animationTime);
        zombie_assert(p_floats[max] >= animationTime);
        zombie_assert(max < timeAccessor.count);

        *keyframes_out = {min, max};
        if (min != max) {
            *t_out = (animationTime - p_floats[min]) / (p_floats[max] - p_floats[min]);
        }
        else {
            *t_out = 0;
        }
    }

    void GLModel::p_UpdateAnimation(float step) {
        this->animationTime += step;

        if (this->animationTime > magic_constant2) {
            this->animationTime = 0;
        }

        auto& anim = this->model->animations[animationIndex];

        for (auto& channel : anim.channels) {
            auto& sampler = anim.samplers[channel.sampler];

            auto& timeAccessor = this->model->accessors[sampler.input];

            std::pair<size_t, size_t> keyframes;
            float t;
            p_GetKeyframesForTime(timeAccessor, this->animationTime, &keyframes, &t);

            auto& valueAccessor = this->model->accessors[sampler.output];
            auto& bufferView = this->model->bufferViews[valueAccessor.bufferView];
            auto& buffer = this->model->buffers[bufferView.buffer];

            if (valueAccessor.type == TINYGLTF_TYPE_VEC3) {
                zombie_assert(valueAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

                glm::vec3 values[2];
                memcpy(&values[0], &buffer.data[bufferView.byteOffset +
                       valueAccessor.byteOffset + keyframes.first * sizeof(glm::vec3)],
                       sizeof(glm::vec3));
                memcpy(&values[1], &buffer.data[bufferView.byteOffset +
                       valueAccessor.byteOffset + keyframes.second * sizeof(glm::vec3)],
                       sizeof(glm::vec3));

                auto interp = values[0] * (1 - t) + values[1] * t;

                if (channel.target_path == "translation") {
                    model->nodes[channel.target_node].translation.resize(3);
                    model->nodes[channel.target_node].translation[0] = interp.x;
                    model->nodes[channel.target_node].translation[1] = interp.y;
                    model->nodes[channel.target_node].translation[2] = interp.z;
                }
                else if (channel.target_path == "scale") {
                    model->nodes[channel.target_node].scale.resize(3);
                    model->nodes[channel.target_node].scale[0] = interp.x;
                    model->nodes[channel.target_node].scale[1] = interp.y;
                    model->nodes[channel.target_node].scale[2] = interp.z;
                }
                else {
                    zombie_assert(channel.target_path != channel.target_path);
                }
            }
            else if (valueAccessor.type == TINYGLTF_TYPE_VEC4) {
                zombie_assert(valueAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

                glm::vec4 values[2];
                memcpy(&values[0], &buffer.data[bufferView.byteOffset +
                       valueAccessor.byteOffset + keyframes.first * sizeof(glm::vec4)],
                       sizeof(glm::vec4));
                memcpy(&values[1], &buffer.data[bufferView.byteOffset +
                       valueAccessor.byteOffset + keyframes.second * sizeof(glm::vec4)],
                       sizeof(glm::vec4));

                auto interp = values[0] * (1 - t) + values[1] * t;

                if (channel.target_path == "rotation") {
                    model->nodes[channel.target_node].rotation.resize(4);
                    model->nodes[channel.target_node].rotation[0] = interp.x;
                    model->nodes[channel.target_node].rotation[1] = interp.y;
                    model->nodes[channel.target_node].rotation[2] = interp.z;
                    model->nodes[channel.target_node].rotation[3] = interp.w;
                }
                else {
                    zombie_assert(channel.target_path != channel.target_path);
                }
            }
            else {
                zombie_assert(valueAccessor.type != valueAccessor.type);
            }
        }
    }

    void GLModel::p_UpdateNodeMatrixRecursive(int node_index, const glm::mat4x4& parent_matrix)
    {
        auto& node = this->model->nodes[node_index];

        if (!node.matrix.empty()) {
            zombie_assert(node.matrix.size() == 16);
            zombie_assert(node.translation.empty());
            zombie_assert(node.rotation.empty());
            zombie_assert(node.scale.empty());

            this->node_matrix[node_index] = parent_matrix * glm::mat4x4(
                                            node.matrix[0], node.matrix[1], node.matrix[2], node.matrix[3],
                                            node.matrix[4], node.matrix[5], node.matrix[6], node.matrix[7],
                                            node.matrix[8], node.matrix[9], node.matrix[10], node.matrix[11],
                                            node.matrix[12], node.matrix[13], node.matrix[14], node.matrix[15]);
        }
        else {
            auto T = (node.translation.size() == 3) ? glm::translate({}, Float3(node.translation[0], node.translation[1], node.translation[2])) : glm::mat4x4();
            auto R = (node.rotation.size() == 4) ? glm::fquat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]) : glm::fquat();
            auto S = (node.scale.size() == 3) ? glm::scale({}, Float3(node.scale[0], node.scale[1], node.scale[2])) : glm::mat4x4();

            this->node_matrix[node_index] = parent_matrix * T * glm::mat4_cast(R) * S;
        }

        for (auto child_index : node.children) {
            p_UpdateNodeMatrixRecursive(child_index, this->node_matrix[node_index]);
        }
    }

    void GLModel::p_UpdateSkin(int skin_index, const glm::mat4x4& mesh_matrix, glm::mat4x4* jointMatrix) {
        auto& skin = this->model->skins[skin_index];
        zombie_assert(skin.joints.size() < kMaxJoints);

        auto& inverseBindMatrixAccessor = this->model->accessors[skin.inverseBindMatrices];
        zombie_assert(inverseBindMatrixAccessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
        zombie_assert(inverseBindMatrixAccessor.count == skin.joints.size());
        zombie_assert(inverseBindMatrixAccessor.type == TINYGLTF_TYPE_MAT4);
        zombie_assert(this->model->bufferViews[inverseBindMatrixAccessor.bufferView].byteStride == 0);

        auto& bufferView = this->model->bufferViews[inverseBindMatrixAccessor.bufferView];
        auto& buffer = this->model->buffers[bufferView.buffer];

        for (int j = 0; j < skin.joints.size(); j++) {
            glm::mat4 inverseBindMatrix;
            memcpy(&inverseBindMatrix,
                   &buffer.data[bufferView.byteOffset + inverseBindMatrixAccessor.byteOffset + j * sizeof(glm::mat4)],
                   sizeof(glm::mat4));

            jointMatrix[j] = glm::inverse(mesh_matrix) *
                             this->node_matrix[skin.joints[j]] *
                             inverseBindMatrix;
        }
    }

    void GLModel::PlayAnimation(const char* name) {
        if (!this->model) {
            return;
        }

        for (size_t i = 0; i < this->model->animations.size(); i++) {
            if (this->model->animations[i].name == name) {
                this->animationIndex = i;
                this->animationTime = 0;
                break;
            }
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
            auto rkMaterial = rk->GetRenderingManagerBackend()->GetSharedResourceManager2()->GetResource<IGLMaterial>("shader=path=ntile/shaders/skinned", 0);
            zombie_assert(rkMaterial);
            this->materials.push_back(rkMaterial);
        }

        this->node_matrix.resize(this->model->nodes.size());

        this->PlayAnimation(magic_string);
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
