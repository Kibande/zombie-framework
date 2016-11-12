
#include <framework/event.hpp>
#include <framework/utility/essentials.hpp>

#include <reflection/toolkit.hpp>

namespace zfw
{
    using namespace ::reflection;

    class VkeyReflectionTemplate
    {
        public:
            static bool fromString(IErrorHandler* err, const char* str, size_t strLen, Vkey_t& value_out) {
                if (!Vkey::ParseVkey(&value_out, str))
                    Vkey::ClearVkey(&value_out);
                return true;
            }

            static bool toString(IErrorHandler* err, char*& buf, size_t& bufSize, const Vkey_t& value) {
                const char* str = Vkey::FormatVkey(value);
                return bufStringSet(err, buf, bufSize, str, strlen(str));
            }
    };
}

namespace serialization
{
    template <>
    class Serializer<zfw::Vkey_t> {
        public:
            enum { TAG = TAG_NO_TYPE };

            static bool serialize(IErrorHandler* err, IWriter* writer, const zfw::Vkey_t& value) {
                zombie_assert(false);
                return false;
            }

            static bool deserialize(IErrorHandler* err, IReader* reader, zfw::Vkey_t& value_out) {
                zombie_assert(false);
                return false;
            }
    };
}

namespace reflection
{
    DEFINE_REFLECTION(VkeyReflection, zfw::Vkey_t, zfw::VkeyReflectionTemplate)
}
