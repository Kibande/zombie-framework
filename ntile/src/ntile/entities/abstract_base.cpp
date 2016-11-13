
#include "entities.hpp"
#include "../ntile.hpp"

#include <framework/resourcemanager2.hpp>

namespace ntile
{
namespace entities
{
    abstract_base::abstract_base()
    {
        this->name = "abstract_base";

        editorImage = nullptr;
    }

    int abstract_base::ApplyProperties(cfx2_Node* properties_in)
    {
        cfx2::Node properties(properties_in);

        editorImagePath = properties.getAttrib("editorImage");

        if (properties.getName() != nullptr)
        {
            name_buffer = properties.getName();
            this->name = name_buffer.c_str();
        }

        return 1;
    }

    bool abstract_base::Init()
    {
        return true;
    }

    void abstract_base::EditingModeInit()
    {
        const char* p_editorImagePath;

        if (!editorImagePath.isEmpty())
            p_editorImagePath = editorImagePath.c_str();
        else
            p_editorImagePath = "ntile/ui_gfx/editor_abstract_base.png";

        // FIXME: Error severity management
        g_res->ResourceByPath(&editorImage, p_editorImagePath);

        //if ((editorImage = ir->LoadTexture(p_editorImagePath, TEXTURE_DEFAULT_FLAGS, 0)) == nullptr)
        //    g_sys->Printf(kLogError,"abstract_base: failed to load editor graphics '%s'", p_editorImagePath);
    }

    void abstract_base::Draw(const UUID_t* uuidOrNull)
    {
        if (uuidOrNull != &DRAW_ENT_PICKING && uuidOrNull != &DRAW_EDITOR_MODE)
            return;

        if (editorImage != nullptr)
        {
            ir->PushTransform(glm::translate(glm::mat4x4(), pos));
            ir->DrawTextureBillboard(editorImage, Float3(), Float2(16.0f, 16.0f));
            ir->PopTransform();
        }
    }

    size_t abstract_base::GetNumProperties()
    {
        return 0;
    }

    bool abstract_base::GetRWProperties(NamedValuePtr* buffer, size_t count)
    {
        return false;
    }

    SERIALIZE_BEGIN_2(abstract_base)
        if (output->write(&pos, sizeof(pos)) != sizeof(pos))
            return 0;
        return 1;
    SERIALIZE_END
    
    UNSERIALIZE_BEGIN(abstract_base)
        if (input->read(&pos, sizeof(pos)) != sizeof(pos))
            return 0;
        return 1;
    UNSERIALIZE_LINK
        return -1;
    UNSERIALIZE_END
}
}
