#ifndef RenderingKit_Model_hpp
#define RenderingKit_Model_hpp

#include <framework/datamodel.hpp>
#include <framework/resource.hpp>

namespace RenderingKit
{
    class IModel : public zfw::IResource2
    {
    public:
        virtual ~IModel() {}

        virtual void Draw(const glm::mat4x4& transform) = 0;
    };
}

#endif
