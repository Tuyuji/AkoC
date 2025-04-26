// Copyright (c) 2025 Tuyuji, Reece Hagan
// SPDX-License-Identifier: MIT
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "ako/ako.h"
#include "dyn_string.h"

void dyn_string_realloc(dyn_string_t *str, size_t new_capacity)
{
    if (str->capacity == new_capacity)
    {
        return;
    }

    if (str->data == NULL)
    {
        str->data = ako_malloc(new_capacity);
        assert(str->data != NULL);
        str->capacity = new_capacity;
        return;
    }

    if (str->capacity < new_capacity)
    {
        char *new_data = ako_realloc(str->data, new_capacity);
        assert(new_data != NULL);
        str->data = new_data;
        str->capacity = new_capacity;
    }
}

dyn_string_t dyn_string_create(size_t initial_capacity)
{
    dyn_string_t str;
    memset(&str, 0, sizeof(dyn_string_t));
    dyn_string_realloc(&str, initial_capacity);
    str.size = 0;
    return str;
}

void dyn_string_append(dyn_string_t *str, const char *data)
{
    size_t len = strlen(data);
    dyn_string_realloc(str, str->size + len + 1);
    memcpy(str->data + str->size, data, len);
    str->size += len;
    str->data[str->size] = '\0';
}

void dyn_string_append_fmt(dyn_string_t *str, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    va_list args_copy;
    va_copy(args_copy, args);
    size_t len = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args);

    dyn_string_realloc(str, str->size + len + 1);

    vsnprintf(str->data + str->size, len + 1, fmt, args);
    va_end(args);

    str->size += len;
}

void dyn_string_append_char(dyn_string_t *str, char c)
{
    dyn_string_realloc(str, str->size + 2);
    str->data[str->size] = c;
    str->size++;
    str->data[str->size] = '\0';
}

void dyn_string_clear(dyn_string_t *str)
{
    // Sets the entire data buffer to '\0' but keeps the allocated memory
    memset(str->data, '\0', str->capacity);
    str->size = 0;
}
