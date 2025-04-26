// Copyright (c) 2025 Tuyuji, Reece Hagan
// SPDX-License-Identifier: MIT
#include <ako/ako.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>
#include <stdbool.h>

#include "private.h"
#include "lex/token.h"
#include "lex/parser.h"
#include "mem/dyn_array.h"
#include "mem/dyn_string.h"

char* empty = NULL;

ako_alloc_t* ako_alloc_get()
{
	static ako_alloc_t ako_alloc = {0};
	if(ako_alloc.malloc_func == NULL){
		ako_alloc.malloc_func = malloc;
		ako_alloc.free_func = free;
		ako_alloc.realloc_func = realloc;
	}
	return &ako_alloc;
}

void* ako_malloc(size_t size)
{
	return ako_alloc_get()->malloc_func(size);
}

void ako_free(void* ptr)
{
	ako_alloc_get()->free_func(ptr);
}

void* ako_realloc(void* ptr, size_t size)
{
	return ako_alloc_get()->realloc_func(ptr, size);
}

void ako_free_string(const char* str)
{
	ako_alloc_get()->free_func((void*)str);
}

ako_elem_t* ako_parse(const char* source){
	static char* err = NULL;
	dyn_array_t tokens = ako_tokenize(source, &err);
	if(tokens.size == 0){
		return ako_elem_create_error(err);
	}

	/*for(size_t i = 0; i < tokens.size; i++)
	{
		token_t* token = dyn_array_get(&tokens, i);
		static char start_str[32];
		static char end_str[32];
		location_format(&token->start, &start_str[0], sizeof(start_str));
		location_format(&token->end, &end_str[0], sizeof(end_str));
		printf("Token: %s, %s to %s", TokenType_Strings[token->type], start_str, end_str);

		char* sym = "-";
		switch (token->type)
		{
		case AKO_TT_STRING:
		case AKO_TT_IDENT:
			printf(" [ %s ]", token->value_string);
			break;
		case AKO_TT_INT:
			printf(" [ %ld ]", token->value_int);
			break;
		case AKO_TT_BOOL:
			if (token->value_int == 1)
			{
				sym = "+";
			}
			printf(" [ %s ]", sym);
			break;
		case AKO_TT_FLOAT:
			printf(" [ %f ]", token->value_float);
			break;
		default: ;
			break;
		}

		printf("\n");
	}*/

	ako_elem_t* result = ako_parse_tokens(&tokens);
	ako_free_tokens(&tokens);

	return result;
}

void _make_indent(dyn_string_t* out, const char* indent, size_t level) {
	assert(out != NULL);
	assert(indent != NULL);
	for (size_t i = 0; i < level; i++) {
		dyn_string_append(out, indent);
	}
}

void _serialise(dyn_string_t* str, ako_elem_t* elem, const char* indent, size_t cur_indent, bool first_run, char** err)
{
	if (err == NULL)
	{
		err = &empty;
	}

	*err = NULL;

	if (!elem)
	{
		return;
	}

	size_t len = 0;
	const char* end = (indent[0] != '\0') ? "\n" : " ";
	switch (elem->type)
	{
	case AT_BOOL:
		dyn_string_append(str, elem->i ? "+" : "-");
		return;
	case AT_NULL:
		dyn_string_append(str, ";");
		return;
	case AT_INT:
		dyn_string_append_fmt(str, "%d", elem->i);
		return;
	case AT_FLOAT:
		dyn_string_append_fmt(str, "%f", elem->f);
		return;

	case AT_STRING:
	case AT_SHORTTYPE:
		dyn_string_append_fmt(str, "\"%s\"", elem->str);
		return;

	case AT_ARRAY:
		len = ako_elem_array_get_length(elem);
		if (len == 0)
		{
			dyn_string_append(str, "[[]]");
			return;
		}


		bool is_vector = (len <= 4);
		for (size_t i = 0; i < len; i++)
		{
			ako_elem_t* ae = ako_elem_array_get(elem, i);
			if (!ae && (ae->type != AT_INT && ae->type != AT_FLOAT))
			{
				is_vector = false;
				break;
			}
		}

		if (first_run)
		{
			//We have to open the array, cant do fancy vector syntax
			is_vector = false;
		}

		if (is_vector)
		{
			for (size_t i = 0; i < len; i++)
			{
				ako_elem_t* ae = ako_elem_array_get(elem, i);
				if (ae->type == AT_INT)
				{
					dyn_string_append_fmt(str, "%d", ae->i);
				}else
				{
					dyn_string_append_fmt(str, "%f", ae->f);
				}
				if (i < len - 1)
				{
					dyn_string_append(str, "x");
				}
			}
			return;
		}

		dyn_string_append(str, "[[");
		dyn_string_append(str, end);

		for (size_t i = 0; i < len; i++)
		{
			_make_indent(str, indent, cur_indent + 1);
			_serialise(str, ako_elem_array_get(elem, i), indent, cur_indent + 1, false, err);
			dyn_string_append(str, end);
		}
		_make_indent(str, indent, cur_indent);
		dyn_string_append(str, "]]");
		return;

	case AT_TABLE:
		len = ako_elem_table_get_length(elem);
		bool opened = false;

		if (!first_run)
		{
			dyn_string_append(str, "[");

			if (len == 0)
			{
				//Nothing so lets just close it and return
				dyn_string_append(str, "]");
				return;
			}

			dyn_string_append(str, end);
			opened = true;
		}

		int indenting = first_run ? 0 : cur_indent + 1;

		dyn_string_t valstr = dyn_string_create(8);

		for (size_t i = 0; i < len; i++)
		{
			const char* key = ako_elem_table_get_key_at(elem, i);
			ako_elem_t* value = ako_elem_table_get_value_at(elem, i);

			_make_indent(str, indent, indenting);

			dyn_string_clear(&valstr);

			*err = NULL;
			_serialise(&valstr, value, indent, indenting, false, err);
			if (*err != NULL)
			{
				//Error serialising value
				ako_free(valstr.data);
				return;
			}

			if (strcmp(valstr.data, "+") == 0 || strcmp(valstr.data, "-") == 0 || strcmp(valstr.data, ";") == 0)
			{
				dyn_string_append_fmt(str, "%s%s", valstr.data, key);
			}else
			{
				dyn_string_append_fmt(str, "%s %s", key, valstr.data);
			}

			dyn_string_append(str, end);
		}

		ako_free(valstr.data);

		_make_indent(str, indent, cur_indent);
		if (opened)
		{
			dyn_string_append(str, "]");
		}
		return;
	default:
		*err = "Unknown type for serialisation";
		return;
	}
}

const char* ako_serialize(ako_elem_t* elem, char** err, ako_serialize_flags_t flags)
{
	if (err == NULL)
	{
		err = &empty;
	}

	*err = NULL;

	char* indent = "\t";
	if (flags & ASF_USE_SPACES)
	{
		indent = "    ";
	}

	if (!(flags & ASF_FORMAT))
	{
		indent = "";
	}

	dyn_string_t str = dyn_string_create(128);
	_serialise(&str, elem, indent, 0, true, err);
	if (*err != NULL)
	{
		//Error output was set and we had an error
		ako_free_string(str.data);
		return NULL;
	}

	return str.data;
}
