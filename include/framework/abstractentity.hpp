#pragma once

#include <framework/entity.hpp>

namespace zfw
{
    template <int dummy>
    class AbstractEntityBaseTpl;

    typedef AbstractEntityBaseTpl<0> AbstractEntityBase;

    template <int dummy>
    class AbstractEntityBaseTpl : public IEntity
    {
        protected:
            const char* name;
            int entID;

            // defined even for Abstract entities for studio mode
            Float3 pos;

        public:
            AbstractEntityBaseTpl() : name(nullptr), entID(-1) {}
            virtual ~AbstractEntityBaseTpl() {}

            virtual int             GetID() const override { return entID; }
            virtual void            SetID(int entID) override { this->entID = entID; }
            virtual const char*     GetName() override { return name; }
            virtual const Float3&   GetPos() override { return pos; }
            virtual void            SetPos(const Float3& pos) override { this->pos = pos; }
    };

    /*
    class AIEntity : extends(AbstractEntityBase)
    {
        double ticksAccum;
        
        protected:
            AIEntity();
        
        public:
            virtual void OnFrame(double delta) override;
            virtual void OnThink(int ticks) = 0;
    };*/

    /*class control_1stperson : public AbstractEntity
    {
        public:
            enum { K_FWD, K_BACK, K_LEFT, K_RIGHT, K_MAX };

            struct Controls {
                Vkey_t keys[K_MAX];
            };

        protected:
            ActorEntity         *linked_ent;

            Controls ctl;
            float speed;

            int state[K_MAX];

        public:
            control_1stperson();
            static IEntity* Create() { return new control_1stperson; }

            virtual Controls& GetControls() { return ctl; }
            virtual bool LinkEntity(ActorEntity* ent);
            virtual int OnEvent(int h, const Event_t* ev) override;
            virtual void OnFrame(double delta) override;
            void SetSpeed(float speed) { this->speed = speed; }
    };*/
    
    /*class control_sideview : public AbstractEntity
    {
        public:
            enum { K_LEFT, K_RIGHT, K_JUMP, K_MAX };
            
            struct Controls {
                Vkey_t keys[K_MAX];
            };
            
        protected:
            ActorEntity     *linked_ent;
            Controls        ctl;
            
            int state[K_MAX];
            
        public:
            control_sideview();
            static IEntity* Create() { return new control_sideview; }
            virtual void Release() override { delete this; }
            
            virtual Controls& GetControls();
            virtual void LinkEntity(ActorEntity* ent);
            virtual int OnEvent(int h, const Event_t* ev) override;
    };*/

    /*AIEntity::AIEntity() : ticksAccum(0.0)
    {
    }
    
    void AIEntity::OnFrame(double delta)
    {
        ticksAccum += delta;
        const int wholeTicks = (int) (ticksAccum * 100.0);
        
        OnThink(wholeTicks);
        
        ticksAccum -= wholeTicks * 0.01;
    }*/

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
    
    /*control_sideview::control_sideview()
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
    }*/
}
