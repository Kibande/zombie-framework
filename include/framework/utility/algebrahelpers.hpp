#pragma once

#include <framework/base.hpp>
#include <framework/datamodel.hpp>

namespace zfw
{
    class AlgebraHelpers
    {
        public:
            // Transform vector with w-divide
            // won't work for direction vector in perspective projection
            static Float3 TransformVec(const Float4& v, const glm::mat4x4& m);
            static bool TransformVec(const Float4& v, const glm::mat4x4& m, Float3& v_out);
    };
}
