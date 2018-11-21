#ifndef RenderingKit_utility_VertexFormat2_hpp
#define RenderingKit_utility_VertexFormat2_hpp

#include "../RenderingKit.hpp"

#include <array>

namespace RenderingKit
{
    template<size_t N>
    struct VertexFormat {
        size_t vertexSizeInBytes;
        std::array<VertexAttrib_t, N> attribs;
        void* pimpl = nullptr;

        operator VertexFormatInfo () {
            return VertexFormatInfo { vertexSizeInBytes, &attribs[0], attribs.size(), &pimpl };
        }
    };
}

#endif