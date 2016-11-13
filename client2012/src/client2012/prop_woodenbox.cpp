#include "client.hpp"

namespace client
{
    prop_woodenbox::prop_woodenbox(CFloat3& pos)
    {
        this->pos = pos;
    }

    void prop_woodenbox::Draw(Name name)
    {
        if (name == NO_NAME)
        {
            zr::R::SetTexture(nullptr);
            
            zr::R::DrawBox(pos + Float3(-20.0f, -40.0f, -20.0f), Float3(40.0f, 40.0f, 40.0f), Float4(0.6f, 0.3f, 0.15f, 1.0f));
        }
    }

    bool prop_woodenbox::GetBoundingBox(Float3& min, Float3& max)
    {
        min = pos + Float3(-20.0f, -40.0f, -20.0f);
        max = pos + Float3(20.0f, 0.0f, 20.0f);
        return true;
    }

    SERIALIZE_BEGIN(prop_woodenbox, "prop_woodenbox")
        output->writeItems<Float3>(&pos, 1);
        return 1;
    SERIALIZE_END

    UNSERIALIZE_BEGIN(prop_woodenbox)
        return (input->readItems<Float3>(&pos, 1) == 1);
    UNSERIALIZE_LINK
        return -1;
    UNSERIALIZE_END
}