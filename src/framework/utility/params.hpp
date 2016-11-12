#pragma once

#include <framework/base.hpp>

namespace zfw
{
    // ====================================================================== //
    //  utility class Params
    //      params format:
    //          key=value,key2=value2,key3=value3
    //          whitespace is only allowed before key
    //          semicolon terminates a set
    //              (you can check if *p_params == ';' after Params::Next)
    //
    //      value:
    //          absolutely arbitaty (except NUL) as long as escaped properly
    //          convention: bools always expressed as 0 or 1
    //
    //      encoding:
    //          ASCII or a reasonably compatible one (such as UTF-8)
    //
    //      escaped characters in key/value:    ('\\' is the escaper)
    //          ',' '=' ';' '\\'
    //
    //      limits: (not always enforced, see below)
    //          key length ... 127 bytes max
    //          value length ... 4095 bytes max
    // ====================================================================== //
    enum ParamValidity_t
    {
        PARAMS_INVALID = 0,
        PARAMS_EMPTY,
        PARAMS_VALID
    };

    class Params
    {
        public:
            // limits not enforced for: NextWithBuffers, BuildAlloc[v], CalculateLength

            static bool         Next(const char*& p_params, const char*& key_out, const char*& value_out);
            static bool         NextWithBuffers(const char*& p_params, char* keyBuffer, size_t keyBufferSize, char* valueBuffer, size_t valueBufferSize);

            static bool         GetValueForKey(const char* params, const char* key, const char*& value_out);
            static bool         GetValueForKeyWithBuffers(const char* params, const char* key, char* keyBuffer, size_t keyBufferSize, char* valueBuffer, size_t valueBufferSize);

            // returns one of ParamValidity_t (but it's ok to use < > comparison on it)
            static int          IsValid(const char* possiblyParams);
            static int          IsValidWithBuffers(const char* possiblyParams, char* keyBuffer, size_t keyBufferSize, char* valueBuffer, size_t valueBufferSize);

            // 2, "key", "value", "key2", "value2"
            static char*        BuildAlloc(unsigned int numPairs, ...);
            static char*        BuildAllocv(unsigned int numPairs, size_t length, va_list args);
            static bool         BuildIntoBuffer(char* buffer, size_t sizeof_buffer, unsigned int numPairs, ...);
            static size_t       CalculateLength(unsigned int numPairs, va_list args);
    };
}
