
#include "entities.hpp"
#include "../ntile.hpp"

#include <framework/resourcemanager2.hpp>

namespace ntile
{
namespace entities
{
    prop_base::prop_base()
    {
        this->name = "prop_base";

        model = nullptr;

        movementListener = nullptr;
    }

    int prop_base::ApplyProperties(cfx2_Node* properties_in)
    {
        cfx2::Node properties(properties_in);

        modelPath = properties.getAttrib("model");

        if (properties.getName() != nullptr)
        {
            name_buffer = properties.getName();
            this->name = name_buffer.c_str();
        }

        return 1;
    }

    bool prop_base::Init()
    {
        if (!modelPath.isEmpty())
            g_res->ResourceByPath(&model, modelPath);

        return true;
    }

    void prop_base::EditingModeInit()
    {
        if (modelPath.isEmpty())
        {
            static const char* p_editorImagePath = "ntile/ui_gfx/editor_prop_base.png";

            g_res->ResourceByPath(&editorImage, p_editorImagePath);
        }
    }

    void prop_base::Draw(const UUID_t* uuidOrNull)
    {
        if (uuidOrNull != nullptr && uuidOrNull != &DRAW_ENT_PICKING && uuidOrNull != &DRAW_EDITOR_MODE)
            return;

        if (!modelPath.isEmpty())
        {
            ir->PushTransform(glm::translate(glm::mat4x4(), pos));
            model->Draw();
            ir->PopTransform();
        }
        else
        {
            ir->PushTransform(glm::translate(glm::mat4x4(), pos));
            ir->DrawTextureBillboard(editorImage, Float3(), Float2(16.0f, 16.0f));
            ir->PopTransform();
        }
    }

    bool prop_base::GetAABB(Float3& min, Float3& max)
    {
        ZFW_ASSERT(model != nullptr)

        /*min = pos + model->GetAABBMin();
        max = pos + model->GetAABBMax();
        return true;*/
        return false;
    }

    bool prop_base::GetAABBForPos(const Float3& newPos, Float3& min, Float3& max)
    {
        ZFW_ASSERT(model != nullptr)

        /*min = newPos + model->GetAABBMin();
        max = newPos + model->GetAABBMax();
        return true;*/
        return false;
    }

    size_t prop_base::GetNumProperties()
    {
        return 1;
    }

    bool prop_base::GetRWProperties(NamedValuePtr* buffer, size_t count)
    {
        if (count != 1)
            return false;

        SetNamedValue(buffer[0], "pos", T_Float3, &pos);
        return true;
    }

    void prop_base::SetPos(const Float3& pos)
    {
        if (movementListener)
            movementListener->OnSetPos(this, this->pos, pos);

        this->pos = pos;
    }

    SERIALIZE_BEGIN_2(prop_base)
        if (output->write(&pos, sizeof(pos)) != sizeof(pos))
            return 0;
        return 1;
    SERIALIZE_END
    
    UNSERIALIZE_BEGIN(prop_base)
        if (input->read(&pos, sizeof(pos)) != sizeof(pos))
            return 0;
        return 1;
    UNSERIALIZE_LINK
        return -1;
    UNSERIALIZE_END
}
}
