
#include "entities.hpp"
#include "../ntile.hpp"

namespace ntile
{
namespace entities
{
    char_base::char_base(Int3& pos, float angle) : angle(angle)
    {
        this->pos = pos;
    }

    /*void char_base::Draw(const UUID_t* uuidOrNull)
    {
        if (uuidOrNull != nullptr && uuidOrNull != &DRAW_ENT_PICKING)
            return;

        ZFW_ASSERT(model != nullptr)
        modelView = glm::rotate(glm::translate(glm::mat4x4(), pos), -angle, Float3(0.0f, 0.0f, 1.0f));
        
        ir->PushTransform(modelView);
        model->Draw();
        ir->PopTransform();
    }*/

    bool char_base::GetAABB(Float3& min, Float3& max)
    {
        /*ZFW_ASSERT(model != nullptr)

        min = pos + model->GetAABBMin();
        max = pos + model->GetAABBMax();
        return true;*/
        return false;
    }

    bool char_base::GetAABBForPos(const Float3& newPos, Float3& min, Float3& max)
    {
        /*ZFW_ASSERT(model != nullptr)

        min = newPos + model->GetAABBMin();
        max = newPos + model->GetAABBMax();
        return true;*/
        return false;
    }

    void char_base::SetPos(const Float3& pos)
    {
        if (movementListener)
            movementListener->OnSetPos(this, this->pos, pos);

        this->pos = pos;
    }
}
}
