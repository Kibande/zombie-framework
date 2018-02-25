
#include <framework/entityhandler.hpp>
#include <framework/entityworld.hpp>
#include <framework/system.hpp>

#include <littl/cfx2.hpp>

namespace zfw
{
    using namespace li;

    bool EntityWorld::AddEntity(shared_ptr<IEntity> ent)
    {
        iterate2 (i, entityFilters)
            if (!i->OnAddEntity(this, ent.get()))
                return false;

        ent->SetID((int) entities.getLength());
        entities.add(move(ent));
        return true;
    }

    void EntityWorld::AddEntityFilter(IEntityFilter* filter)
    {
        entityFilters.add(filter);
    }
    
    void EntityWorld::Draw(const UUID_t* modeOrNull)
    {
        iterate2 (i, entities)
            i->Draw(modeOrNull);
    }
    
    IEntity* EntityWorld::GetEntityByID(int entID)
    {
        iterate2 (i, entities)
            if (i->GetID() == entID)
                return (*i).get();

        return nullptr;
    }

	bool EntityWorld::InitAllEntities()
	{
		for (auto ent : entities) {
			if (!ent->Init())
				return false;
		}

		return true;
	}

#if ZOMBIE_API_VERSION >= 201601
    void EntityWorld::IterateEntities(std::function<void(IEntity* ent)> visit)
    {
        for (auto ent : entities)
            visit(ent.get());
    }
#endif

    void EntityWorld::OnFrame(double delta)
    {
        iterate2 (i, entities)
            i->OnFrame(delta);
    }

    void EntityWorld::OnTick()
    {
        iterate2 (i, entities)
            i->OnTick();
    }

    void EntityWorld::RemoveAllEntities(bool destroy)
    {
        entities.clear();
    }

    void EntityWorld::RemoveEntity(IEntity* ent)
    {
        for (size_t i = 0; i < entities.getLength(); i++)
        {
            if (entities[i].get() == ent)
            {
                entities.remove(i);

                iterate2 (i, entityFilters)
                    i->OnRemoveEntity(this, ent);

                break;
            }
        }
    }

    void EntityWorld::RemoveEntityFilter(IEntityFilter* filter)
    {
        entityFilters.removeItem(filter);
    }

    bool EntityWorld::Serialize(OutputStream* output, int flags)
    {
        output->writeLE<uint8_t>(0x10);

        iterate2 (i, entities)
        {
            int r = i->FullSerialize(this, output, flags);
            
            if (r == 0)
            {
                // FIXME: better reporting when possible
                return ErrorBuffer::SetError3(EX_SERIALIZATION_ERR, 2,
                    "desc", sprintf_255("Failed to serialize entity #%i ('%s')", i->GetID(), i->GetName()),
                    "function", li_functionName
                    ), false;
            }
            else if (r > 0)
            {
                output->writeLE<uint8_t>(0xDD);
            }
        }
        
        output->writeLE<uint8_t>(0);
        return true;
    }

    bool EntityWorld::Unserialize(InputStream* input, int flags)
    {
        ZFW_ASSERT(entities.getLength() == 0)
        
        uint8_t v = 0;
        input->readLE<uint8_t>(&v);

        ZFW_ASSERT(v == 0x10)

        IEntityHandler* ieh = sys->GetEntityHandler(true);

        for (;;)
        {
            String entName = input->readString();

            if (entName.isEmpty())
                break;

            int32_t entID;
            input->readLE<int32_t>(&entID);
            
            shared_ptr<IEntity> ent(ieh->InstantiateEntity(entName, ENTITY_REQUIRED));

            if (ent == nullptr)
                return false;

            // FIXME: entIDs are broken. Should we fix now or wait until WorldHandler?
            //sys->Printf(kLogInfo, "World: Unserializing entity [%3i] %s", entID, entName.c_str());
            int r = ent->Unserialize(this, input, flags);
            
            if (r == 0)
                return ErrorBuffer::SetError3(EX_SERIALIZATION_ERR, 2,
                        "desc", sprintf_255("Failed to unserialize entity '%s'.", entName.c_str()),
                        "function", li_functionName
                        ), false;
            
            uint8_t marker;
            zombie_assert(input->readByte(&marker) && marker == 0xDD);
            
            ent->Init();
            AddEntity(ent);
            ent->SetID(entID);
        }
        
        // Link the entities now
        iterate2 (i, entities)
        {
            int r = i->Unserialize(this, nullptr, flags);
            
            if (r == 0)
                return ErrorBuffer::SetError3(EX_SERIALIZATION_ERR, 2,
                "desc", sprintf_255("Failed to link entity #%i.", i->GetID()),
                "function", li_functionName
                ), false;
        }

        return true;
    }

    void EntityWorld::WalkEntities(IEntityVisitor* visitor)
    {
        for (auto ent : entities)
            visitor->Visit(ent.get());
    }
}
