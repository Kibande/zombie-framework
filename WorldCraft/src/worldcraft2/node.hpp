#pragma once

#include <framework/entity.hpp>

#include <RenderingKit/RenderingKit.hpp>
#include <RenderingKit/utility/BasicPainter.hpp>
#include <RenderingKit/utility/Picking.hpp>
#include <RenderingKit/utility/TexturedPainter.hpp>

#include <littl/cfx2.hpp>

namespace worldcraftpro
{
    using namespace li;
    using namespace RenderingKit;
    using namespace zfw;

    enum { kNodeDrawPicking = 1, kNodeDrawWireframe = 2 };
    enum { kNodeLoadOffline = 1 };

    class Node;
    class World;

    class INodeSearch
    {
        public:
            virtual bool TestNode(Node* node) = 0;
    };

    struct NodeProperty
    {
        enum Type
        {
            T_int,
            T_Float3,
            T_String
        };

        const char*     name;
        Type            type;

        int             v_int;
        Float3          v_Float3;
        li::String      v_String;

        NodeProperty() {}
        NodeProperty(const char* name, const Float3& value) : name(name), type(T_Float3), v_Float3(value) {}
        NodeProperty(const char* name, const li::String& value) : name(name), type(T_String), v_String(value) {}

        void GetValue(Float3& value) const;
        void GetValue(li::String& value) const;
    };

    struct RenderState_t
    {
        BasicPainter3D<>&       bp;
        Picking<>&              picking;
        TexturedPainter3D<>&    tp;
    };

    class Node
    {
        protected:
            World* world;
            const char* className;
            String name;

            uint32_t pickingId;

        public:
            Node(World* world, const char* className, const char* name);
            virtual ~Node() {}

            virtual void AcquireResources() {}
            virtual void DropResources() {}

            virtual void Draw(RenderState_t& rs, int flags) {}
            virtual bool GetBoundingBox(Float3& pos, Float3& size) { return false; }
            virtual void GetProperties(List<NodeProperty>& properties) {}
            virtual void Load(cfx2::Node node, int flags);
            virtual bool OnAfterPicking(uint32_t winner);
            //virtual int OnEvent(int h, const Event_t& event) { return h; }
            virtual cfx2::Node Serialize(cfx2::Node parent);
            virtual bool SearchNode(INodeSearch* search) { return search->TestNode(this); }
            virtual void SetProperty(const NodeProperty& property) {}
    };

    class BrushNode : public Node
    {
        protected:
            // Static
            Float3 pos, size;
            li::String textureName;

            // Runtime
            shared_ptr<ITexture> texture;

        public:
            BrushNode(World* world, const char* name);
            virtual ~BrushNode();

            void Init(const Float3& pos, const Float3& size, const char *textureName);

            void AcquireResources() override;
            void DropResources() override;

            virtual void Draw(RenderState_t& rs, int flags) override;
            virtual bool GetBoundingBox(Float3& pos, Float3& size) override;
            virtual void GetProperties(List<NodeProperty>& properties) override;
            virtual void SetProperty(const NodeProperty& property) override;
    };

    class EntityNode : public Node
    {
        protected:
            // Static
            String entdef;
            Float3 pos;
        
            // Runtime
            unique_ptr<IEntity> ent;
        
        public:
            EntityNode(World* world, const char* name);
            virtual ~EntityNode();
            
            void Init(const char* entdef, const Float3& pos);
            
            virtual void AcquireResources() override;
            virtual void DropResources() override;
        
            virtual void Draw(RenderState_t& rs, int flags) override;
            virtual bool GetBoundingBox(Float3& pos, Float3& size) override;
            virtual void GetProperties(List<NodeProperty>& properties) override;
            virtual void SetProperty(const NodeProperty& property) override;

            IEntity* GetEnt() { return ent.get(); }
            const char* GetEntdef() { return entdef; }
    };

    class GroupNode : public Node
    {
        protected:
            li::List<Node*> children;

            Node* CreateNode(const String& type, const char* name);

        public:
            GroupNode(World* world, const char* name);
            virtual ~GroupNode();

            virtual void Draw(RenderState_t& rs, int flags) override;
            virtual void Load(cfx2::Node node, int flags) override;
            virtual bool OnAfterPicking(uint32_t winner) override;
            virtual bool SearchNode(INodeSearch* search) override;
            virtual cfx2::Node Serialize(cfx2::Node parent) override;

            void Add(Node* n);
    };

    class StaticModelNode : public Node
    {
        protected:
            // Static
            String fileName;
            Float3 pos;
        
            // Runtime
            shared_ptr<IGeomChunk> model;
        
        public:
            StaticModelNode(World* world, const char* name);
            virtual ~StaticModelNode();
            
            void Init(const Float3& pos, const char *fileName);
            
            virtual void AcquireResources() override;
            virtual void DropResources() override;
        
            virtual void Draw(RenderState_t& rs, int flags) override;
            virtual bool GetBoundingBox(Float3& pos, Float3& size) override;
            virtual void GetProperties(List<NodeProperty>& properties) override;
            virtual void SetProperty(const NodeProperty& property) override;
    };
    
    class WorldNode : public GroupNode
    {
        public:
            WorldNode(World* world);
    };
}
