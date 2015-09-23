#include <string.h>
#include <stdlib.h>
#include "transfer.h"
#include "catalog.h"
#include"type.h"
#include <assert.h>

void json_to_document(char *json, document *doc) {
    document c;
    int natts;
    int capacity;
    jsmntok_t *tokens;
    size_t i, j;
    typedef enum { START, KEY, VALUE } parse_state;
    parse_state state;

    assert(json);
    assert(doc);

    tokens = jsmn_tokenize(json);
    natts = 0;

    state = START;
    for (i = 0, j = 1; j > 0; ++i, --j)
    {
        jsmntok_t *curtok;
        char *keyname;
        char *value;
        json_typeid type;

        curtok = tokens + i;
        type = NONE;

        switch (state)
        {
        case START:
            assert(curtok->type == JSMN_OBJECT);

            capacity = curtok->size;
            doc->keys = (char**)calloc(capacity, sizeof(char*));
            doc->types = (json_typeid*)calloc(capacity, sizeof(int));
            doc->values = (char**)calloc(capacity, sizeof(char*));

            j += capacity;
            state = KEY;
            break;
        case KEY:
            keyname = jsmntok_to_str(curtok, json);
            state = VALUE;
            break;
        case VALUE:
            value = jsmntok_to_str(curtok, json);
            type = jsmn_get_type(curtok, json);
            if (type == NONE) 
            {
                state = KEY;
                break;
            }

            if (curtok->type == JSMN_ARRAY)
            {
                jsmntok_t *tok;
                int end;

                tok = curtok;
                end = curtok->end;
                while (tok->start <= end && j > 1) {
                    ++i;
                    tok = tokens + i;
                }
                --i; 
            }
            else if (curtok->type == JSMN_OBJECT)
            {
             
                jsmntok_t *tok;
                int end;

                tok = curtok;
                end = curtok->end;
                while (tok->start <= end && j > 1) {
                    ++i;
                    tok = tokens + i;
                }
                --i; 
            }

            doc->keys[natts] = keyname;
            doc->types[natts] = type;
            doc->values[natts] = value;
          

            ++natts;
            if (natts > capacity) {
                capacity = 2 * natts;
                doc->keys = (char**)realloc(doc->keys, capacity * sizeof(char*));
                doc->types = (json_typeid*)realloc(doc->types, capacity * sizeof(int));
                doc->values = (char**)realloc(doc->values, capacity * sizeof(char*));
            }

            state = KEY;
            break;
        }
    }
    free(tokens);
    doc->natts = natts;
}

int array_to_binary(char *json_arr, char **outbuff_ref) {
    jsmntok_t *tokens;
    int data_size;
    int buffpos;
    int arrlen;
    json_typeid arrtype;
    char *outbuff;
    jsmntok_t *curtok; 
    int i;

    tokens = jsmn_tokenize(json_arr);
    assert(tokens->type == JSMN_ARRAY);
    outbuff = *outbuff_ref;

    arrlen = tokens->size;
    
    arrtype = jsmn_get_type(tokens + 1, json_arr);

    data_size = 2 * arrlen * sizeof(int) + 1024;
    outbuff = (char*)calloc(data_size, 1);
    buffpos = 0;

    memcpy(outbuff + buffpos, &arrlen, sizeof(int));
    buffpos += sizeof(int);
    memcpy(outbuff + buffpos, &arrtype, sizeof(int));
    buffpos += sizeof(int);

    curtok = tokens + 1;
    for (i = 0; i < arrlen; i++)
    {
        char *binary;
        int datum_size;

        if (jsmn_get_type(curtok, json_arr) != arrtype)
        {
            
            free(outbuff);
            outbuff = NULL;
            return 0;
        }

        datum_size = to_binary(arrtype, jsmntok_to_str(curtok, json_arr), &binary);
        memcpy(outbuff + buffpos, &datum_size, sizeof(int));
        buffpos += sizeof(int);
        memcpy(outbuff + buffpos, binary, datum_size);
        buffpos += datum_size;

        if (buffpos >= data_size)
        {
            data_size = 2 * buffpos + 1;
            outbuff = (char*)realloc(outbuff, data_size);
        }

        free(binary);
        if (curtok->type == JSMN_ARRAY || curtok->type == JSMN_OBJECT)
        {
            int end;

            end = curtok->end;
           
            while (curtok->start <= end && i < arrlen - 1) ++curtok;
           
        }
        else
        {
            ++curtok;
        }
    }

    free(tokens);

    *outbuff_ref = outbuff;

    return buffpos;
}

int document_to_binary(char *json, char **outbuff_ref) {
    document doc;
    int natts;
    char *outbuff;
    int  *attr_ids;
    int **attr_id_refs; 
    int i;
    int data_size;
    int buffpos;

    json_to_document(json, &doc);
    natts = doc.natts;
    attr_ids = (int*)calloc(natts, sizeof(int));
    attr_id_refs = (int**)calloc(natts, sizeof(int*));

    outbuff = *outbuff_ref;

    for (i = 0; i < natts; i++)
    {
        const char *type;
        type = get_pg_type(doc.types[i], doc.values[i]);
        attr_ids[i] = get_attribute_id(doc.keys[i], type);
        if (attr_ids[i] < 0)
        {
            attr_ids[i] = add_attribute(doc.keys[i], type);
        } else {

        }
        attr_id_refs[i] = attr_ids + i;
    }
    qsort(attr_id_refs, natts, sizeof(int*), intref_comparator);

    data_size = 2 * natts * sizeof(int) + 1024;
    buffpos = 0;
    outbuff = (char*)calloc(data_size, 1);
    memcpy(outbuff, &natts, sizeof(int));
    buffpos += sizeof(int);
    for (i = 0; i < natts; i++)
    {
        memcpy(outbuff + buffpos, attr_id_refs[i], sizeof(int));
        buffpos += sizeof(int);
    }
    buffpos = sizeof(int) + 2 * sizeof(int) * natts +
        sizeof(int);                    //  attrs, attr_ids, offsets, value
    for (i = 0; i < natts; i++)
    {
        char *binary;
        int *attr_id_ref;
        int orig_pos;
        int datum_size;

        attr_id_ref = attr_id_refs[i];
        orig_pos = attr_id_ref - attr_ids;
        datum_size = to_binary(doc.types[orig_pos], doc.values[orig_pos],&binary);
        if (buffpos + datum_size >= data_size)
        {
            data_size = 2 * (buffpos + datum_size) + 1;
            outbuff = (char*)realloc(outbuff, data_size);
        }
        memcpy(outbuff + buffpos, binary, datum_size);
        memcpy(outbuff + (1 + natts + i) * sizeof(int), &buffpos,
            sizeof(int));    //attrs, attr_ids, ith offset

        buffpos += datum_size;
        free(binary);
    }
    memcpy(outbuff + (1 + 2 * natts) * sizeof(int), &buffpos, sizeof(int));
    //printf("%d ", &buffpos);
    free(attr_ids);
    free(attr_id_refs);

    *outbuff_ref = outbuff;

    return buffpos;
}

int to_binary(json_typeid id, char *value, char **outbuff_ref) {
    char *outbuff;

    outbuff = *outbuff_ref;

    switch (id)
    {
    case STRING:
        outbuff = strndup(value, strlen(value));  //strlen  , no '\0'
        *outbuff_ref = outbuff;
        return strlen(value);
    case INTEGER:
        outbuff = (char*)malloc(sizeof(int));  //32bits to store integer
        *((int*)outbuff) = atoi(value);
        *outbuff_ref = outbuff;
        return sizeof(int);

    case FLOAT:
        outbuff = (char*)malloc(sizeof(double));
        *((double*)outbuff) = atof(value);
        *outbuff_ref = outbuff;
        return sizeof(double);
    case BOOLEAN:

        outbuff = (char*)malloc(1);
        if (!strcmp(value, "true")) {
            *outbuff = 1;
        } else if (!strcmp(value, "false")) {
            *outbuff = 0;
        } else {
            return -1;
        }
        *outbuff_ref = outbuff;
        return 1;
    case DOCUMENT:
        return document_to_binary(value, outbuff_ref);
    case ARRAY:
        return array_to_binary(value, outbuff_ref);
    case NONE:
    default:
        return -1;
    }
    return -1;
}


void binary_to_document(char *binary, document *doc)
{
    FILE* demand;
    demand = fopen("insert to pdf form", "a");
    int natts;
    int buffpos;
    char **keys;
    json_typeid *types;
    char **values;
    int i, flag;
    flag = 0;
    assert(binary);

    memcpy(&natts, binary, sizeof(int));
    buffpos = sizeof(int);

    keys = (char**)calloc(natts, sizeof(char*));
    values = (char**)calloc(natts, sizeof(char*));
    types = (json_typeid*)calloc(natts, sizeof(json_typeid));


    if (natts < 3) {
        flag = 1;
    }
    if (!flag) {
    fprintf(demand, "attributes_num:%d |", natts);
     }

    for (i = 0; i < natts; i++)
    {
        int id;
        char *key_string;
        char *type_string;

        memcpy(&id, binary + buffpos, sizeof(int));
        get_attr(id, &key_string, &type_string);       //get the corresponding id
        
        if (!flag) {
        fprintf(demand, "aid%d:%d |", i, id);
        if (i == natts-1) {
            fprintf(demand,"\n");
        }
        }



        keys[i] = strndup(key_string, strlen(key_string));
        types[i] = get_json_type(type_string);

        free(key_string);
        free(type_string);

        buffpos += sizeof(int);
    }
    
    for (i = 0; i < natts; i++)    //len refers to the lastest position
    {
        int start, end;
        char *value_data;

        memcpy(&start, binary + buffpos, sizeof(int));
        memcpy(&end, binary + buffpos + sizeof(int), sizeof(int));
        if (!flag) {
        fprintf(demand, "offs%d:%d |", i, start);
        if (i == natts-1) {
            fprintf(demand, "len:%d\n", end);
        }
        }


        value_data = (char*)calloc(end - start, 1);
        memcpy(value_data, binary + start, end - start);
       
        values[i] = binary_to_string(types[i], value_data, end - start);
        free(value_data);

        buffpos += sizeof(int);
    }

    doc->natts = natts;
    doc->keys = keys;
    doc->types = types;
    doc->values = values;        //key type value natts

    if (!flag) {
    for (int i = 0; i < natts; i++) {
        fprintf(demand, "%s#", values[i]);
    }
    fprintf(demand, "\n");
    }
    fclose(demand);
}

char *binary_document_to_string(char *binary) {
    document doc;
    int natts;
    char *result;
    int result_size, result_maxsize;
    int i; 

    binary_to_document(binary, &doc);
  
    natts = doc.natts;

    result_size = 3;
    result_maxsize = 64; 
    result = (char*)calloc(result_maxsize + 1, 1);
    strcat(result, "{");

    for (i = 0; i < natts; i++)
    {
        char *key;
        char *value;
        char *attr;
        int attr_len;

        key = doc.keys[i];
        value = doc.values[i];

        attr_len = strlen(key) + strlen(value) + 7; /* "k":v,\n" */
        attr = (char*)calloc(attr_len + 1, 1);
        if (i == natts-1) {
            sprintf(attr, "\"%s\": %s", key, value);
        } else {
            sprintf(attr, "\"%s\": %s, ", key, value);
        }
       

        if (result_size + attr_len + 1 >= result_maxsize)
        {
            result_maxsize = 2 * (result_size + attr_len) + 1;
            result = (char*)realloc(result, result_maxsize + 1);
        }
        strcat(result, attr);
        result_size += attr_len;
    }
    strcat(result, "},");
    return result;
}

char *binary_array_to_string(char *binary)
{
    int buffpos;
    int natts;
    json_typeid type;
    char *result;
    int result_size, result_maxsize;
    int i;

    assert(binary);
    memcpy(&natts, binary, sizeof(int));
    memcpy(&type, binary + sizeof(int), sizeof(int));
    buffpos = 2 * sizeof(int);

    result_size = 2; /* '[]' */
    result_maxsize = 64;
    result = (char*)calloc(result_maxsize + 1, 1);
    strcat(result, "[");

    for (i = 0; i < natts; i++) {
        int elt_size;
        char *elt;

        memcpy(&elt_size, binary + buffpos, sizeof(int));
        if (result_size + elt_size + 2 + 1 >= result_maxsize) {
            result_maxsize = 2 * (result_size + elt_size + 2) + 1;
            result = (char*)realloc(result, result_maxsize + 1);
        }

        result_size += sizeof(int);
        buffpos += sizeof(int);

        if (i != 0) {
           strcat(result, ", ");
        }

        elt = (char*)calloc(elt_size, 1);
        memcpy(elt, binary + buffpos, elt_size);
        strcat(result, binary_to_string(type, elt, elt_size));
        free(elt);

        result_size += elt_size;
        buffpos += elt_size;
    }
    strcat(result, "]"); 

    return result;
}

char *binary_to_string(json_typeid type, char *binary, int datum_len)
{
    int i;
    double d;
    char *temp;
    char *result;
    assert(binary);
   
    result = (char*)calloc(datum_len * 8 + 2 + 1, 1); 

    switch (type)
    {
        case STRING:
            temp = strndup(binary, datum_len);
            sprintf(result, "\"%s\"", temp);
            free(temp);
            return result;
        case INTEGER:
            assert(datum_len == sizeof(int));
            memcpy(&i, binary, sizeof(int));
            sprintf(result, "%d", i);
            return result;
        case FLOAT:
            assert(datum_len == sizeof(double));
            memcpy(&d, binary, sizeof(double));
            // sprintf(result, "%f", *(double*)binary)
            sprintf(result, "%f", d);
            return result;
        case BOOLEAN:
            assert(datum_len == 1);
            sprintf(result, "%s", *binary != 0 ? "true" : "false");
            return result;
        case DOCUMENT:
            return binary_document_to_string(binary);
        case ARRAY:
           
            return binary_array_to_string(binary);
        case NONE:
        default:
            fprintf(stderr, "document: invalid binary");
            return NULL;
    }

}

int int_comparator(const void *v1, const void *v2) {
    int i1, i2;

    i1 = *(int*)v1;
    i2 = *(int*)v2;

    if (i1 < i2)
    {
        return -1;
    }
    else if (i1 == i2)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

int intref_comparator(const void *v1, const void *v2)
{
    int *i1, *i2;

    i1 = *(int**)v1;
    i2 = *(int**)v2;

    return int_comparator((void*)i1, (void*)i2);
}
