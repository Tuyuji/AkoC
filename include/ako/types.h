// Copyright (c) 2025 Tuyuji, Reece Hagan
// SPDX-License-Identifier: MIT

#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

typedef int64_t ako_int;
typedef double ako_float;

typedef struct {
	void* (*malloc_func)(size_t);
	void  (*free_func)(void*);
	void* (*realloc_func)(void*, size_t);
	void* userdata;
} ako_alloc_t;