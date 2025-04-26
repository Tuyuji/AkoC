// Copyright (c) 2025 Tuyuji, Reece Hagan
// SPDX-License-Identifier: MIT
#pragma once
#define AKO_VMAJOR 0
#define AKO_VMINOR 1
#define AKO_VPATCH 0

#include <ako/elem.h>

typedef enum
{
    ASF_NONE = 0x0,
    ASF_FORMAT = 0x1, // Tabs by default
    ASF_USE_SPACES = 0x2,
} ako_serialize_flags_t;

// Recommended to do this at the start of your program, before you use ako.
// By default, ako uses malloc/free/realloc, but you can set your own allocators
ako_alloc_t *ako_alloc_get();
void *ako_malloc(size_t size);
void ako_free(void *ptr);
void *ako_realloc(void *ptr, size_t size);

void ako_free_string(const char *str);

// Caller gets ownership of the returned element
// Please free it using ako_elem_destroy
ako_elem_t *ako_parse(const char *source);

// Caller gets ownership of the returned string.
// Please free it using ako_free_string
const char *ako_serialize(ako_elem_t *elem, char **err, ako_serialize_flags_t flags);