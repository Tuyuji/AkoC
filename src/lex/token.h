// Copyright (c) 2025 Tuyuji, Reece Hagan
// SPDX-License-Identifier: MIT
#pragma once
#include <ako/types.h>
#include "../mem/dyn_array.h"

typedef enum {
	AKO_TT_NONE,
	AKO_TT_BOOL,
	AKO_TT_INT,
	AKO_TT_FLOAT,
	AKO_TT_STRING,
	AKO_TT_IDENT,
	AKO_TT_DOT,
	AKO_TT_SEMICOLON,
	AKO_TT_AND,
	AKO_TT_PLUS,
	AKO_TT_MINUS,
	AKO_TT_OPEN_BRACE,
	AKO_TT_CLOSE_BRACE,
	AKO_TT_OPEN_D_BRACE,
	AKO_TT_CLOSE_D_BRACE,
	AKO_TT_VECTORCROSS,
	AKO_TT_MAX
} token_type_t;

static const char* TokenType_Strings[AKO_TT_MAX] = {
	[AKO_TT_NONE]  		   = "None",
	[AKO_TT_BOOL]  		   = "Bool",
	[AKO_TT_INT]           = "Int",
	[AKO_TT_FLOAT]         = "Float",
	[AKO_TT_STRING]        = "String",
	[AKO_TT_IDENT]		   = "Identity",
	[AKO_TT_DOT]           = "Dot",
	[AKO_TT_SEMICOLON]     = "Semicolon",
	[AKO_TT_AND]           = "And",
	[AKO_TT_PLUS]          = "Plus",
	[AKO_TT_MINUS]         = "Minus",
	[AKO_TT_OPEN_BRACE]    = "OpenBrace",
	[AKO_TT_CLOSE_BRACE]   = "CloseBrace",
	[AKO_TT_OPEN_D_BRACE]  = "OpenDoubleBrace",
	[AKO_TT_CLOSE_D_BRACE] = "CloseDoubleBrace",
	[AKO_TT_VECTORCROSS]   = "VectorCross",
};

typedef struct {
	size_t line;
	size_t column;
	size_t index;
} location_t;

typedef struct {
	token_type_t type;
	location_t start;
	location_t end;
	union{
		ako_int value_int; // BOOL, INT
		ako_float value_float; // FLOAT
		const char* value_string; // STRING, IDENT | This is malloced, please free once done :3
	};
} token_t;

size_t location_format(const location_t* loc, char* output, size_t output_size);

//Returns an array of Token_t, please destroy the returned array when finished :)
dyn_array_t ako_tokenize(const char* source, char** err);
void ako_free_tokens(dyn_array_t* tokens);
