// Copyright (c) 2025 Tuyuji, Reece Hagan
// SPDX-License-Identifier: MIT
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ako/ako.h"
#include "token.h"

#if defined(_MSC_VER)
#include <malloc.h>
#define alloca _alloca
#endif

typedef struct state
{
    dyn_array_t tokens;
    const char *source;
    size_t source_len;
    size_t index;

    location_t meta;
    location_t current_loc;
} state_t;

static char consume(state_t *state)
{
    if (state->index >= state->source_len)
    {
        return '\0';
    }

    char next = state->source[state->index++];
    if (next == '\n' || next == '\t')
    {
        state->current_loc.line++;
        state->current_loc.column = 0;
    }

    state->current_loc.column++;
    state->current_loc.index = state->index;
    return next;
}

static bool has_value(state_t *state, size_t offset)
{
    if (state->index + offset >= state->source_len)
    {
        return false;
    }
    return true;
}

static char peek(state_t *state, size_t offset)
{
    if (!has_value(state, offset))
    {
        // cant peek what doesnt exist.
        assert(false);
    }

    return state->source[state->index + offset];
}

static void start_meta(state_t *state)
{
    state->meta = state->current_loc;
}

static void add_token(state_t *state, token_t token)
{
    token.start = state->meta;
    token.end = state->current_loc;
    DYN_APPEND(&state->tokens, token);
}

static bool is_valid_id(char in)
{
    return isalpha(in) || in == '_';
}

static size_t count_id(state_t *state)
{
    size_t offset = 0;
    while (has_value(state, offset))
    {
        char c = peek(state, offset);
        if (isalnum(c) || c == '_')
        {
            // good id!
            offset++;
        }
        else
        {
            break;
        }
    }
    return offset;
}

static size_t count_number(state_t *state)
{
    size_t offset = 0;
    while (has_value(state, offset))
    {
        char c = peek(state, offset);
        if (isdigit(c) || c == '.')
        {
            // good number!
            offset++;
        }
        else
        {
            break;
        }
    }
    return offset;
}

// Assumes the current offset is already in the starting quote
// Should be here when executing this function:
//  "Hello, World"
//	^
static size_t count_string(state_t *state)
{
    size_t offset = 0;
    while (has_value(state, offset))
    {
        char c = peek(state, offset);
        // anything is allowed pretty much other than real new lines
        if (c == '\n' || c == '\t')
        {
            // ignore REAL new lines
            continue;
        }
        // process escapes
        if (c == '\\')
        {
            offset++;
            offset++;
        }

        if (c == '"')
        {
            break;
        }

        offset++;
    }

    return offset;
}

size_t location_format(const location_t *loc, char *output, size_t output_size)
{
    // figure out how much space we need
    size_t size = snprintf(NULL, 0, "%zu:%zu", loc->line, loc->column);

    if (output == NULL)
    {
        return size;
    }

    if (output_size < size + 1)
    {
        // Not enough space, return 0
        return 0;
    }

    // We have enough space, write it out
    size_t written = snprintf(output, output_size, "%zu:%zu", loc->line, loc->column);
    return written;
}

static bool parse_digit(state_t *state, char **err)
{
    *err = NULL;
    token_t token;
    bool is_floating = false;
    size_t num_size = count_number(state);
    if (num_size == 0)
    {
        return false;
    }

    size_t num_bytesize = sizeof(char) * num_size + 1;
    char *num = alloca(num_bytesize);
    memset(num, '\0', num_bytesize);
    for (size_t i = 0; i < num_size; ++i)
    {
        num[i] = consume(state);
        if (num[i] == '.' && !is_floating)
        {
            is_floating = true;
        }
    }
    char *numerr = NULL;
    errno = 0;
    if (is_floating)
    {
        token.type = AKO_TT_FLOAT;
        token.value_float = strtod(num, &numerr);
    }
    else
    {
        token.type = AKO_TT_INT;
        token.value_int = strtoll(num, &numerr, 0);
    }

    if (errno != 0 || strlen(numerr) > 0)
    {
        // Failed
        sprintf(*err, "Failed to parse number at %zu:%zu", state->meta.line, state->meta.column);
        dyn_array_destroy(&state->tokens);
        return false;
    }

    add_token(state, token);
    return true;
}

dyn_array_t ako_tokenize(const char *source, char **err)
{
    static dyn_array_t empty_array = {0};

    state_t *state = alloca(sizeof(state_t));
    memset(state, 0, sizeof(state_t));
    state->tokens = dyn_array_create(sizeof(token_t));
    state->source = source;
    state->source_len = strlen(source);
    state->current_loc.line = 1;
    state->current_loc.column = 1;
    token_t token;
    memset(&token, 0, sizeof(token_t));

    while (has_value(state, 0))
    {
        char c = peek(state, 0);
        if (c == ' ' || c == '\n')
        {
            consume(state);
            continue;
        }

        if (c == '#')
        {
            // Comment, skip until new line
            consume(state);
            size_t comment_line = state->current_loc.line;
            while (state->current_loc.line == comment_line)
            {
                if (consume(state) == '\0')
                {
                    // No more text
                    break;
                }
            }
            continue;
        }

        start_meta(state);

        switch (c)
        {
        case '+':
            consume(state);
            token.type = AKO_TT_PLUS;
            add_token(state, token);
            continue;
        case '-':
            consume(state);
            token.type = AKO_TT_MINUS;
            add_token(state, token);
            continue;
        case ';':
            consume(state);
            token.type = AKO_TT_SEMICOLON;
            token.value_int = 0;
            add_token(state, token);
            continue;
        case '.':
            consume(state);
            token.type = AKO_TT_DOT;
            token.value_int = 0;
            add_token(state, token);
            continue;
        case '&':
            consume(state);
            token.type = AKO_TT_AND;
            token.value_int = 0;
            add_token(state, token);
            continue;
        case '[':
            consume(state);
            token.value_int = 0;
            if (has_value(state, 0) && peek(state, 0) == '[')
            {
                token.type = AKO_TT_OPEN_D_BRACE;
                consume(state);
            }
            else
            {
                token.type = AKO_TT_OPEN_BRACE;
            }
            add_token(state, token);
            continue;
        case ']':
            consume(state);
            token.value_int = 0;
            if (has_value(state, 0) && peek(state, 0) == ']')
            {
                token.type = AKO_TT_CLOSE_D_BRACE;
                consume(state);
            }
            else
            {
                token.type = AKO_TT_CLOSE_BRACE;
            }
            add_token(state, token);
            continue;
        }

        if (is_valid_id(c))
        {
            size_t id_size = count_id(state);
            size_t id_bytesize = sizeof(char) * id_size + 1;
            char *id = ako_malloc(id_bytesize);
            memset(id, '\0', id_bytesize);
            for (size_t i = 0; i < id_size; ++i)
            {
                id[i] = consume(state);
            }
            token.type = AKO_TT_IDENT;
            token.value_string = id;
            add_token(state, token);
            continue;
        }

        if (parse_digit(state, err))
        {
            // we parse a number
            // vectors can start now
            if (has_value(state, 0) && peek(state, 0) == 'x')
            {
                while (has_value(state, 0) && peek(state, 0) == 'x')
                {
                    location_t vector_delimiter = state->current_loc;
                    consume(state);
                    token.type = AKO_TT_VECTORCROSS;
                    token.value_int = 0;
                    add_token(state, token);

                    if (!parse_digit(state, err))
                    {
                        // Failed to parse number and we had an X before, this isn't valid
                        sprintf(*err, "Failed to parse vector at %zu:%zu", vector_delimiter.line,
                                vector_delimiter.column);
                        dyn_array_destroy(&state->tokens);
                        return empty_array;
                    }
                }
                // Loop done
                continue;
            }
            else
            {
                // normal digit
                continue;
            }
        }
        else
        {
            // if theres an error set then we failed to parse in a bad way
            if (*err != NULL)
            {
                // Failed to parse number
                sprintf(*err, "Failed to parse number at %zu:%zu", state->meta.line, state->meta.column);
                dyn_array_destroy(&state->tokens);
                return empty_array;
            }
        }

        if (c == '"')
        {
            consume(state);
            size_t char_count = count_string(state);
            size_t str_bytesize = sizeof(char) * char_count + 1;
            char *str = ako_malloc(str_bytesize);
            memset(str, '\0', str_bytesize);

            char *iter = str;
            while (has_value(state, 0))
            {
                char cc = consume(state);
                if (cc == '\\')
                {
                    cc = consume(state);
                    switch (cc)
                    {
                    case 'n':
                        *iter = '\n';
                        break;
                    case 't':
                        *iter = '\t';
                        break;
                    default:
                        *iter = cc;
                        break;
                    }
                    iter++;
                    continue;
                }
                if (cc == '"')
                {
                    break;
                }
                *iter = cc;
                iter++;
            }

            token.type = AKO_TT_STRING;
            token.value_string = str;
            add_token(state, token);
            continue;
        }
    }

    return state->tokens;
}

void ako_free_tokens(dyn_array_t *tokens)
{
    for (size_t i = 0; i < tokens->size; ++i)
    {
        token_t *token = dyn_array_get(tokens, i);
        if (token->type == AKO_TT_STRING || token->type == AKO_TT_IDENT)
        {
            ako_free((void *)token->value_string);
        }
    }

    dyn_array_destroy(tokens);
}