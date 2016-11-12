#pragma once

#include <framework/entity.hpp>

namespace zfw
{
    /*
    DECL_UNIQ(ACTOR_MOVEMENT_JUMP, 0x483724c8)
    DECL_UNIQ(ACTOR_MOVEMENT_WALK, 0x6215588b)
    */

    template <int dummy>
    class PointEntityBaseTpl;

    typedef PointEntityBaseTpl<0> PointEntityBase;

    template <int dummy>
    class PointEntityBaseTpl : public IEntity, public IPointEntity
    {
        protected:
            const char* name;
            int entID;

            Float3 pos, orientation, speed;

            shared_ptr<ICollisionHandler> collHandler;

            virtual void            UpdatePosFromSpeed();
            virtual void            UpdatePosFromSpeed(double delta);

        public:
            PointEntityBaseTpl() : name(nullptr), entID(-1) {}
            PointEntityBaseTpl(const Float3& pos, const Float3& orientation) : name(nullptr), entID(-1), pos(pos),
                    orientation(orientation) {}
            virtual ~PointEntityBaseTpl();
            DECL_PARTIAL_SERIALIZE_UNSERIALIZE()

            virtual IEntity*        GetEntity() override { return this; }

            virtual bool            GetAABB(Float3& min, Float3& max) override { return false; }
            virtual bool            GetAABBForPos(const Float3& newPos, Float3& min, Float3& max) override;
            virtual void            SetCollisionHandler(shared_ptr<ICollisionHandler> handler) override { collHandler = handler; }
            virtual int             GetID() const override { return entID; }
            virtual void            SetID(int entID) override { this->entID = entID; }
            virtual const char*     GetName() override { return name; }
            virtual const Float3&   GetPos() override { return pos; }
            virtual void            SetPos(const Float3& pos) override { this->pos = pos; }
            virtual const Float3&   GetOrientation() override { return orientation; }
            virtual void            SetOrientation(const Float3& orientation) override { this->orientation = orientation; }
            virtual const Float3&   GetSpeed() override { return speed; }
            virtual void            SetSpeed(const Float3& speed) override { this->speed = speed; }
    };

    template<int dummy>
    PointEntityBaseTpl<dummy>::~PointEntityBaseTpl()
    {
    }

    template <int dummy>
    bool PointEntityBaseTpl<dummy>::GetAABBForPos(const Float3& newPos, Float3& min, Float3& max)
    {
        if (GetAABB(min, max))
        {
            min += newPos - pos;
            max += newPos - pos;
            return true;
        }

        return false;
    }

    template <int dummy>
    void PointEntityBaseTpl<dummy>::UpdatePosFromSpeed()
    {
        Float3 newPos = pos + speed;

        if (collHandler == nullptr || collHandler->CollideMovementTo(this, pos, newPos))
            pos = newPos;

        ZFW_DBGASSERT(!_isnan(pos.x) && !_isnan(pos.y) && !_isnan(pos.z));
    }

    template <int dummy>
    void PointEntityBaseTpl<dummy>::UpdatePosFromSpeed(double delta)
    {
        Float3 newPos = pos + Float3(speed.x * delta, speed.y * delta, speed.z * delta);

        if (collHandler == nullptr || collHandler->CollideMovementTo(this, pos, newPos))
            pos = newPos;

        ZFW_DBGASSERT(!_isnan(pos.x) && !_isnan(pos.y) && !_isnan(pos.z));
    }

    template <int dummy>
    SERIALIZE_PARTIAL_BEGIN(PointEntityBaseTpl<dummy>)
        return (output->write(&orientation, sizeof(Float3) * 3) == sizeof(Float3) * 3);
    SERIALIZE_END
    
    template <int dummy>
    UNSERIALIZE_BEGIN(PointEntityBaseTpl<dummy>)
        return (input->read(&orientation, sizeof(Float3) * 3) == sizeof(Float3) * 3);
    UNSERIALIZE_LINK
        return -1;
    UNSERIALIZE_END

    /*class ActorEntity: extends(PointEntityBase)
    {
        public:
            virtual void SetMovement(bool enable, Uniq_t movementType, CFloat3& direction) = 0;
    };*/
}
