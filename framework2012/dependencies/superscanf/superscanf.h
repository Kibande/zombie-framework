#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*get_char_t)(void* user_arg);

/*
    supported format specifiers:
        %i,d
        %s
        %S
*/

int ssscanf(const char* string, int flags, const char* format, ...);
int ssscanf2(const char* string, char** endptr, int flags, const char* format, ...);

#ifdef __cplusplus
}
#endif