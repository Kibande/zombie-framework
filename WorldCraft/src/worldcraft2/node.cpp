
#include "editorview.hpp"
#include "node.hpp"
#include "worldcraftpro.hpp"

#include <framework/colorconstants.hpp>
#include <framework/engine.hpp>
#include <framework/entityhandler.hpp>
#include <framework/resourcemanager.hpp>

#define GET_PROPERTY(name_) properties.add(NodeProperty(#name_, name_));
#define SET_PROPERTY(name_) if (strcmp(property.name, #name_) == 0) {\
        property.GetValue(name_);\
        return; }

namespace worldcraftpro
{
    void NodeProperty::GetValue(Float3& value) const
    {
        if (type == T_Float3)
            value = v_Float3;
        else if (type == T_String)
            Util::ParseVec(v_String, &value[0], 0, 3);
    }

    void NodeProperty::GetValue(li::String& value) const
    {
        if (type == T_String)
            value.set(v_String);
        else if (type == T_Float3)
            value.set(Util::Format(v_Float3));
    }

    Node::Node(World* world, const char* className, const char* name)
        : world(world), className(className), name(name)
    {
    }

    void Node::Load(cfx2::Node node, int flags)
    {
        cfx2_Node* n = node.node;

        for (size_t i = 0; i < cfx2_list_length(n->attributes); i++)
        {
            cfx2_Attrib* attrib = &cfx2_item(n->attributes, i, cfx2_Attrib);

            SetProperty(NodeProperty(attrib->name, attrib->value));
        }
    }

    bool Node::OnAfterPicking(uint32_t winner)
    {
        if (winner == pickingId)
        {
            world->SetSelection(this);
            return true;
        }

        return false;
    }

    cfx2::Node Node::Serialize(cfx2::Node parent)
    {
        List<NodeProperty> properties;
        GetProperties(properties);

        cfx2::Node node = parent.createChild(className, name);

        iterate2(i, properties)
        {
            const NodeProperty& property = i;

            String value;
            property.GetValue(value);

            node.setAttrib(property.name, value);
        }

        return node;
    }

    BrushNode::BrushNode(World* world, const char* name)
        : Node(world, "Brush", name)
    {
        texture = nullptr;
    }

    void BrushNode::Init(const Float3& pos, const Float3& size, const char *textureName)
    {
        this->pos = pos;
        this->size = size;
        this->textureName = textureName;

        AcquireResources();
    }

    BrushNode::~BrushNode()
    {
        DropResources();
    }

    void BrushNode::AcquireResources()
    {
        texture = world->GetBrushTexture(textureName);
    }

    void BrushNode::DropResources()
    {
        texture.reset();
    }

    void BrushNode::Draw(RenderState_t& rs, int flags)
    {
        if (flags & kNodeDrawPicking)
        {
            pickingId = rs.picking.SetPickingIndexNext();

            rs.bp.DrawFilledCuboid(pos, size, RGBA_WHITE);
        }
        else
        {
            if (!(flags & kNodeDrawWireframe))
                rs.tp.DrawFilledCuboid(texture.get(), pos, size, RGBA_WHITE);
            else
                rs.bp.DrawCuboidWireframe(pos, size, RGBA_COLOUR(255, 0, 0));
        }
    }

    bool BrushNode::GetBoundingBox(Float3& pos, Float3& size)
    {
        pos = this->pos;
        size = this->size;
        return true;
    }

    void BrushNode::GetProperties(List<NodeProperty>& properties)
    {
        GET_PROPERTY(pos)
        GET_PROPERTY(size)
        GET_PROPERTY(textureName)
    }

    void BrushNode::SetProperty(const NodeProperty& property)
    {
        SET_PROPERTY(pos)
        SET_PROPERTY(size)
        SET_PROPERTY(textureName)
    }

    EntityNode::EntityNode(World* world, const char* name)
            : Node(world, "StaticModel", name)
    {
        ent = nullptr;
    }
    
    EntityNode::~EntityNode()
    {
        DropResources();
    }
    
    void EntityNode::Init(const char* entdef, const Float3& pos)
    {
        this->pos = pos;
        this->entdef = entdef;

        AcquireResources();

        ent->SetPos(pos);
    }

    void EntityNode::AcquireResources()
    {
        //ent = Entity::LoadFromFile("entdef/" + entdef + ".cfx2", BASEENT_APPEARANCE_GRAPHICS, true);
        //ent = Entity::Create(entdef, true);

        auto ieh = g_sys->GetEntityHandler(true);
        ent.reset(ieh->InstantiateEntity(entdef, ENTITY_REQUIRED));

        //FIXME1
        //Error handling
    }
    
    void EntityNode::Draw(RenderState_t& rs, int flags)
    {
        //FIXME1
        /*
        if (!(flags & DRAW_PICKING))
        {
            ent->Draw();
        }
        else
        {
            pickingId = zr::Renderer::GetPickingId();

            zr::Renderer::SetPickingId(pickingId);
            ent->DrawSolid();
        }
        */
    }
    
    void EntityNode::DropResources()
    {
    }
    
    bool EntityNode::GetBoundingBox(Float3& pos, Float3& size)
    {
        pos = this->pos - Float3(0.5f, 0.5f, 0.5f);
        size = Float3(1.0f, 1.0f, 1.0f);

        return true;
    }
    
    void EntityNode::GetProperties(List<NodeProperty> &properties)
    {
        GET_PROPERTY(entdef)
        GET_PROPERTY(pos)
    }
    
    void EntityNode::SetProperty(const NodeProperty &property)
    {
        SET_PROPERTY(entdef)
        SET_PROPERTY(pos)

        ent->SetPos(pos);
    }

    GroupNode::GroupNode(World* world, const char* name)
        : Node(world, "Group", name)
    {
    }

    GroupNode::~GroupNode()
    {
        iterate2 (i, children)
            delete i;
    }

    void GroupNode::Add(Node* node)
    {
        children.add(node);
    }

    Node* GroupNode::CreateNode(const String& type, const char* name)
    {
        if (type == "Brush")
            return new BrushNode(world, name);
        else if (type == "Group")
            return new GroupNode(world, name);
        else
        {
            g_sys->Printf(kLogError, "worldcraftpro: Unknown node class '%s'\n", name);
            return nullptr;
        }
    }

    void GroupNode::Draw(RenderState_t& rs, int flags)
    {
        iterate2 (i, children)
            i->Draw(rs, flags);
    }

    void GroupNode::Load(cfx2::Node node, int flags)
    {
        for (auto i : node)
        {
            Node* n = CreateNode(i.getName(), i.getText());

            if (n != nullptr)
            {
                n->Load(i, flags);

                if (!(flags & kNodeLoadOffline))
                    n->AcquireResources();

                children.add(n);
            }
        }
    }

    bool GroupNode::OnAfterPicking(uint32_t winner)
    {
        iterate2 (i, children)
            if (i->OnAfterPicking(winner))
                return true;

        return false;
    }

    bool GroupNode::SearchNode(INodeSearch* search)
    {
        iterate2 (i, children)
            if(i->SearchNode(search))
                return true;

        return false;
    }

    cfx2::Node GroupNode::Serialize(cfx2::Node parent)
    {
        cfx2::Node node = Node::Serialize(parent);

        iterate2 (i, children)
            i->Serialize(node);

        return node;
    }

    StaticModelNode::StaticModelNode(World* world, const char* name)
            : Node(world, "StaticModel", name)
    {
        model = nullptr;
    }
    
    StaticModelNode::~StaticModelNode()
    {
        DropResources();
    }
    
    void StaticModelNode::Init(const Float3& pos, const char *fileName)
    {
        this->pos = pos;
        this->fileName = fileName;

        AcquireResources();
    }

    void StaticModelNode::AcquireResources()
    {
        // FIXME: try_load
        //texture = Loader::LoadTexture(textureName);

        /*media::DIMesh test;
        media::PresetFactory::CreateCube(Float3(1, 1, 1), Float3(0.5, 0.5, 0.0), media::ALL_SIDES, &test);
        zr::Mesh* mesh = zr::Mesh::CreateFromMemory(&test);
        model = zr::Model::CreateFromMeshes(&mesh, 1);*/

        //FIXME1
        //model = zr::Model::LoadFromFile(fileName, true);
    }
    
    void StaticModelNode::Draw(RenderState_t& rs, int flags)
    {
        //FIXME1
        /*
        if (!(flags & DRAW_PICKING))
        {
            model->Render(glm::translate(glm::mat4x4(), pos), 0);
        }
        else
        {
            pickingId = zr::Renderer::GetPickingId();
            
            zr::Renderer::SetPickingId(pickingId);
            model->Render(glm::translate(glm::mat4x4(), pos), zr::MODEL_DISABLE_MATERIALS);
        }
        */
    }
    
    void StaticModelNode::DropResources()
    {
    }
    
    bool StaticModelNode::GetBoundingBox(Float3& pos, Float3& size)
    {
        Float3 min, max;
        //FIXME1
        //model->GetBoundingBox(min, max);

        pos = pos + min;
        size = max - min;

        return true;
    }
    
    void StaticModelNode::GetProperties(List<NodeProperty> &properties)
    {
        GET_PROPERTY(fileName)
    }
    
    void StaticModelNode::SetProperty(const NodeProperty &property)
    {
        SET_PROPERTY(fileName)
    }

    WorldNode::WorldNode(World* world)
        : GroupNode(world, nullptr)
    {
    }
}
