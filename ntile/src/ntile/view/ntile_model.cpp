
#include "ntile_model.hpp"
#include "view.hpp"

#include "../ntile.hpp"

#include <framework/colorconstants.hpp>
#include <framework/errorbuffer.hpp>
#include <framework/interpolator.hpp>
#include <framework/resourcemanager2.hpp>

#include <RenderingKit/RenderingKitUtility.hpp>

namespace ntile
{
    using namespace RenderingKit;

    struct ModelVertex
    {
        float x, y, z;
        int16_t n[4];
        uint8_t rgba[4];
        float u, v;
    };

    static const VertexAttrib_t modelVertexAttribs[] =
    {
        {"in_Position", 0,  RK_ATTRIB_FLOAT_3, RK_ATTRIB_NOT_NORMALIZED},
        {"in_Normal",   12, RK_ATTRIB_SHORT_3},
        {"in_Color",    20, RK_ATTRIB_UBYTE_4},
        {"in_UV",       24, RK_ATTRIB_FLOAT_3},
        {}
    };

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

    struct MeshBuildContext_t
    {
        RKPrimitiveType_t primitiveType;

        ArrayIOStream vertexData;

        List<StudioPrimitive_t*> studioPrimitives;
    };

    struct BoneAnim_t
    {
        Joint_t* joint;

        Interpolator<float, glm::quat, GlmMixInterpolationMode<float, glm::quat>> rotation;
    };

    struct CharacterModel::Animation
    {
        unsigned int numBoneAnims;
        BoneAnim_t* boneAnims;
        List<StudioBoneAnim_t> studioBoneAnims;

        float timeScale, duration, elapsed;

        // Studio-specific
        String name;
        bool beingEdited;
    };

    static Float3 glmPitchYawRoll(const Float3& pitchYawRoll)
    {
        // OpenGL:  pitch=X yaw=Y roll=Z
        // zfw:     pitch=X yaw=Z roll=Y

        return Float3(pitchYawRoll.x, pitchYawRoll.z, pitchYawRoll.y) * f_pi / 180.0f;
    }

    static bool ParseJoint(cfx2::Node jointNode, Float3& pos, ErrorBuffer_t* g_eb)
    {
        if (Util::ParseVec(jointNode.getAttrib("pos"), &pos[0], 0, 3) <= 0)
            return ErrorBuffer::SetError(g_eb, EX_DOCUMENT_MALFORMED,
                    "desc", "Attribute 'pos' is mandatory for a Joint node",
                    "function", li_functionName,
                    nullptr),
                    false;

        return true;
    }

    static bool ParseCuboid(cfx2::Node cuboidNode, Float3& pos, Float3& size, Byte4& colour, const char*& bone, bool& collision, ErrorBuffer_t* g_eb)
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

    static void LineFromTo(ModelVertex v[2], const Float3& from, const Float3& to, Byte4 colour)
    {
        ModelVertex* vertex;
        
        vertex = &v[0];
        vertex->x = from.x;
        vertex->y = from.y;
        vertex->z = from.z;
        vertex->n[0] = 0;
        vertex->n[1] = 0;
        vertex->n[2] = 0;
        vertex->u = 0.0f;
        vertex->v = 0.0f;
        memcpy(&vertex->rgba[0], &colour, 4);

        vertex = &v[1];
        vertex->x = to.x;
        vertex->y = to.y;
        vertex->z = to.z;
        vertex->n[0] = 0;
        vertex->n[1] = 0;
        vertex->n[2] = 0;
        vertex->u = 0.0f;
        vertex->v = 0.0f;
        memcpy(&vertex->rgba[0], &colour, 4);
    }

    static bool CreateJoint(Joint_t* parent, cfx2::Node jointNode, Joint_t** joint_out, ErrorBuffer_t* g_eb, List<ModelVertex>& boneVertices)
    {
        Joint_t* joint;

        if (parent != nullptr)
        {
            Float3 pos;

            if (!ParseJoint(jointNode, pos, g_eb))
                return false;

            joint = new Joint_t;

            joint->name = jointNode.getText();
            joint->pos = pos;
            joint->diffpos = pos - parent->pos;
            joint->boneToParentSpace = glm::translate(glm::mat4x4(), joint->diffpos);
            joint->boneToModelSpace = parent->boneToModelSpace * joint->boneToParentSpace;
            joint->modelToBoneSpace = glm::inverse(joint->boneToModelSpace);

            joint->skeletonMeshFirst = boneVertices.getLength();

            parent->children.add(joint);

            ModelVertex line[2];
            LineFromTo(line, parent->pos, joint->pos, RGBA_WHITE);
            boneVertices.add(line[0]);
            boneVertices.add(line[1]);
        }
        else
        {
            joint = new Joint_t;

            joint->name = "_root";

            *joint_out = joint;
        }

        for (auto& i : jointNode)
            if (!CreateJoint(joint, i, nullptr, g_eb, boneVertices))
                return false;

        return true;
    }

    static void ResetRotation(Joint_t* joint, bool recursive)
    {
        joint->boneRotation = glm::quat();

        if (recursive)
            for (auto child : joint->children)
                ResetRotation(child, true);
    }

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

        p_vertices[3].x = corners[cornerC].x;
        p_vertices[3].y = corners[cornerC].y;
        p_vertices[3].z = corners[cornerC].z;
        p_vertices[3].n[0] = (int16_t)(normal.x * INT16_MAX);
        p_vertices[3].n[1] = (int16_t)(normal.y * INT16_MAX);
        p_vertices[3].n[2] = (int16_t)(normal.z * INT16_MAX);
        p_vertices[3].n[3] = 0;
        p_vertices[3].u = 1.0f;
        p_vertices[3].v = 1.0f;

        p_vertices[4].x = corners[cornerD].x;
        p_vertices[4].y = corners[cornerD].y;
        p_vertices[4].z = corners[cornerD].z;
        p_vertices[4].n[0] = (int16_t)(normal.x * INT16_MAX);
        p_vertices[4].n[1] = (int16_t)(normal.y * INT16_MAX);
        p_vertices[4].n[2] = (int16_t)(normal.z * INT16_MAX);
        p_vertices[4].n[3] = 0;
        p_vertices[4].u = 1.0f;
        p_vertices[4].v = 0.0f;

        p_vertices[5].x = corners[cornerA].x;
        p_vertices[5].y = corners[cornerA].y;
        p_vertices[5].z = corners[cornerA].z;
        p_vertices[5].n[0] = (int16_t)(normal.x * INT16_MAX);
        p_vertices[5].n[1] = (int16_t)(normal.y * INT16_MAX);
        p_vertices[5].n[2] = (int16_t)(normal.z * INT16_MAX);
        p_vertices[5].n[3] = 0;
        p_vertices[5].u = 0.0f;
        p_vertices[5].v = 0.0f;

        memcpy(&p_vertices[0].rgba[0], &colour, 4);
        memcpy(&p_vertices[1].rgba[0], &colour, 4);
        memcpy(&p_vertices[2].rgba[0], &colour, 4);
        memcpy(&p_vertices[3].rgba[0], &colour, 4);
        memcpy(&p_vertices[4].rgba[0], &colour, 4);
        memcpy(&p_vertices[5].rgba[0], &colour, 4);

        p_vertices += 6;
    }

    CharacterModel::CharacterModel(ErrorBuffer_t* eb, IResourceManager2* res)
    {
        this->eb = eb;
        this->res = res;

        vbRef = nullptr;
        vfRef = nullptr;
        materialRef = nullptr;
        skeletonMesh = nullptr;

        timeScale = 1.0f;

        highlightedJoint = nullptr;
        highlightTicks = 0;
        onAnimationEnding = nullptr;

        vertices = nullptr;
        forceSingleRefresh = false;
    }

    CharacterModel::~CharacterModel()
    {
        free(vertices);

        for (auto mesh : meshes)
        {
            //(*i)->formatRef->Release();
            //(*i)->gc->Release();
            delete mesh;
        }

        for (auto mesh : studioMeshes)
        {
            mesh->primitives.deleteAllItems();
            delete mesh;
        }

        /*zfw::Release(skeletonMesh);

        zfw::Release(materialRef);
        zfw::Release(vfRef);
        zfw::Release(vbRef);*/

        for (auto anim : animations)
        {
            for (unsigned int j = 0; j < anim->numBoneAnims; j++)
            {
                BoneAnim_t* boneAnim = &anim->boneAnims[j];

                li::destructPointer(boneAnim);
            }

            free(anim->boneAnims);
            delete anim;
        }
    }

    void CharacterModel::AddAnimation(cfx2::Node animNode)
    {
        ZFW_ASSERT(animNode.getText() != nullptr)

        Animation* anim = new Animation;
        anim->numBoneAnims = 0;
        anim->boneAnims = Allocator<BoneAnim_t>::allocate(animNode.getNumChildren());
        anim->duration = (float) animNode.getAttribInt("duration", 0);
        anim->elapsed = -1.0f;

        anim->name = animNode.getText();
        anim->beingEdited = false;

        for (auto boneAnimNode : animNode)
        {
            BoneAnim_t& boneAnim = anim->boneAnims[anim->numBoneAnims++];
            li::constructPointer(&boneAnim);

            boneAnim.joint = skeleton->Find(boneAnimNode.getName());
            ZFW_ASSERT(boneAnim.joint != nullptr)

            // studioBoneAnim
            StudioBoneAnim_t& studioBoneAnim = anim->studioBoneAnims.addEmpty();
            studioBoneAnim.index = anim->studioBoneAnims.getLength() - 1;
            studioBoneAnim.joint = boneAnim.joint;

            String copyFrom = boneAnimNode.getAttrib("copyFrom");

            if (!copyFrom.isEmpty())
            {
                intptr_t delim = copyFrom.findChar('/');

                ZFW_ASSERT(delim > 0)

                Animation* srcAnim = GetAnimationByName(copyFrom.subString(0, delim));
                zombie_assert(srcAnim);

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

                // studioBoneAnim
                studioBoneAnim.keyframes = srcAnim->studioBoneAnims[j].keyframes;
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

                        // studioBoneAnim
                        auto& studioKeyframe = studioBoneAnim.keyframes.addEmpty();
                        studioKeyframe.time = time;
                        studioKeyframe.pitchYawRoll = pitchYawRoll;
                    }
                }

                if (!rotationKeyframes.isEmpty())
                    boneAnim.rotation.Init(rotationKeyframes.c_array(), rotationKeyframes.getLength(), true);
            }
        }

        animations.add(anim);
    }

    bool CharacterModel::AddCuboid(MeshBuildContext_t* ctx, cfx2::Node cuboidNode)
    {
        static const size_t numFaces = 6;
        static const size_t verticesPerFace = 6;

        Float3 pos, size;
        Byte4 colour;
        const char* bone;
        bool collision;

        if (!ParseCuboid(cuboidNode, pos, size, colour, bone, collision, eb))
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
                g_sys->Log(kLogWarning, "CharacterModel: Ignoring invalid bone '%s' for primitive '%s'\n", bone, cuboidNode.getText());
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
        setUpCuboidSide( p_vertices, corners, Float3( 0.0f, 0.0f, -1.0f ), 1, 0, 3, 2, colour );

        setUpCuboidSide( p_vertices, corners, Float3( -1.0f, 0.0f, 0.0f ), 4, 0, 1, 5, colour );
        setUpCuboidSide( p_vertices, corners, Float3( 1.0f, 0.0f, 0.0f ), 6, 2, 3, 7, colour );

        setUpCuboidSide( p_vertices, corners, Float3( 0.0f, 1.0f, 0.0f ), 5, 1, 2, 6, colour );
        setUpCuboidSide( p_vertices, corners, Float3( 0.0f, -1.0f, 0.0f ), 7, 3, 0, 4, colour );

        // Studio-specific
        StudioPrimitive_t* cuboid = new StudioPrimitive_t;
        cuboid->type = StudioPrimitive_t::CUBOID;
        cuboid->a = pos;
        cuboid->b = size;
        cuboid->colour = colour;
        cuboid->bone = bone;
        cuboid->collision = collision;
        ctx->studioPrimitives.add(cuboid);

        return true;
    }

    void CharacterModel::AnimationTick()
    {
        //if (!anyRunningAnimations && !forceSingleRefresh)
        //    return;

        ResetRotation(skeleton.get(), true);

        iterate2 (i, activeAnims)
        {
            Animation* anim = i;

            bool staticAnim = anim->duration < 1.0e-3f || anim->beingEdited;

            for (unsigned int j = 0; j < anim->numBoneAnims; j++)
            {
                BoneAnim_t* boneAnim = &anim->boneAnims[j];
                Joint_t* joint = boneAnim->joint;

                ZFW_DBGASSERT(boneAnim->rotation.GetNumKeyframes() != 0)

                if (!staticAnim)
                    joint->boneRotation = joint->boneRotation * boneAnim->rotation.AdvanceTime<NO_CHECK_OVERFLOW>(timeScale);
                else
                    joint->boneRotation = joint->boneRotation * boneAnim->rotation.GetValue();

                joint->dirty = true;
            }

            if (staticAnim)
                continue;

            if (timeScale > 1.0e-3f)
            {
                anim->elapsed += timeScale;

                if (anim->elapsed + 1.0e-3f > anim->duration)
                {
                    if (onAnimationEnding != nullptr)
                        onAnimationEnding->OnAnimationEnding(this, anim);

                    anim->elapsed = -1.0f;
                    i.remove();
                    UpdateAnyRunningAnimations();
                    continue;
                }
            }
        }

        UpdateJoint(skeleton.get(), nullptr, false);
        UpdateVertices();
        UpdateCollisions();

        forceSingleRefresh = false;
    }

    bool CharacterModel::BuildMesh(cfx2::Node meshNode, Mesh*& mesh)
    {
        MeshBuildContext_t ctx;
        ctx.primitiveType = RK_TRIANGLES;

        for (auto child : meshNode)
        {
            if (strcmp(child.getName(), "Cuboid") == 0)
            {
                if (!AddCuboid(&ctx, child))
                    return false;
            }
        }

        mesh = new Mesh;
        mesh->gc = vbRef->CreateGeomChunk();
        mesh->primitiveType = ctx.primitiveType;
        mesh->formatRef = vfRef.get();
        mesh->offset = 0;
        mesh->count = (uint32_t)(ctx.vertexData.getSize() / VERTEX_SIZE);
        mesh->transform = glm::mat4x4();

        mesh->gc->AllocVertices(vfRef.get(), mesh->count, 0);
        mesh->gc->UpdateVertices(0, ctx.vertexData.getPtrUnsafe(), (size_t) ctx.vertexData.getSize());

        if (skeleton != nullptr)
            vertices = (ModelVertex*) ctx.vertexData.detachData();

        //
        auto sm = new StudioMesh_t;
        sm->name = meshNode.getText();
        sm->primitives.load(ctx.studioPrimitives.getPtr(), ctx.studioPrimitives.getLength());
        studioMeshes.add(sm);

        return true;
    }

    void CharacterModel::Draw(const glm::mat4x4& transform)
    {
        for (auto mesh : meshes)
            irm->DrawPrimitivesTransformed(materialRef.get(), mesh->primitiveType, mesh->gc, transform);
    }

    Joint_t* CharacterModel::FindJoint(const char* name)
    {
        return (skeleton != nullptr) ? skeleton->Find(name) : nullptr;
    }

    CharacterModel::Animation* CharacterModel::GetAnimationByName(const char* name)
    {
        for (auto anim : animations)
            if (anim->name == name)
                return anim;

        return nullptr;
    }

    Float3 CharacterModel::GetJointPos(Joint_t* joint)
    {
        return Float3(joint->boneToModelSpace * Float4(0.0f, 0.0f, 0.0f, 1.0f));
    }

    bool CharacterModel::Load(const char* path)
    {
        ZFW_ASSERT(meshes.isEmpty())

        program = irm->GetSharedResourceManager2()->GetResourceByPath<IShader>("RenderingKit/basicTextured", IResourceManager2::kResourceRequired);
        zombie_assert(program);

        vfRef = irm->CompileVertexFormat(program, sizeof(ModelVertex), modelVertexAttribs, false);
        vbRef = irm->CreateGeomBuffer(path);

        auto texture = res->GetResourceByPath<ITexture>("ntile/worldtex_0.png", IResourceManager2::kResourceRequired);

        if (texture == nullptr)
            return false;

        /*auto material = client::irm->CreateFPMaterial("plain", 0);
        material->SetNumTextures(1);
        material->SetTexture(0, std::move(texture));
        materialRef = material->GetMaterial();*/

        this->materialRef = irm->CreateMaterial("basicTextured", program);
        materialRef->SetTexture("tex", std::move(texture));

        anyRunningAnimations = false;


        String filename = (String) path + ".cfx2";

        unique_ptr<InputStream> is(g_sys->OpenInput(filename));

        if (is == nullptr)
            return ErrorBuffer::SetError3(EX_ASSET_OPEN_ERR, 2,
                                          "desc", (const char*) sprintf_t<255>("Failed to load model '%s'", path),
                                          "function", li_functionName),
                    false;

        String contents = is->readWhole();
        is.reset();

        cfx2_Node* docNode;

        int rc = cfx2_read_from_string(&docNode, contents, nullptr);

        if (rc < 0)
            return ErrorBuffer::SetError3(EX_ASSET_CORRUPTED, 2,
                                          "desc", (const char*) sprintf_t<255>("Failed to parse model '%s': %s", path, cfx2_get_error_desc(rc)),
                                          "function", li_functionName),
                    false;

        cfx2::Document doc(docNode);

        // Skeleton
        cfx2::Node skeletonNode = doc.findChild("Skeleton");

        if (!skeletonNode.isNull())
        {
            List<ModelVertex> boneVertices;

            Joint_t* root = nullptr;
            CreateJoint(nullptr, skeletonNode, &root, eb, boneVertices);
            skeleton.reset(root);

            skeletonMesh = vbRef->CreateGeomChunk();
            skeletonMesh->AllocVertices(vfRef.get(), boneVertices.getLength(), 0);
            skeletonMesh->UpdateVertices(0, (const uint8_t*) boneVertices.c_array(), boneVertices.getLength() * VERTEX_SIZE);
        }

        // Mesh
        cfx2::Node meshNode = doc.findChild("Mesh");

        ZFW_ASSERT(!meshNode.isNull());

        Mesh* mesh = nullptr;

        if (!BuildMesh(meshNode, mesh))
        {
            g_sys->Log(kLogError, "CharacterModel loading - model '%s'", path);
            return false;
        }

        meshes.add(mesh);

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

    bool CharacterModel::Save(zshared::MediaFile* modelFile)
    {
        unique_ptr<IOStream> modelMeshes(modelFile->OpenOrCreateSection("ntile.ModelMeshes"));

        for (auto sm : studioMeshes)
        {
            modelMeshes->writeString(sm->name);
            modelMeshes->writeLE<uint32_t>(sm->primitives.getLength());

            for (auto prim : sm->primitives)
            {
                modelMeshes->writeLE<int>(prim->type);

                if (prim->type == StudioPrimitive_t::CUBOID)
                {
                    modelMeshes->write(&prim->a, sizeof(prim->a));
                    modelMeshes->write(&prim->b, sizeof(prim->b));;
                    modelMeshes->write(&prim->colour, sizeof(prim->colour));
                    modelMeshes->writeString(prim->bone);
                    modelMeshes->writeLE<int>(prim->collision ? 1 : 0);
                }
            }
        }

        if (skeleton != nullptr)
        {
            unique_ptr<IOStream> modelSkeleton(modelFile->OpenOrCreateSection("ntile.ModelSkeleton"));

            WriteJoint(modelSkeleton.get(), skeleton.get());
        }

        unique_ptr<IOStream> modelAnimations(modelFile->OpenOrCreateSection("ntile.ModelAnimations"));

        for (auto anim : animations)
        {
            modelAnimations->writeString(anim->name);
            modelAnimations->write(&anim->duration, sizeof(anim->duration));
            modelAnimations->writeLE<uint32_t>(anim->numBoneAnims);

            for (size_t j = 0; j < anim->numBoneAnims; j++)
            {
                auto& studioBoneAnim = anim->studioBoneAnims[j];

                modelAnimations->writeString(studioBoneAnim.joint->name);

                modelAnimations->writeLE<uint32_t>(studioBoneAnim.keyframes.getLength());

                for (auto& keyframe : studioBoneAnim.keyframes)
                {
                    modelAnimations->write(&keyframe.time, sizeof(keyframe.time));
                    modelAnimations->write(&keyframe.pitchYawRoll, sizeof(keyframe.pitchYawRoll));
                }
            }
        }

        return true;
    }

    void CharacterModel::StartAnimation(Animation* anim)
    {
        if (anim->elapsed < -1.0e-3f)
        {
            activeAnims.add(anim);
            UpdateAnyRunningAnimations();
            forceSingleRefresh = true;
        }

        anim->elapsed = 0.0f;

        for (unsigned int i = 0; i < anim->numBoneAnims; i++)
        {
            BoneAnim_t* boneAnim = &anim->boneAnims[i];

            boneAnim->rotation.ResetTime();
        }
    }

    void CharacterModel::StopAllAnimations()
    {
        for (auto anim : activeAnims)
        {
            for (unsigned int j = 0; j < anim->numBoneAnims; j++)
            {
                BoneAnim_t* boneAnim = &anim->boneAnims[j];
                Joint_t* joint = boneAnim->joint;

                joint->boneRotation = glm::quat();
                joint->dirty = true;
            }

            anim->elapsed = -1.0f;
        }

        activeAnims.clear();
        anyRunningAnimations = false;

        UpdateJoint(skeleton.get(), nullptr, false);
        UpdateVertices();
        UpdateCollisions();
    }

    CharacterModel::Animation* CharacterModel::StudioAddAnimation(const char* name, float duration)
    {
        Animation* anim = new Animation;
        anim->numBoneAnims = 0;
        anim->boneAnims = nullptr;
        anim->duration = duration;
        anim->elapsed = -1.0f;

        anim->name = name;
        anim->beingEdited = false;

        animations.add(anim);
        return anim;
    }

    void CharacterModel::StudioAddKeyframeAtTime(Animation* anim, StudioBoneAnim_t* sa, float time, const StudioBoneAnimKeyframe_t* data)
    {
        size_t i;

        if (sa->keyframes.getLength() != 0 && time < sa->keyframes[0].time)
            i = 0;
        else
        {
            for (i = 1; i < sa->keyframes.getLength(); i++)
            {
                if (sa->keyframes[i - 1].time < time + 1.0e-3f && sa->keyframes[i].time > time - 1.0e-3f)
                    break;
            }
        }

        BoneAnim_t& boneAnim = anim->boneAnims[sa->index];

        auto keyframes = boneAnim.rotation.GetMutableKeyframes();

        ZFW_ASSERT (keyframes != nullptr)
        
        size_t oldCount = boneAnim.rotation.GetNumKeyframes();
        auto newKeyframes = Allocator<Interpolator<float, glm::quat>::Keyframe>::allocate(oldCount + 1);

        Allocator<Interpolator<float, glm::quat>::Keyframe>::move(newKeyframes, keyframes, i);
        li::constructPointer(&newKeyframes[i]);
        Allocator<Interpolator<float, glm::quat>::Keyframe>::move(newKeyframes + i + 1, keyframes + i, oldCount - i);

        boneAnim.rotation.Shutdown();
        boneAnim.rotation.Init(newKeyframes, oldCount + 1);

        auto& newStudioKey = sa->keyframes.insertEmpty(i);
        newStudioKey.time = time;
        newStudioKey.pitchYawRoll = data->pitchYawRoll;
        StudioUpdateKeyframe(anim, sa, i);

        boneAnim.joint->dirty = true;
    }

    void CharacterModel::StudioAnimationTick()
    {
        if (++highlightTicks == 50)
            highlightTicks = 0;

        if (highlightedJoint != nullptr && (highlightTicks % 25) == 0)
            highlightedJoint->dirty = true;
    }

    void CharacterModel::StudioDrawBones()
    {
        irm->DrawPrimitives(materialRef.get(), RK_LINES, skeletonMesh);
    }

    float CharacterModel::StudioGetAnimationDuration(Animation* anim)
    {
        return anim->duration;
    }

    const char* CharacterModel::StudioGetAnimationName(size_t index)
    {
        ;

        return (index < animations.getLength()) ? animations[index]->name.c_str() : nullptr;
    }

    const char* CharacterModel::StudioGetAnimationName(Animation* anim)
    {
        return anim->name;
    }

    StudioBoneAnim_t* CharacterModel::StudioGetBoneAnimation(Animation* anim, Joint_t* joint)
    {
        iterate2 (i, anim->studioBoneAnims)
            if ((*i).joint == joint)
                return &(*i);

        return nullptr;
    }

    void CharacterModel::StudioGetBoneAnimationAtTime(StudioBoneAnim_t* sa, float time, Float3& pitchYawRoll_out)
    {
        size_t i;

        if (sa->keyframes.getLength() == 0)
        {
            pitchYawRoll_out = Float3();
            return;
        }
        else if (sa->keyframes.getLength() == 1)
        {
            pitchYawRoll_out = sa->keyframes[0].pitchYawRoll;
            return;
        }

        for (i = 0; i < sa->keyframes.getLength() - 1; i++)
        {
            if (sa->keyframes[i].time < time + 1.0e-3f && sa->keyframes[i + 1].time > time - 1.0e-3f)
            {
                const float t = (time - sa->keyframes[i].time) / (sa->keyframes[i + 1].time - sa->keyframes[i].time);

                pitchYawRoll_out = (1.0f - t) * sa->keyframes[i].pitchYawRoll + t * sa->keyframes[i + 1].pitchYawRoll;
                return;
            }
        }

        pitchYawRoll_out = sa->keyframes[i].pitchYawRoll;
    }

    Joint_t* CharacterModel::StudioGetJoint(Joint_t* parent, size_t index)
    {
        if (parent == nullptr)
            return skeleton.get();
        else if (index < parent->children.getLength())
            return parent->children[index];
        else
            return nullptr;
    }

    StudioBoneAnimKeyframe_t* CharacterModel::StudioGetKeyframeByIndex(StudioBoneAnim_t* sa, int index)
    {
        return &sa->keyframes[index];
    }

    intptr_t CharacterModel::StudioGetKeyframeIndexNear(StudioBoneAnim_t* sa, float time, int direction)
    {
        size_t i;

        if (sa->keyframes.getLength() == 0)
            return -1;

        if (direction > 0)
        {
            for (i = 0; i < sa->keyframes.getLength(); i++)
                if (sa->keyframes[i].time > time + 1.0e-3f)
                    return i;

            return sa->keyframes.getLength() - 1;
        }
        else if (direction < 0)
        {
            for (i = sa->keyframes.getLength(); i > 0; i--)
                if (sa->keyframes[i - 1].time < time - 1.0e-3f)
                    return i - 1;

            return 0;
        }
        else
        {
            for (i = 0; i < sa->keyframes.getLength(); i++)
                if (fabs(sa->keyframes[i].time - time) < 1.0e-3f)
                    return i;

            return -1;
        }
    }

    bool CharacterModel::StudioGetPrimitivesList(size_t meshIndex, StudioPrimitive_t**& list_out, size_t& count_out)
    {
        if (studioMeshes.getLength() < 1)
            return false;

        list_out = studioMeshes[0]->primitives.getPtr();
        count_out = studioMeshes[0]->primitives.getLength();
        return true;
    }

    void CharacterModel::StudioRemoveKeyframe(Animation* anim, StudioBoneAnim_t* sa, int index)
    {
        BoneAnim_t& boneAnim = anim->boneAnims[sa->index];

        auto keyframes = boneAnim.rotation.GetMutableKeyframes();

        ZFW_ASSERT (keyframes != nullptr)
        
        size_t oldCount = boneAnim.rotation.GetNumKeyframes();
        auto newKeyframes = Allocator<Interpolator<float, glm::quat>::Keyframe>::allocate(oldCount - 1);

        Allocator<Interpolator<float, glm::quat>::Keyframe>::move(newKeyframes, keyframes, index);
        Allocator<Interpolator<float, glm::quat>::Keyframe>::move(newKeyframes + index, keyframes + index + 1, oldCount - index - 1);

        boneAnim.rotation.Shutdown();
        boneAnim.rotation.Init(newKeyframes, oldCount - 1);
        boneAnim.rotation.SetTime<NO_CHECK_OVERFLOW>(anim->elapsed);

        sa->keyframes.remove(index);

        boneAnim.joint->dirty = true;
    }

    void CharacterModel::StudioSetAnimationTime(Animation* anim, float time)
    {
        anim->elapsed = time;

        for (unsigned int j = 0; j < anim->numBoneAnims; j++)
        {
            BoneAnim_t* boneAnim = &anim->boneAnims[j];

            boneAnim->rotation.SetTime<NO_CHECK_OVERFLOW>(time);
        }
    }

    void CharacterModel::StudioSetHighlightedJoint(Joint_t* joint)
    {
        if (highlightedJoint == joint)
            return;

        if (highlightedJoint != nullptr)
        {
            highlightedJoint->highlighted = false;
            highlightedJoint->dirty = true;
        }
        else
            highlightTicks = 0;

        highlightedJoint = joint;

        if (joint != nullptr)
        {
            joint->highlighted = true;
            joint->dirty = true;
        }
    }

    void CharacterModel::StudioStartAnimationEditing(Animation* anim)
    {
        if (anim->elapsed < -1.0e-3f)
        {
            activeAnims.add(anim);
            UpdateAnyRunningAnimations();
            forceSingleRefresh = true;
        }

        anim->elapsed = 0.0f;
        anim->beingEdited = true;

        for (unsigned int i = 0; i < anim->numBoneAnims; i++)
        {
            BoneAnim_t* boneAnim = &anim->boneAnims[i];

            boneAnim->rotation.ResetTime();
        }
    }

    void CharacterModel::StudioStopAnimationEditing(Animation* anim)
    {
        anim->elapsed = -1.0f;
        anim->beingEdited = false;

        activeAnims.removeItem(anim);
    }

    void CharacterModel::StudioUpdateKeyframe(Animation* anim, StudioBoneAnim_t* sa, int index)
    {
        BoneAnim_t& boneAnim = anim->boneAnims[sa->index];

        auto keyframes = boneAnim.rotation.GetMutableKeyframes();

        ZFW_ASSERT (keyframes != nullptr)
        {
            keyframes[index].time = sa->keyframes[index].time;
            keyframes[index].value = glm::quat(glmPitchYawRoll(sa->keyframes[index].pitchYawRoll));
        }

        boneAnim.rotation.ResetTime();
        boneAnim.rotation.SetTime<NO_CHECK_OVERFLOW>(anim->elapsed);

        boneAnim.joint->dirty = true;
    }

    void CharacterModel::UpdateAnyRunningAnimations()
    {
        this->anyRunningAnimations = false;

        for (auto anim : activeAnims)
            if (anim->duration > 0)
            {
                this->anyRunningAnimations = true;
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

            ModelVertex line[2];
            LineFromTo(line, Float3(parent->boneToModelSpace * Float4(0.0f, 0.0f, 0.0f, 1.0f)),
                    Float3(joint->boneToModelSpace * Float4(0.0f, 0.0f, 0.0f, 1.0f)), RGBA_WHITE);
            skeletonMesh->UpdateVertices(joint->skeletonMeshFirst, (const uint8_t*) line, sizeof(ModelVertex) * 2);
        }

        for (auto child : joint->children)
            UpdateJoint(child, joint, dirty);

        joint->wasDirty = dirty;
        joint->dirty = false;
    }

    void CharacterModel::UpdateJointRange(const JointRange_t& range)
    {
        ModelVertex* vertices_out = (ModelVertex*) alloca(range.count * VERTEX_SIZE);

        ModelVertex* p_vertex = vertices_out;

        for (uint32_t i = range.first; i < range.first + range.count; i++)
        {
            const Float4 pos = range.joint->boneToModelSpace * Float4(vertices[i].x, vertices[i].y, vertices[i].z, 1.0f);
            const Float4 normal = range.joint->boneToModelSpace * (Float4(vertices[i].n[0], vertices[i].n[1], vertices[i].n[2], 0.0f) * (1.0f / INT16_MAX));

            p_vertex->x = pos.x;
            p_vertex->y = pos.y;
            p_vertex->z = pos.z;
            p_vertex->n[0] = (int16_t)(normal.x * INT16_MAX);
            p_vertex->n[1] = (int16_t)(normal.y * INT16_MAX);
            p_vertex->n[2] = (int16_t)(normal.z * INT16_MAX);

            if (!range.joint->highlighted || highlightTicks < 25)
                memcpy(&p_vertex->rgba, &vertices[i].rgba, VERTEX_SIZE - 20);
            else
            {
                p_vertex->rgba[0] = 255;
                p_vertex->rgba[1] = 255;
                p_vertex->rgba[2] = 255;
                p_vertex->rgba[3] = 255;
            }

            p_vertex++;
        }

        // TODO: Fix Me?
        meshes[0]->gc->UpdateVertices(range.first, (const uint8_t*) vertices_out, range.count * VERTEX_SIZE);
    }

    void CharacterModel::UpdateVertices()
    {
        for (auto& range : jointRanges)
            if (range.joint->wasDirty)
                UpdateJointRange(range);
    }

    void CharacterModel::WriteJoint(OutputStream* os, Joint_t* joint)
    {
        os->writeString(joint->name);
        os->write(&joint->pos, sizeof(joint->pos));

        os->writeLE<uint32_t>(joint->children.getLength());

        for (auto child : joint->children)
            WriteJoint(os, child);
    }
}
