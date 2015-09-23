#ifndef TRANSFER_H
#define TRANSFER_H
#include "type.h"
typedef struct {
    int        natts;
    char     **keys;
    json_typeid *types;
    char     **values;
} document;

// the following four functions transfer json file into binary file
void json_to_document(char *json, document *doc);
int array_to_binary(char *json_arr, char **outbuff_ref);
int document_to_binary(char *json, char **outbuff_ref);
int to_binary(json_typeid type, char *value, char **outbuff_ref);

// the following four function transfer binary file back to json file
void binary_to_document(char *binary, document *doc);
char *binary_document_to_string(char *binary);
char *binary_array_to_string(char *binary);
char *binary_to_string(json_typeid type, char *binary, int datum_len);

int int_comparator(const void *v1, const void *v2);
int intref_comparator(const void *v1, const void *v2);

#endif
