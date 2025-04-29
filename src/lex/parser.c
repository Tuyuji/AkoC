// Copyright (c) 2025 Tuyuji, Reece Hagan
// SPDX-License-Identifier: MIT
#include "parser.h"
#include "../private.h"

#include <assert.h>
#include <string.h>

#include "../mem/dyn_string.h"
#include "ako/ako.h"

typedef struct
{
    dyn_array_t* tokens;
    size_t index;
} state_t;

static token_t* _consume(state_t* state)
{
    if (state->index >= state->tokens->size)
    {
        return NULL;
    }

    token_t* token = dyn_array_get(state->tokens, state->index);
    state->index++;
    return token;
}

static token_t* peek(state_t* state, size_t offset)
{
    if (state->index + offset >= state->tokens->size)
    {
        return NULL;
    }

    return dyn_array_get(state->tokens, state->index + offset);
}

static bool __check_peek_type(state_t* state, size_t offset, token_type_t type)
{
    token_t* token = peek(state, offset);
    if (token == NULL)
    {
        return false;
    }
    return token->type == type;
}

#define CHECK_TYPE(token, checktype) (token != NULL && token->type == checktype)

static ako_elem_t* _parse_array(state_t* state);
static ako_elem_t* _parse_table(state_t* state, bool should_ignore_braces);
static ako_elem_t* _parse_value(state_t* state);

static ako_elem_t* _parse_value(state_t* state)
{
    token_t* peeked = peek(state, 0);
    location_t start_loc;
    dyn_string_t str; // Used in short type
    ako_elem_t* ret;
    switch (peeked->type)
    {
    case AKO_TT_OPEN_D_BRACE:
        return _parse_array(state);
    case AKO_TT_OPEN_BRACE:
        return _parse_table(state, false);
    case AKO_TT_SEMICOLON:
        _consume(state);
        return ako_elem_create(AT_NULL);
    case AKO_TT_BOOL:
        _consume(state);
        return ako_elem_create_bool(peeked->value_int);
    case AKO_TT_INT:
    case AKO_TT_FLOAT:
        start_loc = peeked->start;
        if (CHECK_TYPE(peek(state, 1), AKO_TT_VECTORCROSS))
        {
            ako_elem_t* array = ako_elem_create(AT_ARRAY);
            // we can override peeked at this point since we will either error out or return the array
            while (peek(state, 0) != NULL)
            {
                peeked = peek(state, 0);
                bool is_int_float = (peeked->type == AKO_TT_INT || peeked->type == AKO_TT_FLOAT);
                if (!is_int_float)
                {
                    return ako_elem_create_errorf("Trying to use non vector type in vector at %zu:%zu", start_loc.line,
                                                  start_loc.column);
                }

                peeked = _consume(state);
                ako_elem_t* elem = NULL;
                if (peeked->type == AKO_TT_INT)
                {
                    elem = ako_elem_create_int(peeked->value_int);
                }
                else
                {
                    elem = ako_elem_create_float(peeked->value_float);
                }

                ako_elem_array_add(array, elem);
                peeked = peek(state, 0);
                if (CHECK_TYPE(peeked, AKO_TT_VECTORCROSS))
                {
                    // Should continue
                    _consume(state);
                }
                else
                {
                    // No continue to vector, return
                    if (ako_elem_array_get_length(array) > 4)
                    {
                        return ako_elem_create_errorf("Vector size is greater than 4 at %zu:%zu", start_loc.line,
                                                      start_loc.column);
                    }
                    return array;
                }
            }
        }

        _consume(state);
        if (peeked->type == AKO_TT_INT)
        {
            return ako_elem_create_int(peeked->value_int);
        }
        else
        {
            return ako_elem_create_float(peeked->value_float);
        }
    case AKO_TT_STRING:
        _consume(state);
        return ako_elem_create_string(peeked->value_string);
    case AKO_TT_AND:
        // need identifier next
        if (!CHECK_TYPE(peek(state, 1), AKO_TT_IDENT))
        {
            // Invalid start
            return ako_elem_create_errorf("ShortType needs to start with an Identifier, error at %zu:%zu",
                                          peeked->start.line, peeked->start.column);
        }

        _consume(state);
        peeked = peek(state, 0);
        str = dyn_string_create(strlen(peeked->value_string));

        while (CHECK_TYPE(peek(state, 0), AKO_TT_IDENT))
        {
            dyn_string_append(&str, _consume(state)->value_string);
            peeked = peek(state, 0);
            if (peeked == NULL || peeked->type != AKO_TT_DOT)
            {
                break;
            }
            // consume dot
            _consume(state);
            dyn_string_append(&str, ".");
        }

        ret = ako_elem_create_shorttype(str.data);
        ako_free(str.data);
        return ret;

    default:
        return ako_elem_create_errorf("Unsupported type at %zu:%zu -> %zu:%zu", peeked->start.line,
                                      peeked->start.column, peeked->end.line, peeked->end.column);
    }

    return ako_elem_create_errorf("Unexpected escape from switch statement.");
}

static ako_elem_t* _parse_table_element(state_t* state, ako_elem_t* table)
{
    assert(state != NULL);
    assert(table != NULL);
    assert(table->type == AT_TABLE);

    token_t* peeked = peek(state, 0);
    if (peeked == NULL)
    {
        return ako_elem_create_error("Unexpected end of table element.");
    }

    token_t* value_first = NULL;
    if (CHECK_TYPE(peeked, AKO_TT_MINUS) || CHECK_TYPE(peeked, AKO_TT_PLUS) || CHECK_TYPE(peeked, AKO_TT_SEMICOLON))
    {
        value_first = _consume(state);
    }

    // Ensure we have a identifier or string.
    {
        peeked = peek(state, 0);
        bool is_valid_token = (peeked->type == AKO_TT_IDENT || peeked->type == AKO_TT_STRING);
        if (!is_valid_token)
        {
            return ako_elem_create_error("Expected an identifier or string.");
        }
    }

    ako_elem_t* current_table = table;
    const char* ct_id = NULL;

    while (peek(state, 0) != NULL &&
           (CHECK_TYPE(peek(state, 0), AKO_TT_IDENT) || CHECK_TYPE(peek(state, 0), AKO_TT_STRING)))
    {
        const char* id = _consume(state)->value_string;
        bool still_more = __check_peek_type(state, 0, AKO_TT_DOT);

        if (!still_more)
        {
            // At the last identifier
            // We can get the value from the table
            ct_id = id;
            /*ct_value = ako_elem_table_get(current_table, id);
            if (ct_value == NULL)
            {
                ct_value = ako_elem_create(AT_NULL);
                ako_elem_table_add(current_table, id, ct_value);
            }*/
            break;
        }
        else
        {
            // Not the last id
            // see if the id in the table, id not create a new table there
            ako_elem_t* test = ako_elem_table_get(current_table, id);
            if (test == NULL)
            {
                test = ako_elem_create(AT_TABLE);
                ako_elem_table_add(current_table, id, test);
            }

            current_table = test;
        }

        peeked = peek(state, 0);
        if (CHECK_TYPE(peeked, AKO_TT_DOT))
        {
            _consume(state);
        }
    }

    if (ct_id == NULL)
    {
        return ako_elem_create_error("Failed to get table id.");
    }

    if (value_first != NULL)
    {
        // value is first
        switch (value_first->type)
        {
        case AKO_TT_PLUS:
        case AKO_TT_MINUS:
            ako_elem_table_add(current_table, ct_id, ako_elem_create_bool(value_first->value_int));
            break;
        case AKO_TT_SEMICOLON:
            ako_elem_table_add(current_table, ct_id, ako_elem_create(AT_NULL));
            break;
        default:
            return ako_elem_create_error("Unknown value type.");
        }
    }
    else
    {
        ako_elem_t* value = _parse_value(state);
        if (ako_elem_is_error(value))
        {
            // Uh oh, just return the error
            // Don't destroy any tables as who ever called this function will destroy the tree anyway.
            return value;
        }

        ako_elem_table_add(current_table, ct_id, value);
    }

    return NULL;
}

static ako_elem_t* _parse_table(state_t* state, bool should_ignore_braces)
{
    token_t* peeked = peek(state, 0);
    if (!should_ignore_braces)
    {
        if (CHECK_TYPE(peeked, AKO_TT_OPEN_BRACE))
        {
            _consume(state);
        }
        else
        {
            return ako_elem_create_error("Expected an opening brace.");
        }
    }

    ako_elem_t* table = ako_elem_create(AT_TABLE);
    while (peek(state, 0) != NULL && peek(state, 0)->type != AKO_TT_CLOSE_BRACE)
    {
        // We need {id, value} or {value, id} pairs
        peeked = peek(state, 0);
        if (peeked == NULL || peek(state, 1) == NULL)
        {
            return ako_elem_create_error("Expected two tokens, got zero/one.");
        }

        bool has_valid_first_token =
            (peeked->type == AKO_TT_IDENT || peeked->type == AKO_TT_STRING || peeked->type == AKO_TT_PLUS ||
             peeked->type == AKO_TT_MINUS || peeked->type == AKO_TT_SEMICOLON);

        if (!has_valid_first_token)
        {
            ako_elem_destroy(table);
            return ako_elem_create_errorf("Expected an identifier, bool or null but got: %s",
                                          TokenType_Strings[peeked->type]);
        }

        ako_elem_t* err = _parse_table_element(state, table);
        if (err != NULL)
        {
            // Uh oh, just return the error
            ako_elem_destroy(table);
            return err;
        }
    }

    if (!should_ignore_braces)
    {
        peeked = peek(state, 0);
        if (CHECK_TYPE(peeked, AKO_TT_CLOSE_BRACE))
        {
            _consume(state);
            return table;
        }

        return ako_elem_create_error("Expected a closing brace.");
    }
    return table;
}

static ako_elem_t* _parse_array(state_t* state)
{
    token_t* peeked = peek(state, 0);
    if (CHECK_TYPE(peeked, AKO_TT_OPEN_D_BRACE))
    {
        _consume(state);
    }
    else
    {
        if (peeked == NULL)
        {
            return ako_elem_create_error("Unexpected end of array.");
        }
        return ako_elem_create_errorf("Open double brace expected at %zu:%zu -> %zu:%zu", peeked->start.line,
                                      peeked->start.column, peeked->end.line, peeked->end.column);
    }

    ako_elem_t* array = ako_elem_create(AT_ARRAY);

    while (peek(state, 0) != NULL && peek(state, 0)->type != AKO_TT_CLOSE_D_BRACE)
    {
        ako_elem_t* elem = _parse_value(state);
        if (elem == NULL)
        {
            ako_elem_destroy(array);
            return ako_elem_create_error("Failed to parse array element.");
        }

        if (ako_elem_is_error(elem))
        {
            // Uh oh, just return the error
            ako_elem_destroy(array);
            return elem;
        }

        ako_elem_array_add(array, elem);
    }

    peeked = peek(state, 0);
    if (CHECK_TYPE(peeked, AKO_TT_CLOSE_D_BRACE))
    {
        _consume(state);
        return array;
    }

    return ako_elem_create_error("Expected a closing double brace.");
}

ako_elem_t* ako_parse_tokens(dyn_array_t* tokens)
{
    state_t state;
    state.tokens = tokens;
    state.index = 0;

    token_t* peeked = peek(&state, 0);
    if (peeked == NULL)
    {
        return ako_elem_create_error("No tokens to parse");
    }

    if (peeked->type == AKO_TT_OPEN_D_BRACE)
    {
        // array
        return _parse_array(&state);
    }
    else
    {
        bool should_ignore_braces = true;
        if (peeked->type == AKO_TT_OPEN_BRACE)
        {
            should_ignore_braces = false;
        }

        return _parse_table(&state, should_ignore_braces);
    }

    return NULL;
}
