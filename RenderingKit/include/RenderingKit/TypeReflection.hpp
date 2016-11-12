#pragma once

#include "RenderingKit.hpp"

namespace RenderingKit
{
    template <typename Type>
    class TypeReflection
    {
    };

    template<>
    class TypeReflection<float>
    {
        public:
            static const RKAttribDataType_t attribDataType = RK_ATTRIB_FLOAT;

            enum { zero = 0 };
            enum { one = 1 };
    };

    template<>
    class TypeReflection<zfw::Byte4>
    {
        public:
            static const RKAttribDataType_t attribDataType = RK_ATTRIB_UBYTE_4;
    };

    template<>
    class TypeReflection<zfw::Short2>
    {
        public:
            static const RKAttribDataType_t attribDataType = RK_ATTRIB_SHORT_2;
    };

    template<>
    class TypeReflection<zfw::Float2>
    {
        public:
            static const RKAttribDataType_t attribDataType = RK_ATTRIB_FLOAT_2;
    };

    template<>
    class TypeReflection<zfw::Float3>
    {
        public:
            static const RKAttribDataType_t attribDataType = RK_ATTRIB_FLOAT_3;
    };

    template<>
    class TypeReflection<zfw::Float4>
    {
        public:
            static const RKAttribDataType_t attribDataType = RK_ATTRIB_FLOAT_4;
    };
}
