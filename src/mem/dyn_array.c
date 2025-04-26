// Copyright (c) 2025 Tuyuji, Reece Hagan
// SPDX-License-Identifier: MIT
#include "dyn_array.h"
#include <ako/ako.h>
#include <assert.h>
#include <malloc.h>
#include <memory.h>

#define da_assert(expr, message) assert(expr);

dyn_array_t dyn_array_create(size_t element_size)
{
    dyn_array_t array;
    memset(&array, 0, sizeof(dyn_array_t));
    array.internal.element_size = element_size;
    dyn_array_resize(&array, 10);
    return array;
}

void dyn_array_destroy(dyn_array_t *array)
{
    if (array->internal.data != NULL)
    {
        ako_free(array->internal.data);
    }
    memset(array, 0, sizeof(dyn_array_t));
}

void *dyn_array_elem_location(dyn_array_t *array, size_t index)
{
    da_assert(array != NULL, "Null array pointer.");

    return (uint8_t *)array->internal.data + (array->internal.element_size * index);
}

void dyn_array_resize(dyn_array_t *array, size_t new_size)
{
    da_assert(array != NULL, "Null pointer.");
    da_assert(array->internal.element_size != 0, "Element size cant be zero.");
    da_assert(!(new_size < array->internal.total_size), "Cant shrink arrays.");

    // need new data pointers that can store the new data
    // first how much memory do we need?
    size_t new_memory_size = array->internal.element_size * new_size;
    void *new_data;
    if (array->internal.data == NULL)
    {
        new_data = ako_malloc(new_memory_size);
    }
    else
    {
        new_data = ako_realloc(array->internal.data, new_memory_size);
    }

    da_assert(new_data != NULL, "Failed to alloc for resize, buy more ram.");

    array->internal.data = new_data;
    array->internal.total_size = new_size;
}

void dyn_array_append(dyn_array_t *array, void *data, size_t size)
{
    da_assert(array != NULL, "Null array pointer.");
    da_assert(data != NULL, "Null data pointer.");
    da_assert(array->internal.element_size == size, "Element size mismatch.");
    da_assert(array->internal.element_size != 0, "Element size can't be zero.");
    da_assert(size > 0, "Size can't be less than 1");

    // check if array is at max capacity
    if (array->size + 1 > array->internal.total_size)
    {
        // resize
        dyn_array_resize(array, array->internal.total_size * 2);
    }

    void *element_location = dyn_array_elem_location(array, array->size);
    memcpy(element_location, data, size);
    // done, it's good!
    array->size++;
}

void *dyn_array_get(dyn_array_t *array, size_t index)
{
    da_assert(array != NULL, "Null array pointer.");
    da_assert(array->size > index, "Index outside array size.");

    return dyn_array_elem_location(array, index);
}

void dyn_array_remove(dyn_array_t *array, size_t index)
{
    da_assert(array != NULL, "Null array pointer.");
    da_assert(array->size > index, "Index outside array size.");

    void *element_location = (uint8_t *)array->internal.data + (array->internal.element_size * index);

    if (index < array->size - 1)
    {
        void *next_element_location = (uint8_t *)element_location + array->internal.element_size;
        size_t bytes_to_move = (array->size - index - 1) * array->internal.element_size;
        memmove(element_location, next_element_location, bytes_to_move);
    }

    array->size--;
}