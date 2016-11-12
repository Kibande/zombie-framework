
#include <framework/framework.hpp>

#include <cstdarg>

// FIXME: This relies on implementation-defined behavious of va_list

namespace zfw
{
    int vasprintf(char **buf, size_t *size, const char *format, va_list arg)
    {
        if (buf == nullptr)
            return -1;

        int rv = vsnprintf(NULL, 0, format, arg);

        if (rv < 0)
        {
            assert(rv >= -1);
            return rv;
        }

        size_t allocSize = (rv + 1 + 7) & ~7;

        if (size != nullptr)
        {
            if (*size < allocSize)
            {
                *buf = (char*) realloc(*buf, allocSize);
                *size = allocSize;
            }
        }
        else
            *buf = (char*) malloc(allocSize);

        ZFW_ASSERT(*buf != nullptr)

        if ((rv = vsnprintf(*buf, rv + 1, format, arg)) < 0)
        {
            free(*buf);
            *buf = NULL;
        }

        return rv;
    }

    int asprintf(char ** buf, const char * format, ...)
    {
        va_list arg;
        int rv;

        va_start(arg, format);
        rv = vasprintf(buf, nullptr, format, arg);
        va_end(arg);

        return rv;
    }

    int asprintf2(char ** buf, size_t* bufSize, const char * format, ...)
    {
        va_list arg;
        int rv;

        va_start(arg, format);
        rv = vasprintf(buf, bufSize, format, arg);
        va_end(arg);

        return rv;
    }
}