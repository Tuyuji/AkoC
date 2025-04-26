// Copyright (c) 2025 Tuyuji, Reece Hagan
// SPDX-License-Identifier: MIT
#include <ako/ako.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
	const char *name;
	int (*func)();
} test_t;

//Null term list
int test_runner(const test_t* tests)
{
	test_t* current_test = (test_t*)tests;

	while (current_test->name != NULL)
	{
		printf("Running test: %s\n", current_test->name);
		int result = current_test->func();
		if (result != 0)
		{
			printf("Test %s failed with error code: %d\n", current_test->name, result);
			return result;
		}
		current_test++;
	}
	return 0;
}

int assert_elem_valid(ako_elem_t* elem)
{
	if (elem == NULL)
	{
		printf("assert_elem_valid Failed with no result.\n");
		return 1;
	}

	if(ako_elem_is_error(elem)){
		printf("assert_elem_valid failed: %s\n", ako_elem_get_string(elem));
		ako_elem_destroy(elem);
		return 1;
	}

	return 0;
}

static int assert_str_eq(ako_elem_t* a, const char* expected)
{
	if (a == NULL)
	{
		printf("assert_str_eq Failed with no result.\n");
		return 1;
	}

	ako_type_t a_type = ako_elem_get_type(a);
	if (a_type != AT_STRING)
	{
		printf("assert_str_eq Failed, expected string, got %s\n", AkoType_Strings[a_type]);
		return 1;
	}

	const char* value = ako_elem_get_string(a);
	if (strcmp(value, expected) != 0)
	{
		printf("assert_str_eq Failed, expected %s, got %s\n", expected, value);
		return 1;
	}

	return 0;
}

#define ASSERT_ELEM(elem) { int result = assert_elem_valid(elem); if (result != 0) return result; }
#define ASSERT_ELEM_STR(elem, expected) { int result = assert_str_eq(elem, expected); if (result != 0) return result; }

int basic_parse()
{
	ako_elem_t* egg = ako_parse("window.size 180x190");
	ASSERT_ELEM(egg);

	ako_elem_destroy(egg);
	return 0;
}

int parse_float()
{
	ako_elem_t* egg = ako_parse("a 1.0 b 42.0 miku 39.39");
	ASSERT_ELEM(egg);

	ako_elem_t* a = ako_elem_table_get(egg, "a");
	assert(ako_elem_get_float(a) == 1.0);

	ako_elem_t* b = ako_elem_table_get(egg, "b");
	assert(ako_elem_get_float(b) == 42.0);

	ako_elem_t* miku = ako_elem_table_get(egg, "miku");
	assert(ako_elem_get_float(miku) == 39.39);

	ako_elem_destroy(egg);
	return 0;
}

int parse_string_esc()
{
	ako_elem_t* egg = ako_parse("viva \"viva \\\"happy\\\"\"");
	ASSERT_ELEM(egg);

	ako_elem_t* viva = ako_elem_table_get(egg, "viva");
	ASSERT_ELEM(viva);

	ASSERT_ELEM_STR(viva, "viva \"happy\"");

	ako_elem_destroy(egg);
	return 0;
}

int parse_short_type()
{
	ako_elem_t* egg = ako_parse("mi &ku window.width 55");
	ASSERT_ELEM(egg);

	ako_elem_t* mi = ako_elem_table_get(egg, "mi");
	ASSERT_ELEM(mi);

	if (ako_elem_get_type(mi) != AT_SHORTTYPE)
	{
		printf("Given mi is not a short type\n");
		ako_elem_destroy(egg);
		return 1;
	}

	const char* value = ako_elem_get_shorttype(mi);
	if (strcmp(value, "ku") != 0)
	{
		printf("Expected ku got %s\n", value);
		ako_elem_destroy(egg);
		return 1;
	}

	ako_elem_destroy(egg);
	return 0;
}

int parse_multi_short_type()
{
	ako_elem_t* egg = ako_parse("viva &viva.happy window.width 55");
	ASSERT_ELEM(egg);

	ako_elem_t* viva = ako_elem_table_get(egg, "viva");
	ASSERT_ELEM(viva);

	if (ako_elem_get_type(viva) != AT_SHORTTYPE)
	{
		printf("Given viva is not a short type\n");
		ako_elem_destroy(egg);
		return 1;
	}

	const char* value = ako_elem_get_shorttype(viva);
	if (strcmp(value, "viva.happy") != 0)
	{
		printf("Expected viva.happy got %s\n", value);
		ako_elem_destroy(egg);
		return 1;
	}

	ako_elem_destroy(egg);
	return 0;
}

static test_t tests[] = {
	{"Basic parsing", &basic_parse},
	{"Float parsing", &parse_float},
	{"String escape parsing", &parse_string_esc},
	{"Short type parsing", &parse_short_type},
	{"Multi short type parsing", &parse_multi_short_type},
	{NULL, NULL} // Null terminator
};

int main(int argc, char** argv){
	return test_runner(tests);
}