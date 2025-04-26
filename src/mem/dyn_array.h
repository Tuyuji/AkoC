// Copyright (c) 2025 Tuyuji, Reece Hagan
// SPDX-License-Identifier: MIT
#pragma once
#include <stddef.h>

struct dyn_array_private{
	void* data;
	size_t element_size; // sizeof(MyThing)
	size_t total_size; // the total size we can store in our current allocated space.
};

typedef struct dyn_array{
	struct dyn_array_private internal;
	size_t size;
} dyn_array_t;

#define DYN_APPEND(array, item) dyn_array_append(array, &item, sizeof(item))
#define DYN_GET(array, index)   dyn_array_get(array, index)

dyn_array_t  dyn_array_create(size_t element_size);
void		 dyn_array_destroy(dyn_array_t* array);
void         dyn_array_resize(dyn_array_t* array, size_t new_size);
void		 dyn_array_append(dyn_array_t* array, void* data, size_t size);
void*		 dyn_array_get(dyn_array_t* array, size_t index);
void		 dyn_array_remove(dyn_array_t* array, size_t index);
