#ifndef framework_components_drawable_hpp
#define framework_components_drawable_hpp

#include <framework/base.hpp>

#include <string>

namespace zfw
{
    struct Drawable
    {
        std::string modelPath;

        static IComponentType& GetType();
    };
}

#endif
