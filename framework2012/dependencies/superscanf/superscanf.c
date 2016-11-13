
#include "superscanf.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>

int superscanf_core(get_char_t get_char, void* user_arg, int flags, const char* format, va_list args)
{
    int have, input, eof;

    have = 0;
    input = 0;
    eof = 0;

    while (1)
    {
        int fmt, read_until, *out_int, value, num_read;
        char* out_buffer;
        size_t out_size, out_used;

        fmt = *(format++);

        if (fmt <= 0)
            break;
        else if (fmt == '%')
        {
            fmt = *(format++);

            if (fmt <= 0)
                break;

            switch (fmt)
            {
                case 'd':
                case 'i':
                    out_int = va_arg(args, int*);
                    value = 0;
                    num_read = 0;

                  read_decimal:
                    while (1)
                    {
                        if (input <= 0)
                            input = get_char(user_arg);

                        if (input <= 0)
                        {
                            eof = 1;
                            break;
                        }

                        if (num_read == 0 && isspace(input))
                        {
                            input = 0;
                            continue;
                        }

                        if (!isdigit(input))
                            break;

                        value = value * 10 + (input - '0');
                        num_read++;
                        input = 0;
                    }

                    if (num_read > 0)
                    {
                        *out_int = value;
                        have++;
                    }
                    else
                        eof = 1;
                    break;

                case 'n':
                    /* prefixes for hexadecimal: '0x', '$' */

                    out_int = va_arg(args, int*);
                    value = 0;
                    num_read = 0;

                    while (1)
                    {
                        if (input <= 0)
                            input = get_char(user_arg);

                        if (input <= 0)
                        {
                            eof = 1;
                            break;
                        }

                        if (num_read == 0 && isspace(input))
                        {
                            input = 0;
                            continue;
                        }

                        if (num_read == 0 && input == '0')
                            input = 0;
                        else if ((num_read == 0 && input == '$') || (num_read == 1 && input == 'x'))
                        {
                            input = 0;
                            num_read = 0;
                            goto read_hexadecimal;
                        }
                        else
                            goto read_decimal;

                        num_read++;
                    }

                    if (num_read > 0)
                    {
                        *out_int = value;
                        have++;
                    }
                    else
                        eof = 1;
                    break;

                case 's':
                    out_buffer = va_arg(args, char*);
                    out_size = va_arg(args, size_t);
                    out_used = 0;

                    while (out_used < out_size - 1)
                    {
                        if (input <= 0)
                            input = get_char(user_arg);

                        if (input <= 0)
                        {
                            eof = 1;
                            break;
                        }

                        if (isspace(input))
                            break;

                        out_buffer[out_used++] = input;
                        input = 0;
                    }

                    out_buffer[out_used++] = 0;
                    have++;
                    break;

                case 'S':
                    read_until = *(format++);
                    out_buffer = va_arg(args, char*);
                    out_size = va_arg(args, size_t);
                    out_used = 0;

                    while (out_used < out_size - 1)
                    {
                        if (input <= 0)
                            input = get_char(user_arg);

                        if (input == read_until)
                        {
                            input = 0;
                            break;
                        }

                        if (input <= 0)
                        {
                            eof = 1;
                            break;
                        }

                        out_buffer[out_used++] = input;
                        input = 0;
                    }

                    out_buffer[out_used++] = 0;
                    have++;
                    break;

                case 'x':
                case 'X':
                    out_int = va_arg(args, int*);
                    value = 0;
                    num_read = 0;

                  read_hexadecimal:
                    while (1)
                    {
                        if (input <= 0)
                            input = get_char(user_arg);

                        if (input <= 0)
                        {
                            eof = 1;
                            break;
                        }

                        if (num_read == 0 && isspace(input))
                        {
                            input = 0;
                            continue;
                        }

                        if (input >= '0' && input <= '9')
                            value = value * 16 + (input - '0');
                        else if (input >= 'a' && input <= 'f')
                            value = value * 16 + 10 + (input - 'a');
                        else if (input >= 'A' && input <= 'F')
                            value = value * 16 + 10 + (input - 'A');
                        else
                            break;

                        num_read++;
                        input = 0;
                    }

                    if (num_read > 0)
                    {
                        *out_int = value;
                        have++;
                    }
                    else
                        eof = 1;
                    break;
            }
        }
        else
        {
            if (input <= 0)
                input = get_char(user_arg);

            if (input != fmt)
                break;

            input = 0;
        }

        if (eof)
            break;
    }

    return have;
}

int get_char_str(void* user_arg)
{
    return *(*((char**)user_arg))++;
}

int ssscanf(const char* string, int flags, const char* format, ...)
{
    int result;
    va_list args;

    va_start(args, format);

    result = superscanf_core(get_char_str, (void*)&string, flags, format, args);

    va_end(args);

    return result;
}

int ssscanf2(const char* string, char** endptr, int flags, const char* format, ...)
{
    int result;
    va_list args;

    va_start(args, format);

    result = superscanf_core(get_char_str, (void*)&string, flags, format, args);

    if (endptr)
        *endptr = (char *) string;

    va_end(args);

    return result;
}
