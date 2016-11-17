#pragma once

#include <framework/errorcodes.hpp>

#include <framework/utility/essentials.hpp>
#include <framework/utility/params.hpp>
#include <framework/utility/util.hpp>

#include <cstdarg>
#include <ctime>

namespace zfw
{
    // This should stay binary-compatible, like core interfaces
    struct ErrorBuffer_t
    {
        int     errorCode;
        char    timestamp[20];
        char*   params;
        ErrorBuffer_t* next;

        // Don't unload the module that sets these!
        void*   (*realloc)(void* ptr, size_t size);
        void    (*free)(void* ptr);
    };

    class ErrorBuffer
    {
        private:
            ErrorBuffer();

        public:
            static void Create(ErrorBuffer_t*& eb)
            {
                // Must not rely on framework in any way

                eb = new ErrorBuffer_t;

                if (eb != nullptr)
                {
                    eb->errorCode = 0;
                    eb->params = nullptr;
                    eb->next = nullptr;

                    eb->realloc = realloc;
                    eb->free = free;
                }
            }

            static void Release(ErrorBuffer_t*& eb)
            {
                // Must not rely on framework in any way

                while (eb != nullptr)
                {
                    auto next = eb->next;

                    eb->free(eb->params);
                    delete eb;

                    eb = next;
                }
            }

            // Deprecated
            static bool SetError(ErrorBuffer_t* eb, int errorCode, ...)
            {
                va_list args;

                //if (Var::GetInt("dev_errorbreak", false, -1) > 0)
                //    Sys::DebugBreak(true);

                time_t ts;
                time(&ts);

                unsigned int count;

                va_start( args, errorCode );
                for (count = 0; ; count++)
                {
                    if (va_arg(args, const char*) == nullptr
                            || va_arg(args, const char*) == nullptr)
                        break;
                }
                va_end( args );

                va_start( args, errorCode );
                size_t length = Params::CalculateLength(count, args);
                va_end( args );

                va_start( args, errorCode );
                char* params = Params::BuildAllocv(count, length, args);
                va_end( args );

                eb->errorCode = errorCode;
                SetParams(eb, params);
                SetTimestamp(eb, &ts);

                free(params);
                return true;
            }

            static bool SetError2(ErrorBuffer_t* eb, int errorCode, unsigned int numPairs ...)
            {
                va_list args;

                //if (Var::GetInt("dev_errorbreak", false, -1) > 0)
                //    Sys::DebugBreak(true);

                time_t ts;
                time(&ts);

                va_start( args, numPairs );
                size_t length = Params::CalculateLength(numPairs, args);
                va_end( args );

                va_start( args, numPairs );
                char* params = Params::BuildAllocv(numPairs, length, args);
                va_end( args );

                eb->errorCode = errorCode;
                SetParams(eb, params);
                SetTimestamp(eb, &ts);

                free(params);
                return true;
            }

            static bool SetError3(int errorCode, unsigned int numPairs ...)
            {
                ErrorBuffer_t* eb = zfw::GetErrorBuffer();

                va_list args;

                //if (Var::GetInt("dev_errorbreak", false, -1) > 0)
                //    Sys::DebugBreak(true);

                time_t ts;
                time(&ts);

                va_start(args, numPairs);
                size_t length = Params::CalculateLength(numPairs, args);
                va_end(args);

                va_start(args, numPairs);
                char* params = Params::BuildAllocv(numPairs, length, args);
                va_end(args);

                eb->errorCode = errorCode;
                SetParams(eb, params);
                SetTimestamp(eb, &ts);

                free(params);
                return true;
            }

            /*static bool SetErrorWithParams(ErrorBuffer_t* eb, int errorCode, const char* params)
            {
                //if (Var::GetInt("dev_errorbreak", false, -1) > 0)
                //    Sys::DebugBreak(true);

                time_t ts;
                time(&ts);

                eb->errorCode = errorCode;
                SetParams(eb, params);
                SetTimestamp(eb, &ts);
                return true;
            }*/

            static bool SetBufferOverflowError(ErrorBuffer_t* eb, const char* functionName)
            {
                return ErrorBuffer::SetError2(eb, EX_INTERNAL_STATE, 1,
                        "desc", sprintf_255("A buffer overflow occured in function '%s'", functionName)
                        );
            }

            static bool SetParams(ErrorBuffer_t* eb, const char* params)
            {
                const size_t len = strlen(params) + 1;

                if (!(eb->params = (char*) eb->realloc(eb->params, len)))
                    return false;

                memcpy(eb->params, params, len);
                return true;
            }

            static bool SetReadError(const char* outputNameOrNull, const char* functionName)
            { 
                return ErrorBuffer::SetError3(EX_IO_ERROR, 2,
                        "desc", (outputNameOrNull != nullptr) ? sprintf_4095("Failed to read data from '%s'", outputNameOrNull) : "Failed to read data",
                        "functionName", functionName
                        );
            }

            static void SetTimestamp(ErrorBuffer_t* eb, const time_t* time)
            {
                strncpy(eb->timestamp, Util::FormatTimestamp(time), sizeof(eb->timestamp) - 1);
                eb->timestamp[sizeof(eb->timestamp) - 1] = 0;
            }

            static bool SetWriteError(const char* outputNameOrNull, const char* functionName)
            { 
                return ErrorBuffer::SetError3(EX_IO_ERROR, 2,
                        "desc", (outputNameOrNull != nullptr) ? sprintf_4095("Failed to write data to '%s'", outputNameOrNull) : "Failed to write data",
                        "functionName", functionName
                        );
            }
    };
}
