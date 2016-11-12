
#include "sandbox.hpp"

namespace sandbox
{
    class IMapSelectionScene : public IScene
    {
        public:
            static shared_ptr<IMapSelectionScene> Create();
    };
}
