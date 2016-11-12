
#include "private.hpp"

#include <framework/entity.hpp>
#include <framework/entityhandler.hpp>
#include <framework/framework.hpp>

#include <littl/cfx2.hpp>
#include <littl/HashMap.hpp>

namespace zfw
{
    using namespace li;

    class EntityHandler : public IEntityHandler
    {
        protected:
            struct EntityDef_t
            {
                IEntity*    (*Instantiate)();
                cfx2::Node  properties;
            };

            ErrorBuffer_t* eb;
            ISystem* sys;
            HashMap<String, EntityDef_t> registeredEntities;

        public:
            EntityHandler(ErrorBuffer_t* eb, ISystem* sys) : eb(eb), sys(sys) {}
            virtual ~EntityHandler();

            virtual bool Register(const char* className, IEntity* (*Instantiate)()) override;
            virtual bool RegisterExternal(const char* path, int flags) override;

            virtual IEntity* InstantiateEntity(const char* className, int flags) override;
            virtual int IterateEntityClasses(IEntityClassListener* listener) override;

            bool RegisterEntityClass(const char* className, IEntity* (*Instantiate)(), cfx2::Node properties);
    };

    IEntityHandler* p_CreateEntityHandler(ErrorBuffer_t* eb, ISystem* sys)
    {
        return new EntityHandler(eb, sys);
    }

    EntityHandler::~EntityHandler()
    {
        iterate2 (i, registeredEntities)
        {
            if (!(*i).value.properties.isNull())
                (*i).value.properties.release();
        }
    }

    IEntity* EntityHandler::InstantiateEntity(const char* className, int flags)
    {
        EntityDef_t* entdef = registeredEntities.find(className);

        if (entdef == nullptr)
            return ((flags & ENTITY_REQUIRED) && ErrorBuffer::SetError(eb, EX_OBJECT_UNDEFINED,
                    "desc", (const char*) sprintf_t<255>("Failed to create undefined entity '%s'.", className),
                    "function", li_functionName,
                    nullptr)),
                    nullptr;

        IEntity* ent = entdef->Instantiate();

        if (!entdef->properties.isNull())
            ent->ApplyProperties(entdef->properties.node);

        return ent;
    }

    int EntityHandler::IterateEntityClasses(IEntityClassListener* listener)
    {
        iterate2 (i, registeredEntities)
        {
            int ret = listener->OnEntityClass((*i).key);

            if (ret <= 0)
                return ret;
        }

        return 0;
    }

    bool EntityHandler::Register(const char* className, IEntity* (*Instantiate)())
    {
        return RegisterEntityClass(className, Instantiate, cfx2::Node());
    }

    bool EntityHandler::RegisterEntityClass(const char* className, IEntity* (*Instantiate)(), cfx2::Node properties)
    {
        if (registeredEntities.find(className) != nullptr)
            sys->Printf(kLogWarning, "Entity: redefinition of '%s'", className);

        EntityDef_t entdef;
        entdef.Instantiate = Instantiate;
        entdef.properties = properties;
        registeredEntities.set(String(className), (EntityDef_t&&) entdef);

        return true;
    }

    bool EntityHandler::RegisterExternal(const char* path, int flags)
    {
        String filename = (String) path + ".cfx2";

        unique_ptr<InputStream> is(sys->OpenInput(filename));
        
        if (is == nullptr)
            return ErrorBuffer::SetError(eb, EX_ASSET_OPEN_ERR,
                    "desc", (const char*) sprintf_t<255>("Failed to load entity definition '%s'", path),
                    "function", li_functionName,
                    nullptr),
                    false;

        String contents = is->readWhole();
        is.reset();

        cfx2_Node* docNode;

#if libcfx2_version >= 0x0090
        int rc = cfx2_read_from_string(&docNode, contents, nullptr);
#else
        int rc = cfx2_read_from_string(contents, &docNode);
#endif

        if (rc != cfx2_ok)
            return ErrorBuffer::SetError(eb, EX_ASSET_OPEN_ERR,
                    "desc", (const char*) sprintf_t<1023>("Parse error in entity definition '%s': %s", path, cfx2_get_error_desc(rc)),
                    "function", li_functionName,
                    nullptr),
                    false;

        cfx2::Document doc(docNode);

        ZFW_ASSERT(doc.getNumChildren() == 1)

        cfx2::Node node = doc[0];
        const char* baseEntity = node.getText();
        
        EntityDef_t* entdef = registeredEntities.find(baseEntity);

        if (entdef == nullptr)
        {
            sys->Printf(kLogError, "Entity: undefined baseent '%s' in '%s'", baseEntity, filename.c_str());

            return ErrorBuffer::SetError(eb, EX_OBJECT_UNDEFINED,
                    "desc", (const char*) sprintf_t<511>("Failed to load entity definition '%s'.", path),
                    "function", li_functionName,
                    "filename", filename.c_str(),
                    "baseent", baseEntity,
                    nullptr),
                    false;
        }

        doc.removeChild(node);

        return RegisterEntityClass(node.getName(), entdef->Instantiate, node);
    }
}
