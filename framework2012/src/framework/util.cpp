
#include "framework.hpp"

extern "C" {
#include "../ztype/md5.h"
}

#include <cctype>

namespace zfw
{
    static char convBuffer[160];

    Int2 Util::Align(Int2 pos, Int2 size, int alignment)
    {
        if (alignment & (ALIGN_CENTER|ALIGN_RIGHT|ALIGN_MIDDLE|ALIGN_BOTTOM))
        {
            if (alignment & ALIGN_CENTER)
                pos.x -= size.x / 2;
            else if (alignment & ALIGN_RIGHT)
                pos.x -= size.x;

            if (alignment & ALIGN_MIDDLE)
                pos.y -= size.y / 2;
            else if (alignment & ALIGN_BOTTOM)
                pos.y -= size.y;

            return pos;
        }
        else
            return pos;
    }

    const char* Util::Format(const glm::ivec2& vec)
    {
        snprintf(convBuffer, lengthof(convBuffer), "%i, %i", vec.x, vec.y);
        return convBuffer;
    }

    const char* Util::Format(const glm::vec2& vec)
    {
        snprintf(convBuffer, lengthof(convBuffer), "%g, %g", vec.x, vec.y);
        return convBuffer;
    }

    const char* Util::Format(const glm::vec3& vec)
    {
        snprintf(convBuffer, lengthof(convBuffer), "%g, %g, %g", vec.x, vec.y, vec.z);
        return convBuffer;
    }

    const char* Util::Format(const glm::vec4& vec)
    {
        snprintf(convBuffer, lengthof(convBuffer), "%g, %g, %g, %g", vec.x, vec.y, vec.z, vec.w);
        return convBuffer;
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

        return count;
    }

    struct ParseIntVecStruct
    {
        int* vec;
        size_t maxCount;
    };

    static int ParseIntVecCallback(const char* value, const char** end_value, void *user)
    {
        auto parseIntVec = (ParseIntVecStruct*) user;

        if (parseIntVec->maxCount == 0)
            return -1;

        parseIntVec->vec[0] = strtol(value, (char**) end_value, 0);

        parseIntVec->vec++;
        parseIntVec->maxCount--;

        return 0;
    }

    int Util::ParseIntVec(const char* string, int* vec, size_t minCount, size_t maxCount)
    {
        ParseIntVecStruct parseIntVec = { vec, maxCount };

        int count = ParseVec(string, ParseIntVecCallback, &parseIntVec);

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
            return count;
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
            return count;
        else
            return -1;
    }
};
