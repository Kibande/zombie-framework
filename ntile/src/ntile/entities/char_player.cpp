
#include "entities.hpp"
#include "../ntile.hpp"

#include <framework/resourcemanager2.hpp>

#define ENABLE_ANIMATION

namespace ntile
{
namespace entities
{
    char_player::char_player(Int3 pos, float angle) : char_base(pos, angle)
    {
        t = 0;
        lastAnim = 0;

        g_res->ResourceByPath(&model, "ntile/models/player");

#ifdef ENABLE_ANIMATION
        auto anim = model->GetAnimationByName("standing");

        if (anim != nullptr)
            model->StartAnimation(anim);

        sword_tip = model->FindJoint("sword_tip");
#endif
    }

    Float3 char_player::Hack_GetTorchLightPos()
    {
#ifdef ENABLE_ANIMATION
        return model->GetJointPos(sword_tip);
#else
        return Float3();
#endif
    }

    void char_player::Hack_ShieldAnim()
    {
#ifdef ENABLE_ANIMATION
        auto anim = model->GetAnimationByName("raise_shield");

        if (anim != nullptr)
            model->StartAnimation(anim);
#endif
    }

    void char_player::Hack_SlashAnim()
    {
#ifdef ENABLE_ANIMATION
        auto anim = model->GetAnimationByName("slash");

        if (anim != nullptr)
            model->StartAnimation(anim);
#endif
    }

    void char_player::OnTick()
    {
#ifdef ENABLE_ANIMATION
        model->AnimationTick();

        if (t != 0)
        {
            const Float3 newPos = pos + speed;

            SetPos(newPos);

            t--;
        }

        if (t == 0)
        {
            if (motionVec.x != 0 || motionVec.y != 0)
            {
                // Just to be sure
                motionVec.x = std::min(1, std::max(-1, motionVec.x));
                motionVec.y = std::min(1, std::max(-1, motionVec.y));

                // Update player angle
                angle = (float) atan2(-motionVec.y, motionVec.x);
                
                const float newX = pos.x + 16.0f * (float) motionVec.x;
                const float newY = pos.y + 16.0f * (float) motionVec.y;

                Float3 newPos = Float3(newX, newY, pos.z);

                if (collHandler != nullptr)
                {
                    if (!collHandler->CollideMovementTo(this, pos, newPos))
                    {
                        newPos = Float3(pos.x, newY, pos.z);

                        if (!collHandler->CollideMovementTo(this, pos, newPos))
                        {
                            newPos = Float3(newX, pos.y, pos.z);

                            if (!collHandler->CollideMovementTo(this, pos, newPos))
                                return;
                        }
                    }
                }

                // Move for 16 ticks
                speed = (newPos - pos) * (1.0f / 16);
                t = 16;

                auto anim = model->GetAnimationByName((lastAnim == 0) ? "step" : "step2");

                if (anim != nullptr)
                {
                    model->StartAnimation(anim);
                    lastAnim = 1 - lastAnim;
                }
            }
        }
#endif
    }
}
}
