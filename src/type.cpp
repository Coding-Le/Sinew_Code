#include "type.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

char *jsmntok_to_str(jsmntok_t *tok, char *json)
{
    char *retval;
    int   len;
    assert(tok && json);

    len = tok->end - tok->start;
    retval = strndup(&json[tok->start], len);

    return retval;
}

json_typeid jsmn_primitive_get_type(char *value_str)
{
    char *ptr;
    switch(value_str[0]) {
    case 't': case 'f':
        return BOOLEAN;
    case 'n':
        return NONE;
    case '-':
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        ptr = NULL;
        if ((ptr = strchr(value_str, '.')) && !strchr(ptr + 1, '.')) 
        {
            return FLOAT;
        }
        else
        {
            return INTEGER;
        }
    default:
        return NONE;
    }
}

json_typeid jsmn_get_type(jsmntok_t* tok, char *json) {
    jsmntype_t tok_type = tok->type;
    char *value_str = jsmntok_to_str(tok, json);

    if (!value_str) {
        return NONE;
    }

    switch (tok_type)
    {
    case JSMN_STRING:
        return STRING;
    case JSMN_PRIMITIVE:
        return jsmn_primitive_get_type(value_str);
    case JSMN_OBJECT:
        return DOCUMENT;
    case JSMN_ARRAY:
        return ARRAY;
    default:
        return NONE;
    }
}


jsmntok_t *jsmn_tokenize(char *json) {
    jsmn_parser parser;
    jsmntok_t *tokens;
    unsigned maxToks;
    int status;

    jsmn_init(&parser);
    maxToks = 256;
    tokens = (jsmntok_t*)calloc(sizeof(jsmntok_t), maxToks);
    assert(tokens);

    if (json == NULL)
    {
        assert(0);
        return NULL;
    }
    status = jsmn_parse(&parser, json, tokens, maxToks);
    while (status == JSMN_ERROR_NOMEM)
    {
        maxToks = maxToks * 2 + 1;
        tokens = (jsmntok_t*)realloc(tokens, sizeof(jsmntok_t) * maxToks);
        assert(tokens);
        status = jsmn_parse(&parser, json, tokens, maxToks);
    }
    return tokens;
}

char *get_pg_type(json_typeid type, char *value) {
    json_typeid arr_elt_type;
    char *arr_elt_pg_type;
    char *buffer;
    jsmntok_t *tokens;

    assert(value);
    arr_elt_type = NONE;

    switch (type)
    {
        case STRING:
            return STRING_TYPE;
        case INTEGER:
            return INTEGER_TYPE;
        case FLOAT:
            return FLOAT_TYPE;
        case BOOLEAN:
            return BOOLEAN_TYPE;
        case DOCUMENT:
            return DOCUMENT_TYPE;
        case ARRAY:
            tokens = jsmn_tokenize(value);
            assert(tokens->type == JSMN_ARRAY);
            arr_elt_type = jsmn_get_type(tokens + 1, value);
            arr_elt_pg_type = get_pg_type(arr_elt_type, jsmntok_to_str(tokens + 1, value));
            buffer = (char*)calloc(1, strlen(arr_elt_pg_type) + 2 + 1);
            
            sprintf(buffer, "%s%s", arr_elt_pg_type, ARRAY_TYPE);
            return buffer;
        case NONE:
            return NULL_TYPE;
        default:
            fprintf(stderr, "document: invalid type id on serialization");
    }
}

char *get_pg_type_for_path(char **path,char *path_arr_index_map,int depth,char *base_type) {
    if (depth == 1)
    {
        return base_type;
    }
    else {
        if (path_arr_index_map[1])
        {
            char *array_pg_type;
            char *buffer;

            array_pg_type = get_pg_type_for_path(path + 1,path_arr_index_map + 1,--depth,base_type);
            buffer = (char*)calloc(1, strlen(array_pg_type) + 2 + 1);
            sprintf(buffer, "%s%s", array_pg_type, ARRAY_TYPE);
            return buffer;
        }
        else{
            return DOCUMENT_TYPE;
        }
    }
}

json_typeid get_json_type(const char *pg_type) {
    if (!strcmp(pg_type, STRING_TYPE))
    {
        return STRING;
    }
    else if (!strcmp(pg_type, INTEGER_TYPE))
    {
        return INTEGER;
    }
    else if (!strcmp(pg_type, FLOAT_TYPE))
    {
        return FLOAT;
    }
    else if (!strcmp(pg_type, BOOLEAN_TYPE))
    {
        return BOOLEAN;
    }
    else if (!strcmp(pg_type, DOCUMENT_TYPE))
    {
        return DOCUMENT;
    }
    else
    {
        int len;
        len = strlen(pg_type);
        if (len > 2 && pg_type[len-2] == '[' && pg_type[len-1] == ']')
        {
            return ARRAY;
        }
        else {
            return NONE;
        }
    }
}
