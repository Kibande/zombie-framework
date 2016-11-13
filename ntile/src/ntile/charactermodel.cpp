
#include "entities/entities.hpp"
#include "ntile.hpp"

#include <framework/errorbuffer.hpp>
#include <framework/interpolator.hpp>

#include <glm/gtc/quaternion.hpp>

namespace ntile
{
    static const int ANIM_SLOWDOWN = 1;
    static const size_t VERTEX_SIZE = sizeof(ModelVertex);

    template <typename TimeType, typename ValueType>
    class GlmMixInterpolationMode : public InterpolationModeBase<TimeType, ValueType>
    {
        public:
            typedef typename InterpolationModeBase<TimeType, ValueType>::Keyframe Keyframe;
        
        public:
            void SetInterpolationBetween(const Keyframe& prev, const Keyframe& next)
            {
                this->prev = prev;
                this->next = next;
            }

            void SetTime(TimeType time)
            {
                this->value = glm::mix(this->prev.value, this->next.value, (time - this->prev.time) / (this->next.time - this->prev.time));
            }
    };

    struct Joint_t
    {
        String name;

        Float3 pos, diffpos;
        glm::quat boneRotation;

        glm::mat4 boneToParentSpace;
        glm::mat4 boneToModelSpace;
        glm::mat4 modelToBoneSpace;

        List<Joint_t*> children;

        bool dirty, wasDirty;

        Joint_t()
        {
            dirty = true;
        }

        ~Joint_t()
        {
            children.deleteAllItems();
        }

        Joint_t* Find(const char* name)
        {
            if (this->name == name)
                return this;

            Joint_t* joint;

            iterate2 (i, children)
                if ((joint = i->Find(name)) != nullptr)
                    return joint;
            
            return nullptr;
        }
    };

    struct BoneAnim_t
    {
        Joint_t* joint;
        
        Interpolator<float, glm::quat, GlmMixInterpolationMode<float, glm::quat>> rotation;
    };

    struct MeshBuildContext_t
    {
        PrimitiveType primitiveType;

        ArrayIOStream vertexData;
    };

    struct CharacterModel::Animation
    {
        unsigned int numBoneAnims;
        BoneAnim_t* boneAnims;

        float timeScale;
        int duration, tick;
    };

    static Float3 glmPitchYawRoll(const Float3& pitchYawRoll)
    {
        // OpenGL:  pitch=X yaw=Y roll=Z
        // zfw:     pitch=X yaw=Z roll=Y

        return Float3(pitchYawRoll.x, pitchYawRoll.z, pitchYawRoll.y) * f_pi / 180.0f;
    }

    static bool ParseJoint(cfx2::Node jointNode, Float3& pos)
    {
        if (Util::ParseVec(jointNode.getAttrib("pos"), &pos[0], 0, 3) <= 0)
            return ErrorBuffer::SetError(g_eb, EX_DOCUMENT_MALFORMED,
                    "desc", "Attribute 'pos' is mandatory for a Joint node",
                    "function", li_functionName,
                    nullptr),
                    false;

        return true;
    }

    static bool ParseCuboid(cfx2::Node cuboidNode, Float3& pos, Float3& size, Byte4& colour, const char*& bone, bool& collision)
    {
        if (Util::ParseVec(cuboidNode.getAttrib("pos"), &pos[0], 0, 3) <= 0
                || Util::ParseVec(cuboidNode.getAttrib("size"), &size[0], 0, 3) <= 0)
            return ErrorBuffer::SetError(g_eb, EX_DOCUMENT_MALFORMED,
                    "desc", "Attributes 'pos', 'size' are mandatory for a Cuboid node",
                    "function", li_functionName,
                    nullptr),
                    false;

        if (!Util::ParseColour(cuboidNode.getAttrib("colour"), colour))
            colour = RGBA_WHITE;
        
        bone = cuboidNode.getAttrib("bone");
        collision = cuboidNode.hasAttrib("collision");

        return true;
    }

    static bool CreateJoint(Joint_t* parent, cfx2::Node jointNode, Joint_t** joint_out)
    {
        Joint_t* joint;

        if (parent != nullptr)
        {
            Float3 pos;

            if (!ParseJoint(jointNode, pos))
                return false;

            joint = new Joint_t;

            joint->name = jointNode.getText();
            joint->pos = pos;
            joint->diffpos = pos - parent->pos;
            joint->boneToParentSpace = glm::translate(glm::mat4x4(), joint->diffpos);
            joint->boneToModelSpace = parent->boneToModelSpace * joint->boneToParentSpace;
            joint->modelToBoneSpace = glm::inverse(joint->boneToModelSpace);

            parent->children.add(joint);
        }
        else
        {
            joint = new Joint_t;

            joint->name = "_root";

            *joint_out = joint;
        }

        for (auto i : jointNode)
            if (!CreateJoint(joint, i, nullptr))
                return false;

        return true;
    }

    static void ResetRotation(Joint_t* joint, bool recursive)
    {
        joint->boneRotation = glm::quat();

        if (recursive)
            iterate2 (i, joint->children)
                ResetRotation(i, true);
    }

//#ifndef ZOMBIE_CTR
    static void setUpCuboidSide(ModelVertex*& p_vertices, const Float3 corners[8], const Float3& normal,
            unsigned cornerA, unsigned cornerB, unsigned cornerC, unsigned cornerD, Byte4 colour)
    {
        p_vertices[0].x = corners[cornerA].x;
        p_vertices[0].y = corners[cornerA].y;
        p_vertices[0].z = corners[cornerA].z;
        p_vertices[0].n[0] = (int16_t)(normal.x * INT16_MAX);
        p_vertices[0].n[1] = (int16_t)(normal.y * INT16_MAX);
        p_vertices[0].n[2] = (int16_t)(normal.z * INT16_MAX);
        p_vertices[0].n[3] = 0;
        p_vertices[0].u = 0.0f;
        p_vertices[0].v = 0.0f;

        p_vertices[1].x = corners[cornerB].x;
        p_vertices[1].y = corners[cornerB].y;
        p_vertices[1].z = corners[cornerB].z;
        p_vertices[1].n[0] = (int16_t)(normal.x * INT16_MAX);
        p_vertices[1].n[1] = (int16_t)(normal.y * INT16_MAX);
        p_vertices[1].n[2] = (int16_t)(normal.z * INT16_MAX);
        p_vertices[1].n[3] = 0;
        p_vertices[1].u = 0.0f;
        p_vertices[1].v = 1.0f;

        p_vertices[2].x = corners[cornerC].x;
        p_vertices[2].y = corners[cornerC].y;
        p_vertices[2].z = corners[cornerC].z;
        p_vertices[2].n[0] = (int16_t)(normal.x * INT16_MAX);
        p_vertices[2].n[1] = (int16_t)(normal.y * INT16_MAX);
        p_vertices[2].n[2] = (int16_t)(normal.z * INT16_MAX);
        p_vertices[2].n[3] = 0;
        p_vertices[2].u = 1.0f;
        p_vertices[2].v = 1.0f;

        p_vertices[3].x = corners[cornerA].x;
        p_vertices[3].y = corners[cornerA].y;
        p_vertices[3].z = corners[cornerA].z;
        p_vertices[3].n[0] = (int16_t)(normal.x * INT16_MAX);
        p_vertices[3].n[1] = (int16_t)(normal.y * INT16_MAX);
        p_vertices[3].n[2] = (int16_t)(normal.z * INT16_MAX);
        p_vertices[3].n[3] = 0;
        p_vertices[3].u = 0.0f;
        p_vertices[3].v = 0.0f;

        p_vertices[4].x = corners[cornerC].x;
        p_vertices[4].y = corners[cornerC].y;
        p_vertices[4].z = corners[cornerC].z;
        p_vertices[4].n[0] = (int16_t)(normal.x * INT16_MAX);
        p_vertices[4].n[1] = (int16_t)(normal.y * INT16_MAX);
        p_vertices[4].n[2] = (int16_t)(normal.z * INT16_MAX);
        p_vertices[4].n[3] = 0;
        p_vertices[4].u = 1.0f;
        p_vertices[4].v = 1.0f;

        p_vertices[5].x = corners[cornerD].x;
        p_vertices[5].y = corners[cornerD].y;
        p_vertices[5].z = corners[cornerD].z;
        p_vertices[5].n[0] = (int16_t)(normal.x * INT16_MAX);
        p_vertices[5].n[1] = (int16_t)(normal.y * INT16_MAX);
        p_vertices[5].n[2] = (int16_t)(normal.z * INT16_MAX);
        p_vertices[5].n[3] = 0;
        p_vertices[5].u = 1.0f;
        p_vertices[5].v = 0.0f;

        memcpy(&p_vertices[0].rgba[0], &colour, 4);
        memcpy(&p_vertices[1].rgba[0], &colour, 4);
        memcpy(&p_vertices[2].rgba[0], &colour, 4);
        memcpy(&p_vertices[3].rgba[0], &colour, 4);
        memcpy(&p_vertices[4].rgba[0], &colour, 4);
        memcpy(&p_vertices[5].rgba[0], &colour, 4);

        p_vertices += 6;
    }
/*#else
        static void setUpCuboidSide(ModelVertex*& p_vertices, const Float3 corners[8], const Float3& normal,
            unsigned cornerA, unsigned cornerB, unsigned cornerC, unsigned cornerD, Byte4 colour)
    {
        p_vertices[0].x = corners[cornerA].x;
        p_vertices[0].y = corners[cornerA].y;
        p_vertices[0].z = corners[cornerA].z;
        p_vertices[0].n[0] = normal.x;
        p_vertices[0].n[1] = normal.y;
        p_vertices[0].n[2] = normal.z;
        p_vertices[0].u = 0.0f;
        p_vertices[0].v = 0.0f;

        p_vertices[1].x = corners[cornerB].x;
        p_vertices[1].y = corners[cornerB].y;
        p_vertices[1].z = corners[cornerB].z;
        p_vertices[1].n[0] = normal.x;
        p_vertices[1].n[1] = normal.y;
        p_vertices[1].n[2] = normal.z;
        p_vertices[1].u = 0.0f;
        p_vertices[1].v = 1.0f;

        p_vertices[2].x = corners[cornerC].x;
        p_vertices[2].y = corners[cornerC].y;
        p_vertices[2].z = corners[cornerC].z;
        p_vertices[2].n[0] = normal.x;
        p_vertices[2].n[1] = normal.y;
        p_vertices[2].n[2] = normal.z;
        p_vertices[2].u = 1.0f;
        p_vertices[2].v = 1.0f;

        p_vertices[3].x = corners[cornerA].x;
        p_vertices[3].y = corners[cornerA].y;
        p_vertices[3].z = corners[cornerA].z;
        p_vertices[3].n[0] = normal.x;
        p_vertices[3].n[1] = normal.y;
        p_vertices[3].n[2] = normal.z;
        p_vertices[3].u = 0.0f;
        p_vertices[3].v = 0.0f;

        p_vertices[4].x = corners[cornerC].x;
        p_vertices[4].y = corners[cornerC].y;
        p_vertices[4].z = corners[cornerC].z;
        p_vertices[4].n[0] = normal.x;
        p_vertices[4].n[1] = normal.y;
        p_vertices[4].n[2] = normal.z;
        p_vertices[4].u = 1.0f;
        p_vertices[4].v = 1.0f;

        p_vertices[5].x = corners[cornerD].x;
        p_vertices[5].y = corners[cornerD].y;
        p_vertices[5].z = corners[cornerD].z;
        p_vertices[5].n[0] = normal.x;
        p_vertices[5].n[1] = normal.y;
        p_vertices[5].n[2] = normal.z;
        p_vertices[5].u = 1.0f;
        p_vertices[5].v = 0.0f;

        p_vertices += 6;
    }
#endif*/
    CharacterModel::CharacterModel(String&& path)
            : state(CREATED), path(std::forward<String>(path))
    {
        vertices = nullptr;
        forceSingleRefresh = false;
    }

    CharacterModel::~CharacterModel()
    {
        Unload();
    }

    void CharacterModel::AddAnimation(cfx2::Node animNode)
    {
        ZFW_ASSERT(animNode.getText() != nullptr)

        Animation* anim = new Animation;
        anim->numBoneAnims = 0;
        anim->boneAnims = Allocator<BoneAnim_t>::allocate(animNode.getNumChildren());
        anim->duration = animNode.getAttribInt("duration", 0);
        anim->tick = -1;

        for (auto boneAnimNode : animNode)
        {
            BoneAnim_t& boneAnim = anim->boneAnims[anim->numBoneAnims++];
            li::constructPointer(&boneAnim);

            boneAnim.joint = skeleton->Find(boneAnimNode.getName());
            ZFW_ASSERT(boneAnim.joint != nullptr)

            String copyFrom = boneAnimNode.getAttrib("copyFrom");

            if (!copyFrom.isEmpty())
            {
                intptr_t delim = copyFrom.findChar('/');

                ZFW_ASSERT(delim > 0)

                Animation* srcAnim = animations.get(copyFrom.subString(0, delim));

                Joint_t* srcJoint = skeleton->Find(copyFrom.dropLeftPart(delim + 1));
                ZFW_ASSERT(srcJoint != nullptr)

                size_t j;

                for (j = 0; j < srcAnim->numBoneAnims; j++)
                {
                    if (srcAnim->boneAnims[j].joint == srcJoint)
                        break;
                }

                ZFW_ASSERT(j < srcAnim->numBoneAnims)

                boneAnim.rotation.Init(srcAnim->boneAnims[j].rotation);
            }
            else
            {
                List<Interpolator<float, glm::quat>::Keyframe> rotationKeyframes;

                for (auto keyframeNode : boneAnimNode)
                {
                    ZFW_ASSERT(strcmp(keyframeNode.getName(), "Keyframe") == 0)
                    ZFW_ASSERT(keyframeNode.getText() != nullptr)

                    float time = (float) strtod(keyframeNode.getText(), nullptr);

                    const char* rotation = keyframeNode.getAttrib("rotation");

                    if (rotation != nullptr)
                    {
                        Float3 pitchYawRoll;
                        Util::ParseVec(rotation, &pitchYawRoll[0], 3, 3);

                        auto& keyframe = rotationKeyframes.addEmpty();
                        keyframe.time = time;
                        keyframe.value = glm::quat(glmPitchYawRoll(pitchYawRoll));
                    }
                }
            
                if (!rotationKeyframes.isEmpty())
                    boneAnim.rotation.Init(rotationKeyframes.c_array(), rotationKeyframes.getLength(), true);
            }
        }

        animations.set((String&&) animNode.getText(), (Animation*&&) anim);
    }

    bool CharacterModel::AddCuboid(MeshBuildContext_t* ctx, cfx2::Node cuboidNode)
    {
        static const size_t numFaces = 6;
        static const size_t verticesPerFace = 6;

        Float3 pos, size;
        Byte4 colour;
        const char* bone;
        bool collision;

        if (!ParseCuboid(cuboidNode, pos, size, colour, bone, collision))
            return false;

        Float3 corners[8];
        corners[0] =  pos + Float3( 0,      0,      0 );
        corners[1] =  pos + Float3( 0,      size.y, 0 );
        corners[2] =  pos + Float3( size.x, size.y, 0 );
        corners[3] =  pos + Float3( size.x, 0,      0 );
        corners[4] =  pos + Float3( 0,      0,      size.z );
        corners[5] =  pos + Float3( 0,      size.y, size.z );
        corners[6] =  pos + Float3( size.x, size.y, size.z );
        corners[7] =  pos + Float3( size.x, 0,      size.z );

        Joint_t* joint;

        if (bone != nullptr)
        {
            joint = skeleton->Find(bone);

            if (joint == nullptr)
                g_sys->Printf(kLogWarning, "CharacterModel: Ignoring invalid bone '%s' for primitive '%s'\n", bone, cuboidNode.getText());
            else
            {
                // Transform coordinates into bone space

                for (int i = 0; i < 8; i++)
                    corners[i] = Float3(joint->modelToBoneSpace * Float4(corners[i], 1.0f));

                auto& range = jointRanges.addEmpty();
                range.first = (uint32_t)(ctx->vertexData.getSize() / VERTEX_SIZE);
                range.count = numFaces * verticesPerFace;
                range.joint = joint;
            }
        }
        else
            joint = nullptr;

        if (collision)
        {
            AABB_t& aabb = collisions.addEmpty();

            if (joint != nullptr)
            {
                aabb.min = Float3(joint->modelToBoneSpace * Float4(corners[0], 1.0f));
                aabb.max = Float3(joint->modelToBoneSpace * Float4(corners[6], 1.0f));
            }
            else
            {
                aabb.min = corners[0];
                aabb.max = corners[6];
            }

            aabb.joint = joint;
        }

        ModelVertex* p_vertices = ctx->vertexData.writeItemsEmpty<ModelVertex>(numFaces * verticesPerFace);

        setUpCuboidSide( p_vertices, corners, Float3( 0.0f, 0.0f, 1.0f ), 4, 5, 6, 7, colour );
        setUpCuboidSide( p_vertices, corners, Float3( 0.0f, 0.0f, -1.0f ), 1, 2, 3, 0, colour );

        setUpCuboidSide( p_vertices, corners, Float3( -1.0f, 0.0f, 0.0f ), 4, 0, 1, 5, colour );
        setUpCuboidSide( p_vertices, corners, Float3( 1.0f, 0.0f, 0.0f ), 6, 2, 3, 7, colour );

        setUpCuboidSide( p_vertices, corners, Float3( 0.0f, 1.0f, 0.0f ), 5, 1, 2, 6, colour );
        setUpCuboidSide( p_vertices, corners, Float3( 0.0f, -1.0f, 0.0f ), 7, 3, 0, 4, colour );

        return true;
    }

    void CharacterModel::AnimationTick()
    {
        if (!anyRunningAnimations && !forceSingleRefresh)
            return;

        ResetRotation(skeleton.get(), true);

        iterate2 (i, activeAnims)
        {
            Animation* anim = i;

            for (unsigned int j = 0; j < anim->numBoneAnims; j++)
            {
                BoneAnim_t* boneAnim = &anim->boneAnims[j];
                Joint_t* joint = boneAnim->joint;

                joint->dirty = true;

                ZFW_DBGASSERT(boneAnim->rotation.GetNumKeyframes() != 0)
                
                if (anim->duration > 0)
                    joint->boneRotation = joint->boneRotation * boneAnim->rotation.AdvanceTime<CHECK_OVERFLOW>(1.0f / ANIM_SLOWDOWN);
                else
                    joint->boneRotation = joint->boneRotation * boneAnim->rotation.GetValue();
            }

            if (anim->duration > 0 && ++anim->tick == anim->duration * ANIM_SLOWDOWN)
            {
                anim->tick = -1;
                i.remove();
                UpdateAnyRunningAnimations();
                continue;
            }
        }

        UpdateJoint(skeleton.get(), nullptr, false);
        UpdateVertices();
        UpdateCollisions();

        forceSingleRefresh = false;
    }

    bool CharacterModel::BuildMesh(cfx2::Node meshNode, Mesh*& mesh)
    {
        vb = ir->CreateVertexBuffer();
        vf = g_modelVertexFormat.get();

        MeshBuildContext_t ctx;
        ctx.primitiveType = PRIMITIVE_TRIANGLES;

        for (auto child : meshNode)
        {
            if (strcmp(child.getName(), "Cuboid") == 0)
            {
                if (!AddCuboid(&ctx, child))
                    return false;
            }
        }

        mesh = new Mesh;
        mesh->vb.reset(vb);
        mesh->primitiveType = ctx.primitiveType;
        mesh->format = vf;
        mesh->offset = 0;
        mesh->count = (uint32_t)(ctx.vertexData.getSize() / VERTEX_SIZE);
        mesh->transform = glm::mat4x4();

        //g_sys->Printf(kLogInfo, "CharacterModel: %u bytes", (unsigned int) ctx.vertexData.getSize());
        mesh->vb->Upload(ctx.vertexData.getPtrUnsafe(), (size_t) ctx.vertexData.getSize());

        if (skeleton != nullptr)
        {
            vertices = (ModelVertex*) ctx.vertexData.detachData();
            pauseThread(20);        // WHAT THE ACTUAL FUCKING SHIT
        }

        return true;
    }

    void CharacterModel::Draw()
    {
        ZFW_ASSERT(model != nullptr)

        model->Draw();
    }

    Joint_t* CharacterModel::FindJoint(const char* name)
    {
        return (skeleton != nullptr) ? skeleton->Find(name) : nullptr;
    }

    CharacterModel::Animation* CharacterModel::GetAnimationByName(const char* name)
    {
        return animations.get(name);
    }

    Float3 CharacterModel::GetJointPos(Joint_t* joint)
    {
        return Float3(joint->boneToModelSpace * Float4(0.0f, 0.0f, 0.0f, 1.0f));
    }

    bool CharacterModel::Preload(IResourceManager2* resMgr)
    {
        return true;
    }

    bool CharacterModel::Realize(IResourceManager2* resMgr)
    {
        ZFW_ASSERT(model == nullptr)

        anyRunningAnimations = false;

        String filename = (String) path + ".cfx2";

        unique_ptr<InputStream> is(g_sys->OpenInput(filename));
        
        if (is == nullptr)
            return ErrorBuffer::SetError(g_eb, EX_ASSET_OPEN_ERR,
                    "desc", (const char*) sprintf_t<255>("Failed to load model '%s'", path.c_str()),
                    "function", li_functionName,
                    nullptr),
                    false;

        String contents = is->readWhole();
        is.reset();

        cfx2_Node* docNode;

        int rc = cfx2_read_from_string(&docNode, contents, nullptr);

        if (rc < 0)
            return ErrorBuffer::SetError(g_eb, EX_ASSET_CORRUPTED,
                    "desc", (const char*) sprintf_t<255>("Failed to parse model '%s': %s", path.c_str(), cfx2_get_error_desc(rc)),
                    "function", li_functionName,
                    nullptr),
                    false;

        cfx2::Document doc(docNode);

        // Skeleton
        cfx2::Node skeletonNode = doc.findChild("Skeleton");

        if (!skeletonNode.isNull())
        {
            Joint_t* root = nullptr;
            CreateJoint(nullptr, skeletonNode, &root);
            skeleton.reset(root);
        }

        // Mesh
        cfx2::Node meshNode = doc.findChild("Mesh");

        ZFW_ASSERT(!meshNode.isNull());

        Mesh* mesh = nullptr;

        if (!BuildMesh(meshNode, mesh))
        {
            g_sys->Printf(kLogError, "Error in CharacterModel loading - model '%s'", path.c_str());
            return false;
        }

        model.reset(ir->CreateModelFromMeshes(&mesh, 1));

        if (!model)
        {
            delete mesh;
            return false;
        }

        // Animation
        for (auto child : doc)
        {
            if (strcmp(child.getName(), "Animation") == 0)
                AddAnimation(child);
        }

        if (skeleton != nullptr)
            UpdateVertices();

        UpdateCollisions();

        return true;
    }

    void CharacterModel::StartAnimation(Animation* anim)
    {
        if (anim->tick < 0)
        {
            activeAnims.add(anim);
            UpdateAnyRunningAnimations();
            forceSingleRefresh = true;
        }

        anim->tick = 0;

        for (unsigned int i = 0; i < anim->numBoneAnims; i++)
        {
            BoneAnim_t* boneAnim = &anim->boneAnims[i];

            boneAnim->rotation.ResetTime();
        }
    }

    void CharacterModel::Unload()
    {
        free(vertices);

        iterate2 (i, animations)
        {
            Animation* anim = (*i).value;

            for (unsigned int j = 0; j < anim->numBoneAnims; j++)
            {
                BoneAnim_t* boneAnim = &anim->boneAnims[j];

                li::destructPointer(boneAnim);
            }

            free(anim->boneAnims);
            delete anim;
            anim = nullptr;
        }
    }

    void CharacterModel::Unrealize()
    {
        activeAnims.clear();

        model.reset();
    }

    void CharacterModel::UpdateAnyRunningAnimations()
    {
        anyRunningAnimations = false;

        iterate2 (i, activeAnims)
            if (i->duration > 0)
            {
                anyRunningAnimations = true;
                break;
            }
    }

    void CharacterModel::UpdateCollisions()
    {
        if (collisions.isEmpty())
            return;

        {
            const auto& aabb = collisions[0];

            if (aabb.joint != nullptr)
            {
                const Float4 min = aabb.joint->boneToModelSpace * Float4(aabb.min, 1.0f);
                const Float4 max = aabb.joint->boneToModelSpace * Float4(aabb.max, 1.0f);

                aabbMinMax[0] = Float3(min);
                aabbMinMax[1] = Float3(max);
            }
            else
            {
                aabbMinMax[0] = aabb.min;
                aabbMinMax[1] = aabb.max;
            }
        }

        for (size_t i = 1; i < collisions.getLength(); i++)
        {
            const auto& aabb = collisions[i];

            if (aabb.joint != nullptr)
            {
                const Float4 min = aabb.joint->boneToModelSpace * Float4(aabb.min, 1.0f);
                const Float4 max = aabb.joint->boneToModelSpace * Float4(aabb.max, 1.0f);

                aabbMinMax[0] = glm::min(Float3(min), aabbMinMax[0]);
                aabbMinMax[1] = glm::max(Float3(max), aabbMinMax[1]);
            }
            else
            {
                aabbMinMax[0] = glm::min(aabb.min, aabbMinMax[0]);
                aabbMinMax[1] = glm::max(aabb.max, aabbMinMax[1]);
            }
        }
    }

    void CharacterModel::UpdateJoint(Joint_t* joint, Joint_t* parent, bool force)
    {
        const bool dirty = (force || joint->dirty);

        if (dirty && parent != nullptr)
        {
            joint->boneToParentSpace = glm::mat4_cast(joint->boneRotation) * glm::translate(glm::mat4x4(), joint->diffpos);
            joint->boneToModelSpace = parent->boneToModelSpace * joint->boneToParentSpace;
        }

        iterate2 (i, joint->children)
            UpdateJoint(i, joint, dirty);

        joint->wasDirty = dirty;
        joint->dirty = false;
    }

    void CharacterModel::UpdateJointRange(const JointRange_t& range)
    {
        ModelVertex* vertices_out = (ModelVertex*) alloca(range.count * VERTEX_SIZE);

        ModelVertex* p_vertex = vertices_out;

        for (uint32_t i = range.first; i < range.first + range.count; i++)
        {
/*#ifdef ZOMBIE_CTR
            const Float4 pos = range.joint->boneToModelSpace * Float4(vertices[i].x, vertices[i].y, vertices[i].z, 1.0f);
            const Float4 normal = range.joint->boneToModelSpace * (Float4(vertices[i].n[0], vertices[i].n[1], vertices[i].n[2], 0.0f));

            p_vertex->x = pos.x;
            p_vertex->y = pos.y;
            p_vertex->z = pos.z;
            p_vertex->n[0] = normal.x;
            p_vertex->n[1] = normal.y;
            p_vertex->n[2] = normal.z;
#else*/
            const Float4 pos = range.joint->boneToModelSpace * Float4(vertices[i].x, vertices[i].y, vertices[i].z, 1.0f);
            const Float4 normal = range.joint->boneToModelSpace * (Float4(vertices[i].n[0], vertices[i].n[1], vertices[i].n[2], 0.0f) * (1.0f / INT16_MAX));

            p_vertex->x = pos.x;
            p_vertex->y = pos.y;
            p_vertex->z = pos.z;
            p_vertex->n[0] = (int16_t)(normal.x * INT16_MAX);
            p_vertex->n[1] = (int16_t)(normal.y * INT16_MAX);
            p_vertex->n[2] = (int16_t)(normal.z * INT16_MAX);

//#ifdef ZOMBIE_CTR
            memcpy(&p_vertex->u, &vertices[i].u, 8);
            memcpy(&p_vertex->rgba, &vertices[i].rgba, 4);
/*#else
            memcpy(&p_vertex->rgba, &vertices[i].rgba, 12);
#endif*/
//#endif

            p_vertex++;
        }

        vb->UploadRange(range.first * VERTEX_SIZE, range.count * VERTEX_SIZE, (const uint8_t*) vertices_out);
    }

    void CharacterModel::UpdateVertices()
    {
        iterate2 (i, jointRanges)
            if ((*i).joint->wasDirty)
                UpdateJointRange(i);
    }
}
