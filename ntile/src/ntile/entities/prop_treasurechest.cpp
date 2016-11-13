
#include "entities.hpp"
#include "../ntile.hpp"

namespace ntile
{
namespace entities
{
    prop_treasurechest::prop_treasurechest()
    {
        this->name = "prop_treasurechest";
        this->modelPath = "ntile/models/prop_treasurechest";
    }

    void prop_treasurechest::OnClicked(IGameScreen* gs, int button)
    {
    }

    SERIALIZE_BEGIN(prop_treasurechest, "prop_treasurechest")
        if (output->write(&pos, sizeof(pos)) != sizeof(pos))
            return 0;
        return 1;
    SERIALIZE_END
    
    UNSERIALIZE_BEGIN(prop_treasurechest)
        if (input->read(&pos, sizeof(pos)) != sizeof(pos))
            return 0;
        return 1;
    UNSERIALIZE_LINK
        return -1;
    UNSERIALIZE_END
}
}
