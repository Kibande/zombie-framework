#pragma once

#include <stddef.h>

#define ARG_DEFAULT             0
#define ARG_SINGLE_CHAR         1
#define ARG_SINGLE_CHAR_EXT     2
#define ARG_MULTI_CHAR          3
#define ARG_MULTI_CHAR_EXT      4

typedef struct
{
    const char *single_char, *single_char_ext;
    const char **multi_char, **multi_char_ext;

    int (*on_arg)(int type, const char* arg, const char* ext);
    void (*on_err)(const char* arg);
}
parse_args_t;

#define PARSE_ARGS_ARG(type_, arg_, ext_) {if (args->on_arg != NULL && args->on_arg((type_), (arg_), (ext_)) < 0)\
        return -1;}

#define PARSE_ARGS_ERR(arg_) {if (args->on_err != NULL)\
        args->on_err((arg_));}

static int parse_args(int argc, char** argv, const parse_args_t* args)
{
    int i, j, only_defaults;

    only_defaults = 0;

    for (i = 0; i < argc; i++)
    {
        /* test arguments starting with '-' */

        if (!only_defaults && argv[i][0] == '-')
        {
            if (argv[i][1] == 0)
                only_defaults = 1;
            else
            {
                if (argv[i][1] == '-')
                {
                    /* multi-character arguments */
                    if (args->multi_char_ext != NULL)
                    {
                        for (j = 0; args->multi_char_ext[j]; j++)
                        {
                            size_t k;

                            for (k = 0; ; k++)
                                if (argv[i][k + 2] == 0 || args->multi_char_ext[j][k] == 0 || argv[i][k + 2] != args->multi_char_ext[j][k])
                                    break;

                            if (argv[i][k + 2] != '=' || args->multi_char_ext[j][k] == 0)
                                continue;

                            PARSE_ARGS_ARG(ARG_MULTI_CHAR_EXT, argv[i], argv[i] + k + 3)
                            break;
                        }

                        if (args->multi_char_ext[j])
                            continue;
                    }

                    if (args->multi_char != NULL)
                    {
                        for (j = 0; args->multi_char[j]; j++)
                            if (strcmp(argv[i] + 2, args->multi_char[j]) == 0)
                            {
                                PARSE_ARGS_ARG(ARG_MULTI_CHAR, argv[i], NULL)
                                break;
                            }

                        if (args->multi_char[j])
                            continue;
                    }
                }

                /* single-character extended arguments (-ofilename; -o filename) */

                if (args->single_char_ext != NULL)
                {
                    for (j = 0; args->single_char_ext[j]; j++)
                        if (argv[i][1] == args->single_char_ext[j])
                        {
                            if (argv[i][2] == 0)
                            {
                                if (i + 1 >= argc)
                                {
                                    PARSE_ARGS_ERR(argv[i])
                                    return -1;
                                }
                                else
                                {
                                    PARSE_ARGS_ARG(ARG_SINGLE_CHAR_EXT, argv[i], argv[i + 1]);
                                    i++;
                                }
                            }
                            else
                                PARSE_ARGS_ARG(ARG_SINGLE_CHAR_EXT, argv[i], argv[i] + 2)
                            break;
                        }

                    if (args->single_char_ext[j])
                        continue;
                }

                /* single-character arguments (-a) */

                if (args->single_char != NULL)
                {
                    for (j = 0; args->single_char[j]; j++)
                        if (argv[i][1] == args->single_char[j])
                        {
                            if (argv[i][2] != 0)
                            {
                                PARSE_ARGS_ERR(argv[i])
                                return -1;
                            }
                            else
                                PARSE_ARGS_ARG(ARG_SINGLE_CHAR, argv[i], NULL)
                            break;
                        }

                    if (args->single_char[j])
                        continue;
                }

                PARSE_ARGS_ERR(argv[i])
                return -1;
            }
        }
        else
            PARSE_ARGS_ARG(ARG_DEFAULT, argv[i], NULL)
    }

    return 0;
}
