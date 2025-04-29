// Copyright (c) 2025 Tuyuji, Reece Hagan
// SPDX-License-Identifier: MIT
#pragma once
#include <stddef.h>

typedef struct
{
    char* data;
    size_t size;
    size_t capacity;
} dyn_string_t;

// To destroy a dyn_string, just free the data pointer with ako_free :)
dyn_string_t dyn_string_create(size_t initial_capacity);

void dyn_string_append(dyn_string_t* str, const char* data);
void dyn_string_append_fmt(dyn_string_t* str, const char* fmt, ...);
void dyn_string_append_char(dyn_string_t* str, char c);

// Sets the entire data buffer to '\0' but keeps the allocated memory
void dyn_string_clear(dyn_string_t* str);