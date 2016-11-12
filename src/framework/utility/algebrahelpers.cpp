
#include <framework/utility/algebrahelpers.hpp>

namespace zfw
{
    Float3 AlgebraHelpers::TransformVec(const Float4& v, const glm::mat4x4& m)
    {
        const float w_scale = 1.0f / (m[0][3] * v.x + m[1][3] * v.y + m[2][3] * v.z + m[3][3] * v.w);

        return Float3(
                m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z + m[3][0] * v.w,
                m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1] * v.w,
                m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2] * v.w
        ) * w_scale;
    }

    bool AlgebraHelpers::TransformVec(const Float4& v, const glm::mat4x4& m, Float3& v_out)
    {
        const float w = m[0][3] * v.x + m[1][3] * v.y + m[2][3] * v.z + m[3][3] * v.w;

        if (fabs(w) < 10e-5f)
            return false;

        v_out = Float3(
                m[0][0] * v.x + m[1][0] * v.y + m[2][0] * v.z + m[3][0] * v.w,
                m[0][1] * v.x + m[1][1] * v.y + m[2][1] * v.z + m[3][1] * v.w,
                m[0][2] * v.x + m[1][2] * v.y + m[2][2] * v.z + m[3][2] * v.w
        ) * (1.0f / w);

        return true;
    }
}
