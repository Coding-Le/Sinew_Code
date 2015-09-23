#ifndef TYPE_H
#define TYPE_H
#include "jsmn.h"

typedef enum { STRING = 1,INTEGER, FLOAT,BOOLEAN,DOCUMENT,ARRAY,NONE} json_typeid;

#define STRING_TYPE "text"
#define INTEGER_TYPE "int"
#define FLOAT_TYPE "double"
#define BOOLEAN_TYPE "bool"
#define DOCUMENT_TYPE "object"
#define ARRAY_TYPE "[]"
#define NULL_TYPE "null"

json_typeid jsmn_primitive_get_type(char *value_str);
json_typeid jsmn_get_type(jsmntok_t* tok, char *json);

char *jsmntok_to_str(jsmntok_t *tok, char *json);
jsmntok_t *jsmn_tokenize(char *json);

json_typeid get_json_type(const char *pg_type);
char *get_pg_type(json_typeid type, char *value);
char *get_pg_type_for_path(char **path,char *path_arr_index_map,int depth,char *base_type);
#endif
