
#include <framework/datamodel.hpp>
#include <framework/framework.hpp>
#include <framework/utility/errorbuffer.hpp>

namespace zfw
{
    const char* DataModel::GetTypeName(ValueType type)
    {
        switch (type)
        {
            case T_undefined:       return "undefined";
            case T_Bool:            return "Bool";
            case T_Int:             return "Int";
            case T_Size:            return "Size";
            case T_Utf8:            return "UTF-8";
            case T_Int2:            return "Int2";
            case T_Int3:            return "Int3";
            case T_Int4:            return "Int4";
            case T_Float2:          return "Float2";
            case T_Float3:          return "Float3";
            case T_Float4:          return "Float4";
            default:                return "unknown";
        }
    }

    const char* DataModel::ToString(const NamedValuePtr& v)
    {
        switch (v.desc.type)
        {
            case T_Float3:
                return Util::Format(*reinterpret_cast<Float3*>(v.value));

            default:
                return ErrorBuffer::SetError3(EX_INVALID_ARGUMENT, 4,
                        "desc", "Internal Error: Unrecognized value type",
                        "function", li_functionName,
                        "valuename", v.desc.name,
                        "valuetype", (const char*) sprintf_t<15>("%i", v.desc.type)
                        ), nullptr;
        }
    }

    bool DataModel::UpdateFromString(NamedValuePtr& v, const char* value)
    {
        // FIXME: Value validation

        switch (v.desc.type)
        {
            case T_Float3:
                Util::ParseVec(value, reinterpret_cast<float*>(v.value), 3, 3);
                return true;

            default:
                return false;
        }
    }
}
