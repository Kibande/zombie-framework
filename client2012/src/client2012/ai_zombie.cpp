
#include "client.hpp"

namespace client
{
    ai_zombie::ai_zombie()
            : target(nullptr), ticksSinceLastRetarget(0)
    {
    }
    
    void ai_zombie::OnThink(int ticks)
    {
        // TODO: Implement zombie AI
        // TODO: Can use https://developer.valvesoftware.com/wiki/AI as reference

        if (ticksSinceLastRetarget > 100 || target == nullptr)
        {
            auto worldEntities = g_world->GetEntityList();
            
            iterate2 (i, worldEntities)
                if ((target = dynamic_cast<character_player*>(*i)) != nullptr)
                    break;
            
            ticksSinceLastRetarget = 0;
        }
        else
            ticksSinceLastRetarget += ticks;
        
        linked_ent->SetMovement(false, ACTOR_MOVEMENT_WALK, Float3());
        
        do {
            if (target == nullptr)
                break;
            
            auto entPos = linked_ent->GetPos();
            auto targetPos = target->GetPos();
            
            if (fabs(entPos.x - targetPos.x) < 20.0f)
                break;

            if (entPos.x < targetPos.x)
                linked_ent->SetMovement(true, ACTOR_MOVEMENT_WALK, Float3(1.0f, 0.0f, 0.0f));
            else
                linked_ent->SetMovement(true, ACTOR_MOVEMENT_WALK, Float3(-1.0f, 0.0f, 0.0f));

            if (entPos.y > targetPos.y + 30.0f)
                linked_ent->SetMovement(true, ACTOR_MOVEMENT_JUMP, Float3(0.0f, 0.0f, -1.0f));
        }
        while (false);
    }
    
    SERIALIZE_BEGIN(ai_zombie, "ai_zombie")
        linked_ent_id = linked_ent->GetID();
    
        return (output->writeItems(&linked_ent_id, 1) == 1);
    SERIALIZE_END
    
    UNSERIALIZE_BEGIN(ai_zombie)
        return (input->readItems(&linked_ent_id, 1) == 1);
    UNSERIALIZE_LINK
        linked_ent = dynamic_cast<character_zombie*>(world->GetEntityByID(linked_ent_id));
        return linked_ent != nullptr;
    UNSERIALIZE_END
}