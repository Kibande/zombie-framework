#include "client.hpp"

namespace client
{
    void world_background::Draw(Name name)
    {
        if (name == NO_NAME)
        {
            zr::R::SetTexture(nullptr);
            
            zr::R::DrawFilledRect(Float3(-150, -10, -10), Float3(150, 0, -10), RGBA_COLOUR(60, 150, 30, 255));
            zr::R::DrawFilledRect(Float3(-150, -5, 10), Float3(150, 0, 10), RGBA_COLOUR(80, 200, 50, 255));

            zr::R::DrawBox(Float3(-150, 0, -20), Float3(300, 100, 40), Float4(0.4f, 0.2f, 0.1f, 1.0f));
        }
    }

    bool world_background::GetBoundingBox(Float3& min, Float3& max)
    {
        min = Float3(-300.0f, 0.0f, -20.0f);
        max = Float3(300.0f, 300.0f, 20.0f);
        return true;
    }

    SERIALIZE_BEGIN(world_background, "world_background")
        return 1;
    SERIALIZE_END

    UNSERIALIZE_BEGIN(world_background)
        return 1;
    UNSERIALIZE_LINK
        return -1;
    UNSERIALIZE_END
}