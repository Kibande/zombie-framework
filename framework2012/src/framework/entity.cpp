
#include <framework/entity.hpp>
#include <framework/framework.hpp>
#include <framework/rendering.hpp>

#include <littl/cfx2.hpp>

namespace zfw
{
    DECLNAME(ACTOR_MOVEMENT_JUMP)
    DECLNAME(ACTOR_MOVEMENT_WALK)

    DECLNAME(DRAW_HUD_LAYER)
    DECLNAME(DRAW_MASK)
    DECLNAME(DRAW_STATIC)

    DECLNAME(IENTITY)
    DECLNAME(ABSTRACTENTITY)
    DECLNAME(POINTENTITY)

    struct Entdef {
        String entdef;
        IEntity* (* Create)();
    };

    static List<Entdef> entdefs;

    void* AbstractEntity::GetInterface(Name interfaceName)
    {
        if (interfaceName == ABSTRACTENTITY)
            return this;
        else
            return IEntity::GetInterface(interfaceName);
    }

    bool PointEntity::GetBoundingBoxForPos(CFloat3& newPos, Float3& min, Float3& max)
    {
        if (GetBoundingBox(min, max))
        {
            min += newPos - pos;
            max += newPos - pos;
            return true;
        }

        return false;
    }

    void* PointEntity::GetInterface(Name interfaceName)
    {
        if (interfaceName == POINTENTITY)
            return this;
        else
            return IEntity::GetInterface(interfaceName);
    }

    void PointEntity::OnFrame(double delta)
    {
        CFloat3 newPos = pos + Float3(speed.x * delta, speed.y * delta, speed.z * delta);

        if (collHandler == nullptr || collHandler->CHCanMove(this, pos, newPos))
            pos = newPos;
    }

    SERIALIZE_PARTIAL_BEGIN(PointEntity)
        return (output->writeItems(&orientation, 3) == 3);
    SERIALIZE_END
    
    UNSERIALIZE_BEGIN(PointEntity)
        return (input->readItems(&orientation, 3) == 3);
    UNSERIALIZE_LINK
        return -1;
    UNSERIALIZE_END
    
    AIEntity::AIEntity() : ticksAccum(0.0)
    {
    }
    
    void AIEntity::OnFrame(double delta)
    {
        ticksAccum += delta;
        const int wholeTicks = (int) (ticksAccum * 100.0);
        
        OnThink(wholeTicks);
        
        ticksAccum -= wholeTicks * 0.01;
    }

    /*control_1stperson::control_1stperson()
    {
        linked_ent = nullptr;
        speed = 1.0f;

        memset(&ctl, 0, sizeof(ctl));
        memset(&state, 0, sizeof(state));
    }

    void control_1stperson::LinkEntity(ActorEntity* ent)
    {
        linked_ent = ent;
    }

    int control_1stperson::OnEvent(int h, const Event_t* ev)
    {
        if (ev->type == EV_VKEY)
        {
            for (int j = 0; j < K_MAX; j++)
            {
                if (ev->vkey.vk.inclass == ctl.keys[j].inclass
                        && ev->vkey.vk.key == ctl.keys[j].key)
                {
                    if (ev->vkey.flags & VKEY_PRESSED)
                        state[j] = 1;
                    else if (ev->vkey.flags & VKEY_RELEASED)
                        state[j] = 0;

                    return 1;
                }
            }
        }

        return h;
    }

    void control_1stperson::OnFrame(double delta)
    {
        if (linked_ent == nullptr)
            return;

        const auto pos = linked_ent->GetPos();
        const auto orientation = linked_ent->GetOrientation();

        if (state[K_FWD])
            linked_ent->SetPos(pos + orientation * float(speed * delta));

        if (state[K_BACK])
            linked_ent->SetPos(pos - orientation * float(speed * delta));

        if (state[K_LEFT])
            linked_ent->SetPos(pos + Float3(-orientation.y, -orientation.x, 0.0f) * float(speed * delta));

        if (state[K_RIGHT])
            linked_ent->SetPos(pos + Float3(orientation.y, orientation.x, 0.0f) * float(speed * delta));
    }*/
    
    control_sideview::control_sideview()
    {
        linked_ent = nullptr;
        
        memset(&ctl, 0, sizeof(ctl));
        memset(&state, 0, sizeof(state));
    }
    
    control_sideview::Controls& control_sideview::GetControls()
    {
        return ctl;
    }

    void control_sideview::LinkEntity(ActorEntity* ent)
    {
        linked_ent = ent;
    }
    
    int control_sideview::OnEvent(int h, const Event_t* ev)
    {
        if (ev->type == EV_VKEY)
        {
            for (int j = 0; j < K_MAX; j++)
            {
                if (ev->vkey.vk.inclass == ctl.keys[j].inclass && ev->vkey.vk.key == ctl.keys[j].key)
                {
                    bool on = (ev->vkey.flags & VKEY_PRESSED);
                    
                    switch (j)
                    {
                        case K_LEFT:    linked_ent->SetMovement(on, ACTOR_MOVEMENT_WALK, Float3(-1.0f, 0.0f, 0.0f)); break;
                        case K_RIGHT:   linked_ent->SetMovement(on, ACTOR_MOVEMENT_WALK, Float3(1.0f, 0.0f, 0.0f)); break;
                        case K_JUMP:    linked_ent->SetMovement(on, ACTOR_MOVEMENT_JUMP, Float3(0.0f, -1.0f, 0.0f)); break;
                    }
                    
                    return 1;
                }
            }
        }
        
        return h;
    }

    /*static bool LoadEntityFromFile(IEntity* ent, const char* path, int preferredAppearance, bool required)
    {
        Reference<SeekableInputStream> input = Sys::OpenInput(path);

        if (input == nullptr)
        {
            if (required)
                Sys::RaiseException(EX_ASSET_OPEN_ERR, "Entity::LoadFromFile", "Failed to open '%s'.", path);

            return false;
        }

        cfx2::Document doc;
        doc.loadFrom(input);

        // FIXME: Error handling
        cfx2::Node entityNode = doc.findChild("Entity");

        ent->LoadFrom(entityNode);
        return true;
    }*/

    /*IEntity* Entity::LoadFromFile(const char* path, int preferredAppearance, bool required)
    {
        Object<BaseEntity> ent = new BaseEntity;

        if (!LoadEntityFromFile(ent, path, preferredAppearance, required))
            return nullptr;

        return ent.detach();
    }*/

    IEntity* Entity::Create(const char* entdef, bool required)
    {
        Object<IEntity> ent;

        iterate2 (i, entdefs)
            if ((*i).entdef == entdef)
            {
                ent = (*i).Create();
                break;
            }

        /*if (ent == nullptr)
            ent = new BaseEntity;

        if (!LoadEntityFromFile(ent, (String) "entdef/" + entdef + ".cfx2", -1, required))
            return false;*/

        if (ent == nullptr)
        {
            ZFW_ASSERT(!required)
            return ent.detach();
        }

        return ent.detach();
    }

    void Entity::Register(const char* entdef, IEntity* (* Create)())
    {
        // TODO: duplicates, hashtable

        Entdef def = { entdef, Create };
        entdefs.add(def);
    }
    
    World::~World()
    {
        iterate2(i, entities)
            delete i;
    }
    
    void World::AddEntity(IEntity *ent)
    {
        ent->SetID((int) entities.getLength());
        entities.add(ent);

        iterate2 (i, entityFilters)
            i->OnAddEntity(ent);
    }

    void World::AddEntityFilter(IEntityFilter* filter)
    {
        entityFilters.add(filter);
    }
    
    void World::Draw(Name name)
    {
        iterate2 (i, entities)
            i->Draw(name);
    }
    
    IEntity* World::GetEntityByID(int entID)
    {
        iterate2 (i, entities)
            if (i->GetID() == entID)
                return i;

        return nullptr;
    }
    
    void World::OnFrame(double delta)
    {
        iterate2 (i, entities)
            i->OnFrame(delta);
    }

    void World::RemoveAllEntities(bool destroy)
    {
        reverse_iterate2(i, entities)
            RemoveEntity(i, destroy);
    }

    void World::RemoveEntity(IEntity* ent, bool destroy)
    {
        if (entities.removeItem(ent))
        {
            iterate2 (i, entityFilters)
                i->OnRemoveEntity(ent);
        }

        if (destroy)
            delete ent;
    }

    void World::RemoveEntityFilter(IEntityFilter* filter)
    {
        entityFilters.removeItem(filter);
    }

    void World::Serialize(OutputStream* output, int flags)
    {
        output->write<uint8_t>(0x10);

        iterate2 (i, entities)
        {
            int r = i->FullSerialize(this, output, flags);
            
            if (r == 0)
            {
                // FIXME: better reporting when possible
                Sys::RaiseException(EX_SERIALIZATION_ERR, "World::Serialize", "failed to serialize entity #%i\n", i->GetID());
            }
            else if (r > 0)
            {
                output->write<uint8_t>(0xDD);
            }
        }
        
        output->write<uint8_t>(0);
    }

    void World::Unserialize(InputStream* input, int flags)
    {
        ZFW_ASSERT(entities.getLength() == 0)
        
        uint8_t v = input->read<uint8_t>();

        ZFW_ASSERT(v == 0x10)

        while (true)
        {
            String entName = input->readString();

            if (entName.isEmpty())
                break;

            int entID = input->read<int>();
            
            IEntity* ent = Entity::Create(entName, true);
            int r = ent->Unserialize(this, input, flags);
            
            if (r == 0)
            {
                Sys::RaiseException(EX_SERIALIZATION_ERR, "World::Serialize", "failed to unserialize entity '%s'\n", entName.c_str());
            }
            
            ZFW_ASSERT(input->read<uint8_t>() == 0xDD)
            
            AddEntity(ent);
            ent->SetID(entID);
        }
        
        // Link the entities now
        iterate2 (i, entities)
        {
            int r = i->Unserialize(this, nullptr, flags);
            
            if (r == 0)
                Sys::RaiseException(EX_SERIALIZATION_ERR, "World::Serialize", "failed to link entity #%i\n", i->GetID());
        }
    }
}
