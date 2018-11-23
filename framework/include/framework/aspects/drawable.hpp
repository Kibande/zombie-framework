#ifndef framework_drawable_hpp
#define framework_drawable_hpp

#include <framework/base.hpp>

#include <string>

namespace zfw
{
    struct Drawable
    {
        std::string modelPath;

        static IAspectType& GetType();
    };
}

#endif
