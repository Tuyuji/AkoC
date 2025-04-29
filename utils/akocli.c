// Copyright (c) 2025 Tuyuji, Reece Hagan
// SPDX-License-Identifier: MIT
#include "ako/ako.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

void print_help()
{
    printf("Usage: akocli [OPTIONS]\n");
    printf("\nOptions:\n");
    printf("\t-h, --help       Show this help message and exit\n");
    printf("\t-v, --version    Show version information and exit\n");
    printf("\t-i               Input file\n");
    printf("\t-t, --validate   Validate the input file\n");
}

char* read_stdin_all()
{
    size_t buffer_size = 4096;
    size_t len = 0;
    char* buffer = malloc(buffer_size);
    if (buffer == NULL)
    {
        return NULL;
    }

    size_t i = 0;
    while ((i = fread(buffer + len, 1, buffer_size - len, stdin)) > 0)
    {
        len += i;
        if (len == buffer_size)
        {
            buffer_size += 4096;
            char* new_buffer = realloc(buffer, buffer_size);
            if (new_buffer == NULL)
            {
                free(buffer);
                return NULL;
            }
            buffer = new_buffer;
        }
    }

    buffer[len] = '\0';
    return buffer;
}

static struct
{
    char* source;
    ako_elem_t* result;
} state;

void free_state()
{
    if (state.source != NULL)
    {
        free(state.source);
        state.source = NULL;
    }

    if (state.result != NULL)
    {
        ako_elem_destroy(state.result);
        state.result = NULL;
    }
}

//Examples:
//akocli --help
//akocli
int main(int argc, char **argv)
{
    memset(&state, 0, sizeof(state));
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            print_help();
            return 0;
        }
    }

    const char* input_file = NULL;
    const char* query_str = NULL;
    bool validate = false;

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {"input", required_argument, 0, 'i'},
        {"validate", no_argument, 0, 't'},
        {"query", required_argument, 0, 'q'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "hvti:q:", long_options, NULL)) != -1)
    {
        switch (opt)
        {
        case 'h':
            print_help();
            return 0;
        case 'v':
            printf("akocli version " AKO_VERSION_STR "\n");
            return 0;
        case 'i':
            input_file = optarg;
            break;
        case 't':
            validate = true;
            break;
        case 'q':
            query_str = optarg;
            break;
        default:
            print_help();
            return 1;
        }
    }

    bool read_from_stdin = false;

    if (!isatty(STDIN_FILENO))
    {
        read_from_stdin = true;
    }

    if (input_file != NULL && strcmp(input_file, "-") == 0)
    {
        read_from_stdin = true;
    }

    if (read_from_stdin)
    {
        state.source = read_stdin_all();
    }
    else
    {
        if (input_file == NULL)
        {
            printf("No input file specified\n");
            return 1;
        }

        //input_file has to be a file path
        FILE* file = fopen(input_file, "r");
        if (file == NULL)
        {
            perror("fopen");
            return 1;
        }
        fseek(file, 0, SEEK_END);
        size_t file_size = ftell(file);
        fseek(file, 0, SEEK_SET);
        state.source = malloc(file_size + 1);
        if (state.source == NULL)
        {
            perror("malloc");
            fclose(file);
            return 1;
        }
        fread((char*)state.source, 1, file_size, file);
        ((char*)state.source)[file_size] = '\0';
        fclose(file);
    }

    state.result = ako_parse(state.source);
    if (validate)
    {
        if (state.result == NULL || ako_elem_is_error(state.result))
        {
            printf("Failed to parse: %s\n", ako_elem_get_string(state.result));
            free_state();
            return 1;
        }
        printf("Parsed successfully\n");
        free_state();
        return 0;
    }

    if (ako_elem_is_error(state.result))
    {
        printf("Failed to parse: %s\n", ako_elem_get_string(state.result));
        free_state();
        return 1;
    }

    if (query_str != NULL)
    {
        ako_elem_t* elem = ako_elem_get(state.result, query_str);
        if (elem == NULL)
        {
            free_state();
            return 1;
        }

        const char* str = ako_serialize(elem, NULL, ASF_FORMAT);
        printf("%s\n", str);
        ako_free_string(str);
    }

    //idk
    free_state();
    return 0;
}