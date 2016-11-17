#pragma once

#include <framework/datamodel.hpp>

#include <algorithm>
#include <ctime>

#define sprintf_15      (const char*) zfw::sprintf_t<15>
#define sprintf_63      (const char*) zfw::sprintf_t<63>
#define sprintf_255     (const char*) zfw::sprintf_t<255>
#define sprintf_4095    (const char*) zfw::sprintf_t<4095>

namespace zfw
{
    enum
    {
        ALIGN_LEFT = 0,
        ALIGN_HCENTER = 1,
        ALIGN_RIGHT = 2,

        ALIGN_TOP = 0,
        ALIGN_VCENTER = 4,
        ALIGN_BOTTOM = 8,

        // RenderingKit::IFontFace needs this for layoutFlags
        ALIGN_MAX = 16
    };

    struct Flag
    {
        const char* name;
        int value;
    };

    class Util
    {
        public:
            enum { TIMESTAMP_LENGTH = 19 };

            static Int2         Align(Int2 pos, Int2 size, int alignment);
            static bool         Align(Int2& topLeftOut, const Int2& topLeft, const Int2& bottomRight, const Int2& size,
                    int alignment);
            static const char*  Md5String(const char *string);

            // Recognized formats: #RGB, #RRGGBB, #RRGGBBAA (1 character = 1 hex digit)
            static bool         ParseBool(const char* stringOrNull);
            static bool         ParseColour(const char* string, Byte4& colour_out);
            static bool         ParseColourHex(const char* chars, size_t length, Byte4& colour_out);
            static int          ParseEnum(const char* string, int& value, const Flag* flags);
            static int          ParseFlagVec(const char* string, int& value, const Flag* flags);
            static int          ParseInt(const char* stringOrNull, int def = -1);
            static int          ParseIntVec(const char* string, int* vec, size_t minCount, size_t maxCount);
            static int          ParseVec(const char* string,
                    int(*callback)(const char* value, const char** end_value, void *user), void *user);
            static int          ParseVec(const char* string, float* vec, size_t minCount, size_t maxCount);

            static const char*  Format(const Int2& vec);
            static const char*  Format(const Float2& vec);
            static const char*  Format(const Float3& vec);
            static const char*  Format(const Float4& vec);
            static const char*  FormatColour(Byte4 colour);
            static const char*  FormatColourHRRGGBB(Byte4 colour);
            static const char*  FormatTimestamp(const time_t* t);

            static Byte3        HSLtoRGB(float h, float s, float l);

            static void*        MemDup(const void* data, size_t length);
            static char*        StrDup(const char* str);
            static bool         StrEmpty(const char* str) { return str == nullptr || *str == 0; }

            template <typename Struct>
            static Struct*      StructDup(const Struct* data, size_t count)
            {
                return reinterpret_cast<Struct*>(Util::MemDup(data, count * sizeof(Struct)));
            }
    };

    int vasprintf(char **buf, size_t *size, const char *format, va_list arg);
    int asprintf(char ** buf, const char * format, ...);
    int asprintf2(char ** buf, size_t* bufSize, const char * format, ...);

    template <size_t length>
    class sprintf_t
    {
        protected:
            char buffer[length + 1];

        private:
            sprintf_t(const sprintf_t&);

        public:
            sprintf_t(const char* format, ...)
            {
                va_list args;

                va_start(args, format);
                int res = vsnprintf(buffer, length + 1, format, args);
                va_end(args);

                if (res > 0)
                    buffer[std::min<size_t>(res, length)] = 0;
                else
                    buffer[0] = 0;
            }

            operator const char* () const
            {
                return buffer;
            }
    };

    template <typename BitField>
    BitField SetFlagInBitField(BitField flags, int flag, bool set)
    {
        return set ? (flags | flag) : (flags & ~flag);
    }
}
