
#include <framework/utility/util.hpp>
#include <framework/utility/params.hpp>

extern "C" {
#include "../../zshared/md5.h"
}

#include <cctype>
#include <cerrno>
#include <cstdarg>
#include <ctime>

#pragma warning( disable : 4127 )

namespace zfw
{
    static char convBuffer[160];

    struct ParseIntVecStruct
    {
        int* vec;
        size_t maxCount;
    };

    static float getHueToRGBCoeff(float v1, float v2, float vH)
    {
        if ( vH < 0.0f )
            vH += 1.0f;

        if ( vH > 1.0f )
            vH -= 1.0f;

        if ( 6.0f * vH < 1.0f )
            return ( v1 + ( v2 - v1 ) * 6.0f * vH );

        if ( 2.0f * vH < 1.0f )
            return ( v2 );

        if ( 3.0f * vH < 2.0f )
            return ( v1 + ( v2 - v1 ) * ( ( 2.0f / 3.0f ) - vH ) * 6.0f );

        return v1;
    }

    static uint8_t HexDigit(char d)
    {
        if (d >= '0' && d <= '9')
            return d - '0';
        else if (d >= 'a' && d <= 'f')
            return 10 + d - 'a';
        else if (d >= 'A' && d <= 'F')
            return 10 + d - 'A';
        else
            return 0;
    }

    static int ParseIntVecCallback(const char* value, const char** end_value, void *user)
    {
        auto parseIntVec = (ParseIntVecStruct*) user;

        if (parseIntVec->maxCount == 0)
            return -1;

        parseIntVec->vec[0] = (int) strtol(value, (char**) end_value, 0);

        parseIntVec->vec++;
        parseIntVec->maxCount--;

        return 0;
    }

    Int2 Util::Align(Int2 pos, Int2 size, int alignment)
    {
        if (alignment & (ALIGN_HCENTER|ALIGN_RIGHT|ALIGN_VCENTER|ALIGN_BOTTOM))
        {
            if (alignment & ALIGN_HCENTER)
                pos.x -= size.x / 2;
            else if (alignment & ALIGN_RIGHT)
                pos.x -= size.x;

            if (alignment & ALIGN_VCENTER)
                pos.y -= size.y / 2;
            else if (alignment & ALIGN_BOTTOM)
                pos.y -= size.y;

            return pos;
        }
        else
            return pos;
    }

    bool Util::Align(Int2& topLeftOut, const Int2& topLeft, const Int2& bottomRight, const Int2& size, int alignment)
    {
        if (alignment & ALIGN_HCENTER)
            topLeftOut.x = topLeft.x + (bottomRight.x - topLeft.x - size.x) / 2;
        else if (alignment & ALIGN_RIGHT)
            topLeftOut.x = bottomRight.x - size.x;
        else
            topLeftOut.x = topLeft.x;

        if (alignment & ALIGN_VCENTER)
            topLeftOut.y = topLeft.y + (bottomRight.y - topLeft.y - size.y) / 2;
        else if (alignment & ALIGN_BOTTOM)
            topLeftOut.y = bottomRight.y - size.y;
        else
            topLeftOut.y = topLeft.y;

        return (bottomRight.x - topLeft.x < size.x && bottomRight.y - topLeft.y < size.y);
    }

    const char* Util::Format(const glm::ivec2& vec)
    {
        snprintf(convBuffer, li_lengthof(convBuffer), "%i, %i", vec.x, vec.y);
        return convBuffer;
    }

    const char* Util::Format(const glm::vec2& vec)
    {
        snprintf(convBuffer, li_lengthof(convBuffer), "%g, %g", vec.x, vec.y);
        return convBuffer;
    }

    const char* Util::Format(const glm::vec3& vec)
    {
        snprintf(convBuffer, li_lengthof(convBuffer), "%g, %g, %g", vec.x, vec.y, vec.z);
        return convBuffer;
    }

    const char* Util::Format(const glm::vec4& vec)
    {
        snprintf(convBuffer, li_lengthof(convBuffer), "%g, %g, %g, %g", vec.x, vec.y, vec.z, vec.w);
        return convBuffer;
    }

    const char* Util::FormatColour(Byte4 colour)
    {
        // Challenge: can you do faster than this? :P

        if (colour.a != 0xFF)
        {
            snprintf(convBuffer, li_lengthof(convBuffer), "#%02X%02X%02X%02X", colour.r, colour.g, colour.b, colour.a);
            return convBuffer;
        }

#ifdef li_little_endian
        unsigned int rgba;
        memcpy(&rgba, &colour[0], 4);
#else
        unsigned int rgba = colour.r | (colour.g << 8) | (colour.b << 16);
#endif

        const unsigned int rgba_hinib = (rgba & 0x00F0F0F0) >> 4;
        const unsigned int rgba_lonib = rgba & 0x000F0F0F;

        if (rgba_hinib == rgba_lonib)
            snprintf(convBuffer, li_lengthof(convBuffer), "#%01X%01X%01X", rgba_lonib & 0x0F, (rgba_lonib >> 8) & 0x0F, rgba_lonib >> 16);
        else
            snprintf(convBuffer, li_lengthof(convBuffer), "#%02X%02X%02X", colour.r, colour.g, colour.b);

        return convBuffer;
    }

    const char* Util::FormatColourHRRGGBB(Byte4 colour)
    {
        snprintf(convBuffer, li_lengthof(convBuffer), "#%02X%02X%02X", colour.r, colour.g, colour.b);
        return convBuffer;
    }
    
    const char* Util::FormatTimestamp(const time_t* t)
    {
        strftime(convBuffer, li_lengthof(convBuffer), "%Y-%m-%dT%H:%M:%S", localtime(t));
        return convBuffer;
    }

    Byte3 Util::HSLtoRGB(float h, float s, float l)
    {
        float r, g, b;

        if ( s < 0.01f )
        {
           r = l * 255.0f;
           g = l * 255.0f;
           b = l * 255.0f;
        }
        else
        {
            float var_1, var_2;

            if ( l < 0.5f )
                var_2 = l * ( 1 + s );
            else
                var_2 = ( l + s ) - ( s * l );

            var_1 = 2 * l - var_2;

            r = 255.0f * getHueToRGBCoeff( var_1, var_2, h + ( 1.0f / 3.0f ) );
            g = 255.0f * getHueToRGBCoeff( var_1, var_2, h );
            b = 255.0f * getHueToRGBCoeff( var_1, var_2, h - ( 1.0f / 3.0f ) );
        }

        return Byte3((uint8_t) r, (uint8_t) g, (uint8_t) b);
    }

    const char* Util::Md5String(const char *str)
    {
        MD5_CTX ctx;
        uint8_t md5[16];
        static char string[33];

        MD5_Init(&ctx);
        MD5_Update(&ctx, (void*) str, strlen(str));
        MD5_Final(md5, &ctx);

        snprintf(string, sizeof(string),
                "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
                md5[0], md5[1], md5[2], md5[3], md5[4], md5[5], md5[6], md5[7],
                md5[8], md5[9], md5[10], md5[11], md5[12], md5[13], md5[14], md5[15]
        );

        return string;
    }

    void* Util::MemDup(const void* data, size_t length)
    {
        if (length == 0)
            return nullptr;
        
        char* dup = (char*) malloc(length);
        
        if (dup != nullptr)
            memcpy(dup, data, length);

        return dup;
    }

    bool Util::ParseBool(const char* stringOrNull)
    {
        if (stringOrNull == nullptr)
            return false;

        return strcmp(stringOrNull, "1") == 0
                || strcmp(stringOrNull, "true") == 0;
    }

    bool Util::ParseColour(const char* string, Byte4& colour_out)
    {
        if (string == nullptr || *string == 0)
            return false;

        if (*string == '#')
            return ParseColourHex(string + 1, strlen(string + 1), colour_out);

        return false;
    }

    bool Util::ParseColourHex(const char* chars, size_t length, Byte4& colour_out)
    {
        if (length == 3)
        {
            colour_out.r = HexDigit(chars[0]) * 0x11;
            colour_out.g = HexDigit(chars[1]) * 0x11;
            colour_out.b = HexDigit(chars[2]) * 0x11;
            colour_out.a = 0xFF;
        }
        else if (length == 6)
        {
            colour_out.r = HexDigit(chars[0]) * 0x10 + HexDigit(chars[1]);
            colour_out.g = HexDigit(chars[2]) * 0x10 + HexDigit(chars[3]);
            colour_out.b = HexDigit(chars[4]) * 0x10 + HexDigit(chars[5]);
            colour_out.a = 0xFF;
        }
        else if (length == 8)
        {
            colour_out.r = HexDigit(chars[0]) * 0x10 + HexDigit(chars[1]);
            colour_out.g = HexDigit(chars[2]) * 0x10 + HexDigit(chars[3]);
            colour_out.b = HexDigit(chars[4]) * 0x10 + HexDigit(chars[5]);
            colour_out.a = HexDigit(chars[6]) * 0x10 + HexDigit(chars[7]);
        }
        else
            return false;

        return true;
    }

    int Util::ParseEnum(const char* string, int& value, const Flag* flags)
    {
        if (Util::StrEmpty(string))
            return -1;

        for (int i = 0; flags[i].name != nullptr; i++)
        {
            if (strcmp(flags[i].name, string) == 0)
            {
                value = flags[i].value;
                return 1;
            }
        }

        return -1;
    }

    int Util::ParseFlagVec(const char* string, int& value, const Flag* flags)
    {
        size_t count = 0;

        if (Util::StrEmpty(string))
            return 0;

        while (1)
        {
            const char *sep, *flag;
            size_t flag_length;
            size_t i;

            while (isspace(*string))
                string++;

            sep = strchr(string, ',');

            if (sep != NULL)
            {
                flag = string;
                flag_length = (sep - string);
            }
            else
            {
                flag = string;
                flag_length = strlen(string);
            }

            for (i = 0; flags[i].name != nullptr; i++)
            {
                if (strlen(flags[i].name) == flag_length
                        && memcmp(flags[i].name, flag, flag_length) == 0)
                    break;
            }

            if (flags[i].name == nullptr)
                return -1;

            value |= flags[i].value;
            count++;

            if (sep != NULL)
                string = sep + 1;
            else
                break;
        }

        return (int) count;
    }

    int Util::ParseInt(const char* stringOrNull, int def)
    {
        if (stringOrNull == nullptr)
            return def;

        char* endPtr = nullptr;

        errno = 0;
        long int value = strtol(stringOrNull, &endPtr, 0);

        if (errno != 0 || endPtr == stringOrNull || (int) value != value)
            return def;

        return (int) value;
    }

    int Util::ParseIntVec(const char* string, int* vec, size_t minCount, size_t maxCount)
    {
        ParseIntVecStruct parseIntVec = { vec, maxCount };

        int count = Util::ParseVec(string, ParseIntVecCallback, &parseIntVec);

        if (count >= 0 && (unsigned) count < minCount)
            return -1;

        return count;
    }

    int Util::ParseVec(const char* string, int (*callback)(const char* value, const char** end_value, void *user), void *user)
    {
        size_t count = 0;

        if (Util::StrEmpty(string))
            return 0;

        while (1)
        {
            const char *end_ptr;

            if (callback(string, &end_ptr, user) < 0)
                break;

            if (*end_ptr == ',')
            {
                count++;
                string = end_ptr + 1;
                continue;
            }

            if (end_ptr != string)
            {
                count++;
                string = end_ptr;
            }

            break;
        }

        if (*string == 0)
            return (int) count;
        else
            return -1;
    }

    int Util::ParseVec(const char* string, float* vec, size_t minCount, size_t maxCount)
    {
        size_t count = 0;

        if (Util::StrEmpty(string))
            return (minCount == 0) ? (0) : (-1);

        while (count < maxCount)
        {
            char *end_ptr;

            vec[count] = (float) strtod(string, &end_ptr);

            if (*end_ptr == ',')
            {
                count++;
                string = end_ptr + 1;
                continue;
            }

            if (end_ptr != string)
            {
                count++;
                string = end_ptr;
            }

            break;
        }

        if (*string == 0 && count >= minCount)
            return (int) count;
        else
            return -1;
    }
    
    char* Util::StrDup(const char* str)
    {
        char* dup = (char*) malloc(strlen(str) + 1);
        
        if (dup != nullptr)
            strcpy(dup, str);

        return dup;
    }
};
