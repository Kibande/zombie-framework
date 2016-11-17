#pragma once

#include <framework/resource.hpp>

namespace RenderingKit
{
    class IWorldGeometry : public zfw::IResource2
    {
        public:
            virtual ~IWorldGeometry() {}

            virtual void Draw() = 0;
    };
}
