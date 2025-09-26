// Copyright (c) 2025 Tuyuji, Reece Hagan
// SPDX-License-Identifier: MIT
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "ako/ako.h"
#include "lex/token.h"
#include "private.h"

#define IS_TABLE_OR_ARRAY(type) (type == AT_TABLE || type == AT_ARRAY)

static const char* string_cpy(const char* source)
{
    size_t len = strlen(source);
    size_t size = len + 1;
    char* str = ako_malloc(size);
    memcpy(str, source, size);
    return str;
}

ako_elem_t* ako_elem_create(ako_type_t type)
{
    ako_elem_t* elem = ako_malloc(sizeof(ako_elem_t));
    if (elem == NULL)
    {
        return NULL;
    }
    memset(elem, '\0', sizeof(ako_elem_t));
    ako_elem_set_type(elem, type);
    return elem;
}

void ako_elem_destroy(ako_elem_t* elem)
{
    assert(elem != NULL);

    if (IS_TABLE_OR_ARRAY(elem->type))
    {
        // iterate over the dyn array and free the elements
        dyn_array_t* array = &elem->a;
        if (elem->type == AT_TABLE)
        {
            for (size_t i = 0; i < array->size; ++i)
            {
                table_elem_t* tableElem = dyn_array_get(array, i);
                ako_free((void*)tableElem->key);
                ako_elem_destroy(tableElem->value);
            }
        }
        else
        {
            for (size_t i = 0; i < array->size; ++i)
            {
                array_elem_t* arrayElem = dyn_array_get(array, i);
                ako_elem_destroy(arrayElem->item);
            }
        }

        // gotta free the dyn_array
        dyn_array_destroy(&elem->a);
    }

    if (elem->type == AT_STRING || elem->type == AT_SHORTTYPE || elem->type == AT_ERROR)
    {
        // free the string
        ako_free((void*)elem->str);
    }

    ako_free(elem);
}

void ako_elem_set_type(ako_elem_t* elem, ako_type_t new_type)
{
    assert(elem != NULL);
    if (elem->type == new_type)
    {
        return;
    }

    bool elem_is_table_array = IS_TABLE_OR_ARRAY(elem->type);
    bool new_type_is_table_array = IS_TABLE_OR_ARRAY(new_type);

    if (elem_is_table_array && !new_type_is_table_array)
    {
        // new type wont make use of the dyn array
        dyn_array_destroy(&elem->a);
        elem->a = (dyn_array_t){0};
    }
    else if (!elem_is_table_array && new_type_is_table_array)
    {
        // elem isnt a table or array
        // new type does need it.
        elem->a = dyn_array_create(sizeof(elem_t));
    }

    elem->type = new_type;
}

ako_type_t ako_elem_get_type(ako_elem_t* elem)
{
    assert(elem != NULL);
    return elem->type;
}

bool ako_elem_is_error(ako_elem_t* elem)
{
    assert(elem != NULL);
    return elem->type == AT_ERROR;
}

static table_elem_t* ako_table_find(ako_elem_t* table, const char* key)
{
    dyn_array_t* array = &table->a;
    for (size_t i = 0; i < array->size; ++i)
    {
        table_elem_t* tableElem = dyn_array_get(array, i);
        if (strcmp(tableElem->key, key) == 0)
        {
            return tableElem;
        }
    }
    return NULL;
}

ako_elem_t* ako_elem_table_add(ako_elem_t* table, const char* key, ako_elem_t* value)
{
    assert(table != NULL);
    assert(table->type == AT_TABLE);
    assert(key != NULL);
    assert(value != NULL);

    elem_t tableElem;
    tableElem.table.key = string_cpy(key);
    tableElem.table.value = value;

    dyn_array_append(&table->a, &tableElem, sizeof(elem_t));
    return value;
}

ako_elem_t* ako_elem_table_get(ako_elem_t* table, const char* key)
{
    assert(table != NULL);
    assert(table->type == AT_TABLE);
    assert(key != NULL);

    table_elem_t* elem = ako_table_find(table, key);
    if (elem == NULL)
    {
        return NULL;
    }
    return elem->value;
}

size_t ako_elem_table_get_length(ako_elem_t* table)
{
    assert(table != NULL);
    assert(table->type == AT_TABLE);

    return table->a.size;
}

const char* ako_elem_table_get_key_at(ako_elem_t* table, size_t index)
{
    assert(table != NULL);
    assert(table->type == AT_TABLE);

    table_elem_t* elem = dyn_array_get(&table->a, index);
    assert(elem != NULL);
    return elem->key;
}

ako_elem_t* ako_elem_table_get_value_at(ako_elem_t* table, size_t index)
{
    assert(table != NULL);
    assert(table->type == AT_TABLE);

    table_elem_t* elem = dyn_array_get(&table->a, index);
    assert(elem != NULL);
    return elem->value;
}

void ako_elem_table_remove(ako_elem_t* table, const char* key)
{
    assert(table != NULL);
    assert(table->type == AT_TABLE);
    assert(key != NULL);

    table_elem_t* elem = NULL;
    size_t elem_idx = 0;

    dyn_array_t* array = &table->a;
    for (size_t i = 0; i < array->size; ++i)
    {
        table_elem_t* tableElem = dyn_array_get(array, i);
        if (strcmp(tableElem->key, key) == 0)
        {
            // found it!
            elem = tableElem;
            elem_idx = i;
        }
    }

    if (elem != NULL)
    {
        ako_free((void*)elem->key);
        ako_elem_destroy(elem->value);
        dyn_array_remove(array, elem_idx);
    }
}

bool ako_elem_table_contains(ako_elem_t* table, const char* key)
{
    assert(table != NULL);
    assert(table->type == AT_TABLE);
    assert(key != NULL);

    table_elem_t* elem = ako_table_find(table, key);
    return elem != NULL;
}

ako_elem_t* ako_elem_array_add(ako_elem_t* array, ako_elem_t* value)
{
    assert(array != NULL);
    assert(array->type == AT_ARRAY);
    assert(value != NULL);

    elem_t array_elem;
    array_elem.array.item = value;

    dyn_array_append(&array->a, &array_elem, sizeof(elem_t));
    return value;
}

ako_elem_t* ako_elem_array_get(ako_elem_t* array, size_t index)
{
    assert(array != NULL);
    assert(array->type == AT_ARRAY);
    assert(index < array->a.size);

    array_elem_t* elem = dyn_array_get(&array->a, index);
    assert(elem != NULL);
    return elem->item;
}

size_t ako_elem_array_get_length(ako_elem_t* array)
{
    assert(array != NULL);
    assert(array->type == AT_ARRAY);
    return array->a.size;
}

void ako_elem_array_remove(ako_elem_t* array, size_t index)
{
    assert(array != NULL);
    assert(array->type == AT_ARRAY);
    assert(index < array->a.size);

    array_elem_t* elem = dyn_array_get(&array->a, index);
    assert(elem != NULL);

    ako_elem_destroy(elem->item);
    dyn_array_remove(&array->a, index);
}

void ako_elem_set_null(ako_elem_t* elem)
{
    assert(elem != NULL);
    ako_elem_set_type(elem, AT_NULL);
}

void ako_elem_set_string(ako_elem_t* elem, const char* str)
{
    assert(elem != NULL);
    assert(str != NULL);
    ako_elem_set_type(elem, AT_STRING);
    if (elem->str != NULL)
    {
        ako_free((void*)elem->str);
    }
    elem->str = string_cpy(str);
}

void ako_elem_set_int(ako_elem_t* elem, ako_int value)
{
    assert(elem != NULL);
    ako_elem_set_type(elem, AT_INT);
    elem->i = value;
}

void ako_elem_set_float(ako_elem_t* elem, ako_float value)
{
    assert(elem != NULL);
    ako_elem_set_type(elem, AT_FLOAT);
    elem->f = value;
}

void ako_elem_set_shorttype(ako_elem_t* elem, const char* str)
{
    assert(elem != NULL);
    ako_elem_set_type(elem, AT_SHORTTYPE);
    elem->str = string_cpy(str);
}

void ako_elem_set_bool(ako_elem_t* elem, bool value)
{
    assert(elem != NULL);
    ako_elem_set_type(elem, AT_BOOL);
    elem->i = value ? 1 : 0;
}

const char* ako_elem_get_string(ako_elem_t* elem)
{
    assert(elem != NULL);
    assert((elem->type == AT_STRING || elem->type == AT_ERROR));
    return elem->str;
}

ako_int ako_elem_get_int(ako_elem_t* elem)
{
    assert(elem != NULL);
    assert(elem->type == AT_INT);
    return elem->i;
}

ako_float ako_elem_get_float(ako_elem_t* elem)
{
    assert(elem != NULL);
    assert(elem->type == AT_FLOAT);
    return elem->f;
}

const char* ako_elem_get_shorttype(ako_elem_t* elem)
{
    assert(elem != NULL);
    assert(elem->type == AT_SHORTTYPE);
    return elem->str;
}

bool ako_elem_get_bool(ako_elem_t* elem)
{
    assert(elem != NULL);
    assert(elem->type == AT_BOOL);
    if (elem->i == 0)
    {
        return false;
    }
    return true;
}

ako_elem_t* ako_elem_create_int(ako_int value)
{
    ako_elem_t* elem = ako_elem_create(AT_INT);
    ako_elem_set_int(elem, value);
    return elem;
}

ako_elem_t* ako_elem_create_float(ako_float value)
{
    ako_elem_t* elem = ako_elem_create(AT_FLOAT);
    ako_elem_set_float(elem, value);
    return elem;
}

ako_elem_t* ako_elem_create_string(const char* str)
{
    ako_elem_t* elem = ako_elem_create(AT_STRING);
    ako_elem_set_string(elem, str);
    return elem;
}

ako_elem_t* ako_elem_create_shorttype(const char* str)
{
    ako_elem_t* elem = ako_elem_create(AT_SHORTTYPE);
    ako_elem_set_shorttype(elem, str);
    return elem;
}

ako_elem_t* ako_elem_create_bool(bool value)
{
    ako_elem_t* elem = ako_elem_create(AT_BOOL);
    ako_elem_set_bool(elem, value);
    return elem;
}

ako_elem_t* ako_elem_create_error(const char* error)
{
    // Same as string but with a different type
    ako_elem_t* elem = ako_elem_create(AT_ERROR);
    elem->str = string_cpy(error);
    return elem;
}

ako_elem_t* ako_elem_create_errorf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    va_list args_copy;
    va_copy(args_copy, args);
    size_t len = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args);

    char* str = ako_malloc(len + 1);

    vsnprintf(str, len + 1, fmt, args);
    va_end(args);

    ako_elem_t* elem = ako_elem_create(AT_ERROR);
    elem->str = str;
    return elem;
}

ako_elem_t* ako_elem_get(ako_elem_t* root, const char* path)
{
    // When tokenizatio this will be our error checking
    // once done were using this for our current table
    ako_elem_t* elem;

    dyn_array_t tokens = ako_tokenize(path, &elem, true);
    if (elem != NULL)
    {
        ako_elem_destroy(elem);
        return NULL;
    }

    // We only support a small subset of the tokens
    //  We only support the following tokens: AKO_TT_IDENT AKO_TT_STRING AKO_TT_DOT AKO_TT_INT
    elem = root;
    for (size_t i = 0; i < tokens.size; ++i)
    {
        token_t* token = dyn_array_get(&tokens, i);
        if (token->type != AKO_TT_IDENT && token->type != AKO_TT_STRING && token->type != AKO_TT_DOT &&
            token->type != AKO_TT_INT)
        {
            // no clue what do with what ever token this is
            goto giveup;
        }

        bool is_end = false;
        if (i == tokens.size - 1)
        {
            is_end = true;
        }

        if (is_end)
        {
            if (ako_elem_get_type(elem) == AT_ARRAY)
            {
                // if its an array we only support int
                if (token->type != AKO_TT_INT)
                {
                    goto giveup;
                }
                elem = ako_elem_array_get(elem, token->value_int);
                goto valid;
            }
            else
            {
                // Only support string or ident at the end
                if (!(token->type == AKO_TT_STRING || token->type == AKO_TT_IDENT))
                {
                    goto giveup;
                }

                elem = ako_elem_table_get(elem, token->value_string);
                goto valid;
            }

            goto giveup;
        }

        if (ako_elem_get_type(elem) == AT_ARRAY)
        {
            if (token->type != AKO_TT_INT)
            {
                goto giveup;
            }

            elem = ako_elem_array_get(elem, token->value_int);
        }
        else
        {
            if (!(token->type == AKO_TT_STRING || token->type == AKO_TT_IDENT))
            {
                // Unsupported
                goto giveup;
            }

            elem = ako_elem_table_get(elem, token->value_string);
        }

        if (elem == NULL)
        {
            goto giveup;
        }

        // ensure our next token is a dot
        i++;
        token = dyn_array_get(&tokens, i);
        if (token->type != AKO_TT_DOT)
        {
            // Incorrect
            goto giveup;
        }
    }

giveup:
    ako_free_tokens(&tokens);
    return NULL;

valid:
    ako_free_tokens(&tokens);
    return elem;
}
