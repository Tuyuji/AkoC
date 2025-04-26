// Copyright (c) 2025 Tuyuji, Reece Hagan
// SPDX-License-Identifier: MIT
#pragma once
#include <ako/types.h>
#include <ako/elem.h>

#include "mem/dyn_array.h"

typedef struct table_elem{
	const char* key;
	ako_elem_t* value;
} table_elem_t;

typedef struct array_elem{
	ako_elem_t* item;
} array_elem_t;

typedef union elem{
	table_elem_t table;
	array_elem_t array;
} elem_t;

typedef struct ako_elem {
	ako_type_t type;
	union {
		const char* str; //String, ShortType
		ako_int i; // Int, Bool(1 true, 0 false)
		ako_float f;
		dyn_array_t a; //stores an array of elem_t
	};
} ako_elem_t;

extern char* empty;