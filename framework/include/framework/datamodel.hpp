#pragma once

#include <framework/base.hpp>

#pragma warning( push, 2 )
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#pragma warning( pop )

namespace zfw
{
    // data types enumeration
    enum ValueType
    {
        T_undefined = 0,

        // scalar types
        T_Bool,
        T_Int,
        T_Size,

        // variable-size types
        T_Utf8,

        // vector types
        T_Int2,             T_Int3,             T_Int4,
        T_Float2,           T_Float3,           T_Float4,

        T_MAX
    };

    // data types declaration
    typedef glm::ivec2 Int2;
    typedef glm::ivec3 Int3;
    typedef glm::ivec4 Int4;

    typedef glm::vec2 Float2;
    typedef glm::vec3 Float3;
    typedef glm::vec4 Float4;

    typedef glm::dvec2 Double2;
    typedef glm::dvec3 Double3;
    typedef glm::dvec4 Double4;

    typedef glm::tvec2<uint8_t, glm::defaultp> Byte2;
    typedef glm::tvec3<uint8_t, glm::defaultp> Byte3;
    typedef glm::tvec4<uint8_t, glm::defaultp> Byte4;

    typedef glm::tvec2<short, glm::defaultp> Short2;
    typedef glm::tvec3<short, glm::defaultp> Short3;
    typedef glm::tvec4<short, glm::defaultp> Short4;

    // data model structures
    struct NamedValueDesc
    {
        const char*     name;
        ValueType       type;
    };

    struct NamedValuePtr
    {
        NamedValueDesc  desc;
        void*           value;
    };

    class DataModel
    {
        public:
            static const char* GetTypeName(ValueType type);
            static const char* ToString(const NamedValuePtr& v);
            static bool UpdateFromString(NamedValuePtr& v, const char* value);
    };

    // helper functions
#define NamedValuePtrFrom_(typename_, type_)\
    static inline NamedValuePtr NamedValuePtrFrom(const char* name, type_& value)\
    {\
        NamedValuePtr p = { { name, typename_ }, reinterpret_cast<void*>(&value) };\
        return p;\
    }

    NamedValuePtrFrom_(T_Float3, Float3)

    static inline NamedValuePtr NamedValuePtrFrom(const char* name, char* value)
    {
        NamedValuePtr p = { { name, T_Utf8 }, value };
        return p;
    }

    static inline void SetNamedValueDesc(NamedValueDesc& v, const char* name, ValueType type)
    {
        v.name = name;
        v.type = type;
    }

    static inline void SetNamedValue(NamedValuePtr& v, const char* name, ValueType type, void* value)
    {
        SetNamedValueDesc(v.desc, name, type);
        v.value = value;
    }
}
