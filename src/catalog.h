#ifndef CATALOG_H
#define CATALOG_H
#include <stdio.h>
typedef struct element element_t;
typedef struct table table_t;
static int num_keys = 0;   //number of attributes
static char **key_names = NULL;
static char **key_types = NULL;
static int* key_count = NULL;
static table_t *attr_table = NULL;

table_t *make_table(void);
const int my_get(table_t* ht, char* key);
const int get_count(table_t* ht, char* key);
const element_t *put(table_t *ht, char *key, int val, int count);
const int get(table_t *ht, char *key);
void get_attr(int id,char **key_name_ref,char **type_name_ref);
int get_attribute_id(const char *key_name, const char *type_name);
int get_myattribute_id(const char *attr);
int add_attribute(const char *key_name, const char *type_name);
#endif
