
#include "client.hpp"

namespace client
{
    static const CharacterSkin player_skin[] = {
        RGBA_COLOUR(213, 186, 94),
        RGBA_COLOUR(86, 39, 12),
        { RGBA_COLOUR(0, 73, 183), RGBA_COLOUR(0, 102, 255) },
        { RGBA_COLOUR(50, 100, 150), RGBA_COLOUR(0, 0, 0) },
        RGBA_COLOUR(0, 0, 0)
    };
    
    static const CharacterSkin zombie_skin[] = {
        RGBA_COLOUR(0, 186, 94),
        RGBA_COLOUR(12, 86, 39),
        { RGBA_COLOUR(0, 183, 73), RGBA_COLOUR(0, 255, 102) },
        { RGBA_COLOUR(50, 150, 100), RGBA_COLOUR(0, 0, 0) },
        RGBA_COLOUR(0, 0, 0)
    };
    
    character_base::character_base(CFloat3& pos)
            : armAnimPhase(0.0f), legAnimPhase(0.0f), eyeDir(0.0f), gravity(0.0f)
    {
        this->pos = pos;
        
        for (size_t i = 0; i < MAX_WEAPONS; i++)
            weapons[i] = nullptr;
        
        SetCollisionHandler(g_collHandler);

        //name = "???";
    }

    bool character_base::CanJump()
    {
        CFloat3 newPos = pos + Float3(0.0f, 1.0f, 0.0f);

        if (collHandler == nullptr || collHandler->CHCanMove(this, pos, newPos))
            return false;
        else
            return true;
    }

    void character_base::Draw(Name name)
    {
        if (name == NO_NAME)
        {
            CFloat3 origin(pos.x, pos.y - (22 + 19 + 4), pos.z);
            
            zr::R::SetTexture(nullptr);
            
            // Head
            zr::R::DrawBox(origin + Float3(-9, -20, -4), CFloat3(18, 20, 8), skin.head_colour);
            
            // Body
            zr::R::DrawBox(origin + Float3(-5, 0, -2), Float3(10, 22, 4), skin.body_colour);
            
            // Legs
            const float leg_angle_max = 30.0f * f_pi / 180.0f;
            const float leg_angle = sin(legAnimPhase) * leg_angle_max;
            
            zr::R::DrawRectRotated(origin + Float3(0.0f, 25.0f, 5.0f), Float2(6, 19), Float2(3, 3), skin.leg_colour[0],
                                   leg_angle);
            zr::R::DrawRectRotated(origin + Float3(0.0f, 25.0f, 5.0f), Float2(10, 4), Float2(3, -16), RGBA_BLACK_A(255),
                                   leg_angle);
            
            zr::R::DrawRectRotated(origin + Float3(0.0f, 25.0f, -5.0f), Float2(6, 19), Float2(3, 3), skin.leg_colour[1],
                                   -leg_angle);
            zr::R::DrawRectRotated(origin + Float3(0.0f, 25.0f, -5.0f), Float2(10, 4), Float2(3, -16), RGBA_BLACK_A(255),
                                   -leg_angle);
            
            // Arms
            const float arm_angle_max = 15.0f * f_pi / 180.0f;
            const float arm_angle = (armAnimPhase < 0.0f) ? (f_pi / 2) : (sin(armAnimPhase) * arm_angle_max);

            zr::R::DrawRectRotated(Float3(origin.x, origin.y + 5.0f, 6.0f), Float2(6, 20), Float2(3.0f, 3.0f), skin.arm_colour[0],
                                  arm_angle);
        }
        else if (name == DRAW_HUD_LAYER)
        {
            zr::R::EnableDepthTest(false);
            g_headsUpFont->DrawAsciiString(this->name, Int2(pos.x, pos.y - 80), RGBA_WHITE, ALIGN_CENTER | ALIGN_MIDDLE);
        }
    }
    
    bool character_base::GetBoundingBox(Float3& min, Float3& max)
    {
        min = Float3(pos.x - 9, pos.y - (22 + 19 + 4 + 20), pos.z - 5.0f);
        max = Float3(pos.x + 9, pos.y, pos.z + 5.0f);
        return true;
    }
    
    void character_base::GiveWeapon(weapon_base* weapon)
    {
        for (size_t i = 0; i < MAX_WEAPONS; i++)
            if (weapons[i] == nullptr)
            {
                weapons[i] = weapon;
                return;
            }
    }

    void character_base::OnFrame(double delta)
    {
        ActorEntity::OnFrame(delta);

        gravity += glm::min(delta, 0.1) * 500.0f;
        CFloat3 newPos = pos + Float3(0.0f, gravity * delta, 0.0f);

        if (collHandler == nullptr || collHandler->CHCanMove(this, pos, newPos))
            pos = newPos;
        else
            gravity = 0.0f;
    }

    void character_base::SetMovement(bool enable, Name movementType, CFloat3& direction)
    {
        if (movementType == ACTOR_MOVEMENT_WALK)
            speed.x = enable ? (direction.x * 100.0f) : 0.0f;
        else if (movementType == ACTOR_MOVEMENT_JUMP && enable && CanJump())
            gravity = -250.0f;
    }
    
    SERIALIZE_PARTIAL_BEGIN(character_base)
        PointEntity::Serialize(world, output, flags);
        output->writeString(name);
        return (output->writeItems(&armAnimPhase, 3) == 3);
    SERIALIZE_END
    
    UNSERIALIZE_BEGIN(character_base)
        PointEntity::Unserialize(world, input, flags);
        name = input->readString();
        return (input->readItems(&armAnimPhase, 3) == 3);
    UNSERIALIZE_END

    character_player::character_player(CFloat3& pos)
            : character_base(pos)
    {
        name = Var::GetStr("g_playername", true);
        memcpy(&skin, &player_skin, sizeof(CharacterSkin));
    }

    void character_player::OnFrame(double delta)
    {
        character_base::OnFrame(delta);
        
        legAnimPhase += delta * f_pi * 2.0f;
        armAnimPhase += delta * f_pi * 2.0f;
        
        while (legAnimPhase > f_pi * 2.0f)
            legAnimPhase -= f_pi * 2.0f;
        
        while (armAnimPhase > f_pi * 2.0f)
            armAnimPhase -= f_pi * 2.0f;
    }
    
    SERIALIZE_BEGIN(character_player, "character_player")
        return character_base::Serialize(world, output, flags);
    SERIALIZE_END
    
    UNSERIALIZE_BEGIN(character_player)
        return character_base::Unserialize(world, input, flags);
    UNSERIALIZE_LINK
        return -1;
    UNSERIALIZE_END
    
    character_zombie::character_zombie(CFloat3& pos)
            : character_base(pos)
    {
        memcpy(&skin, &zombie_skin, sizeof(CharacterSkin));
        
        armAnimPhase = -1.0f;
    }
    
    SERIALIZE_BEGIN(character_zombie, "character_zombie")
        return character_base::Serialize(world, output, flags);
    SERIALIZE_END
    
    UNSERIALIZE_BEGIN(character_zombie)
        return character_base::Unserialize(world, input, flags);
    UNSERIALIZE_LINK
        return -1;
    UNSERIALIZE_END
}