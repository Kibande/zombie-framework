
#include <framework/utility/params.hpp>

#include <cctype>
#include <cstdarg>

namespace zfw
{
    // Note: this whole thing is completely UTF-8-agnostic, therefore it's OK to manipluate plain ol' chars.

    static char s_keyBuffer[128];
    static char s_valueBuffer[4096];

    static int s_NeedsEscaping(char c)
    {
        /* escaped characters:
            '\\'    because it's used for escaping itself
            '='     key-value delimiter, escaped to reduce confusion when reading embedded params
            ','     key-value separator, needs to be escaped for embedding
            ';'     marks the end of a param set (currently not implemented elsewhere)
        */

        if (c == '\\' || c == '=' || c == ',' || c == ';')
            return 1;
        else
            return 0;
    }

    static void s_EscapedCopy(char*& p_destination, const char* p_source)
    {
        if (p_source == nullptr)
            return;

        while (*p_source != 0)
        {
            if (!s_NeedsEscaping(*p_source))
                *p_destination++ = *p_source++;
            else
            {
                *p_destination++ = '\\';
                *p_destination++ = *p_source++;
            }
        }
    }

    static size_t s_EscapedLength(const char* p_source)
    {
        if (p_source == nullptr)
            return 0;

        unsigned int len = 0;

        while (*p_source != 0)
            len += 1 + s_NeedsEscaping(*p_source++);
        
        return (size_t) len;
    }

    static bool s_Unescape(const char*& p_params, char* p_buffer, size_t bufferSize, char delim)
    {
        // 1 for terminating NUL, 1 for 0-based indexing
        const char* lastWritableChar = p_buffer + bufferSize - 2;

        while (*p_params != 0 && *p_params != delim && *p_params != ';')
        {
            // ... and don't even think about returning an incomplete (=incorrect) value - potential security risk!
            if (p_buffer > lastWritableChar)
                return false;

            if (p_params[0] == '\\' && p_params[1] != 0)
            {
                p_params++;
                *p_buffer++ = *p_params++;
            }
            else
                *p_buffer++ = *p_params++;
        }

        *p_buffer = 0;
        return true;
    }

    static void s_BuildParamsUnsafe(char* p_params, unsigned int numPairs, va_list args)
    {
        for (unsigned int pair = 0; pair < numPairs; pair++)
        {
            if (pair != 0)
                *p_params++ = ',';

            s_EscapedCopy(p_params, va_arg(args, const char*));
            *p_params++ = '=';
            s_EscapedCopy(p_params, va_arg(args, const char*));
        }

        *p_params = 0;
    }

    char* Params::BuildAlloc(unsigned int numPairs, ...)
    {
        va_list args;

        va_start( args, numPairs );
        size_t length = Params::CalculateLength(numPairs, args);
        va_end( args );

        va_start( args, numPairs );
        char* desc = Params::BuildAllocv(numPairs, length, args);
        va_end( args );

        return desc;
    }

    char* Params::BuildAllocv(unsigned int numPairs, size_t length, va_list args)
    {
        char* params = (char*) malloc(length);

        if (params == nullptr)
            return nullptr;

        s_BuildParamsUnsafe(params, numPairs, args);

        return params;
    }

    bool Params::BuildIntoBuffer(char* buffer, size_t bufferSize, unsigned int numPairs, ...)
    {
        va_list args;

        va_start( args, numPairs );
        size_t length = Params::CalculateLength(numPairs, args);
        va_end( args );

        if (length > bufferSize)
            return false;

        va_start( args, numPairs );
        s_BuildParamsUnsafe(buffer, numPairs, args);
        va_end( args );

        return true;
    }

    size_t Params::CalculateLength(unsigned int numPairs, va_list args)
    {
        size_t length = 0;

        for ( ; numPairs != 0; numPairs--)
        {
            if (length > 0)
                length++;

            length += s_EscapedLength(va_arg(args, const char*));
            length++;
            length += s_EscapedLength(va_arg(args, const char*));
        }

        return length + 1;
    }

    bool Params::GetValueForKey(const char* params, const char* key, const char*& value_out)
    {
        if (!GetValueForKeyWithBuffers(params, key, s_keyBuffer, sizeof(s_keyBuffer), s_valueBuffer, sizeof(s_valueBuffer)))
            return false;

        value_out = s_valueBuffer;

        return true;
    }

    bool Params::GetValueForKeyWithBuffers(const char* params, const char* key, char* keyBuffer, size_t keyBufferSize, char* valueBuffer, size_t valueBufferSize)
    {
        while (NextWithBuffers(params, keyBuffer, keyBufferSize, valueBuffer, valueBufferSize))
        {
            if (strcmp(keyBuffer, key) == 0)
                return true;
        }

        return false;
    }

    int Params::IsValid(const char* possiblyParams)
    {
        return IsValidWithBuffers(possiblyParams, s_keyBuffer, sizeof(s_keyBuffer), s_valueBuffer, sizeof(s_valueBuffer));
    }

    int Params::IsValidWithBuffers(const char* possiblyParams, char* keyBuffer, size_t keyBufferSize, char* valueBuffer, size_t valueBufferSize)
    {
        if (possiblyParams == nullptr)
            return PARAMS_EMPTY;

        if (keyBufferSize < 1 || valueBufferSize < 1)
            // can't really work with that
            return PARAMS_INVALID;

        auto p_params = possiblyParams;
        int validity = PARAMS_EMPTY;

        for (;;)
        {
            // skip whitespace, bail out on ';'
            while (true)
            {
                if (isspace(*p_params))
                    p_params++;
                else if (*p_params == ';' || *p_params == 0)
                    return validity;
                else
                    break;
            }

            if (!s_Unescape(p_params, keyBuffer, keyBufferSize, '='))
                // buffer overflow, consider invalid
                return PARAMS_INVALID;

            if (keyBuffer[0] == 0 || *p_params != '=')
                // malformed param set (key musn't be null)
                return false;

            p_params++;

            if (!s_Unescape(p_params, valueBuffer, valueBufferSize, ','))
                // buffer overflow, same as above
                return PARAMS_INVALID;

            if (*p_params == ',')
                p_params++;

            validity = PARAMS_VALID;
        }
    }

    bool Params::Next(const char*& p_params, const char*& key_out, const char*& value_out)
    {
        if (!NextWithBuffers(p_params, s_keyBuffer, sizeof(s_keyBuffer), s_valueBuffer, sizeof(s_valueBuffer)))
            return false;

        key_out = s_keyBuffer;
        value_out = s_valueBuffer;

        return true;
    }

    bool Params::NextWithBuffers(const char*& p_params, char* keyBuffer, size_t keyBufferSize, char* valueBuffer, size_t valueBufferSize)
    {
        if (p_params == nullptr || keyBufferSize < 1 || valueBufferSize < 1)
            return false;

        // skip whitespace, bail out on ';'
        while (true)
        {
            if (isspace(*p_params))
                p_params++;
            else if (*p_params == ';')
                return false;
            else
                break;
        }

        if (!s_Unescape(p_params, keyBuffer, keyBufferSize, '='))
            // buffer overflow
            return false;

        if (keyBuffer[0] == 0 || *p_params != '=')
            // malformed param set (key musn't be null)
            return false;

        p_params++;

        if (!s_Unescape(p_params, valueBuffer, valueBufferSize, ','))
            // buffer overflow
            return false;

        if (*p_params == ',')
            p_params++;

        return true;
    }
}
