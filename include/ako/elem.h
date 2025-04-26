// Copyright (c) 2025 Tuyuji, Reece Hagan
// SPDX-License-Identifier: MIT

#pragma once
#include "types.h"

// AElem, aka Ako Element
// This is a variant for ako, it can be a table, array, string, int or float.

typedef enum
{
    AT_NULL,
    AT_STRING,
    AT_INT,
    AT_FLOAT,
    AT_SHORTTYPE,
    AT_BOOL,
    AT_TABLE,
    AT_ARRAY,
    AT_ERROR,
} ako_type_t;

static const char *AkoType_Strings[] = {[AT_NULL] = "null",   [AT_STRING] = "string",       [AT_INT] = "int",
                                        [AT_FLOAT] = "float", [AT_SHORTTYPE] = "shorttype", [AT_BOOL] = "bool",
                                        [AT_TABLE] = "table", [AT_ARRAY] = "array",         [AT_ERROR] = "error"};

typedef struct ako_elem ako_elem_t;

ako_elem_t *ako_elem_create(ako_type_t type);
void ako_elem_destroy(ako_elem_t *elem);
void ako_elem_set_type(ako_elem_t *elem, ako_type_t new_type);
ako_type_t ako_elem_get_type(ako_elem_t *elem);
bool ako_elem_is_error(ako_elem_t *elem);

// You transfer ownership of the element to the given table or array.
// Calling ako_elem_table_remove or ako_elem_array_remove will free the element.

// Table
ako_elem_t *ako_elem_table_add(ako_elem_t *table, const char *key, ako_elem_t *value);
ako_elem_t *ako_elem_table_get(ako_elem_t *table, const char *key);
size_t ako_elem_table_get_length(ako_elem_t *table);
const char *ako_elem_table_get_key_at(ako_elem_t *table, size_t index);
ako_elem_t *ako_elem_table_get_value_at(ako_elem_t *table, size_t index);
void ako_elem_table_remove(ako_elem_t *table, const char *key);
bool ako_elem_table_contains(ako_elem_t *table, const char *key);

// Array
ako_elem_t *ako_elem_array_add(ako_elem_t *array, ako_elem_t *value);
ako_elem_t *ako_elem_array_get(ako_elem_t *array, size_t index);
size_t ako_elem_array_get_length(ako_elem_t *array);
void ako_elem_array_remove(ako_elem_t *array, size_t index);

// Setters, will reset the type of the element
void ako_elem_set_null(ako_elem_t *elem);
void ako_elem_set_string(ako_elem_t *elem, const char *str);
void ako_elem_set_int(ako_elem_t *elem, ako_int value);
void ako_elem_set_float(ako_elem_t *elem, ako_float value);
void ako_elem_set_shorttype(ako_elem_t *elem, const char *str);
void ako_elem_set_bool(ako_elem_t *elem, bool value);

// Getters
const char *ako_elem_get_string(ako_elem_t *elem);
ako_int ako_elem_get_int(ako_elem_t *elem);
ako_float ako_elem_get_float(ako_elem_t *elem);
const char *ako_elem_get_shorttype(ako_elem_t *elem);
bool ako_elem_get_bool(ako_elem_t *elem);

// Utils
ako_elem_t *ako_elem_create_int(ako_int value);
ako_elem_t *ako_elem_create_float(ako_float value);
ako_elem_t *ako_elem_create_string(const char *str);
ako_elem_t *ako_elem_create_shorttype(const char *str);
ako_elem_t *ako_elem_create_bool(bool value);
ako_elem_t *ako_elem_create_error(const char *error);
ako_elem_t *ako_elem_create_errorf(const char *fmt, ...);