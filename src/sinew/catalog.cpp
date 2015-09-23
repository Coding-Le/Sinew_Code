#include "type.h"
#include "catalog.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

struct element {
    char* key;
    int value;
    int count;
    struct element* next;
};

struct table {
    size_t num_elem;
    size_t size;
    element_t** entries;
};

#define MULTIPLIER (37)

static size_t
hash(const char* key)
{
    unsigned const char *uKey;
    size_t val;

    uKey = (unsigned const char*)key;
    val = 0;

    while (*uKey) {
        val = val * MULTIPLIER + *uKey;
        ++uKey;
    }

    return val;
}

#define MAX_LOAD (1)
static table_t *resize(table_t* ht, size_t new_size) {
    element_t **new_entries, **old_entries;
    size_t old_size;
    size_t i;

    new_entries = (element_t**)calloc(new_size, sizeof(element_t*));
    old_entries = ht->entries;
    ht->entries = new_entries;

    old_size = ht->size;
    for (i = 0; i < old_size; i++) {
        element_t *cur_elem, *temp;

        cur_elem = old_entries[i];
        while (cur_elem) {
            temp = cur_elem;
            put(ht, cur_elem->key, cur_elem->value, cur_elem->count);
            cur_elem = cur_elem->next;
            free(temp);
        }
    }
    free(old_entries);
    return ht;
}

static element_t *make_elem(const char* key, const int val) {
    element_t *new_elem;
    size_t keylen;

    new_elem = (element_t*)calloc(1, sizeof(element_t));

    keylen = strlen(key);
    new_elem->key = (char*)calloc(1, keylen + 1);
    strcpy(new_elem->key, key);

    new_elem->value = val;
    new_elem->count = 1;

    return new_elem;
}

#define INIT_SIZE (128)

#define index(ht, key) (hash(key) % ht->size)

table_t *make_table() {
    table_t* new_table;
    new_table = (table_t*)calloc(1, sizeof(table_t));
    new_table->entries = (element_t**)calloc(INIT_SIZE, sizeof(element_t*));
    new_table->size = INIT_SIZE;
    new_table->num_elem = 0;
    return new_table;
}

const int get(table_t* ht, char* key) {
    size_t pos;
    element_t *cur_elem;

    pos = index(ht, key);
    cur_elem = ht->entries[pos];

    while (cur_elem != NULL) {
        if (!strcmp(cur_elem->key, key)) {
            cur_elem->count++;
            return cur_elem->value;
        }
        cur_elem = cur_elem->next;
    }
    return -1;
}

const int get_count(table_t* ht, char* key) {
    size_t pos;
    element_t *cur_elem;
    pos = index(ht, key);
    cur_elem = ht->entries[pos];

    while (cur_elem != NULL) {
        if (!strcmp(cur_elem->key, key)) {
            return cur_elem->count;
        }
        cur_elem = cur_elem->next;
    }
    return -1;
}


const int my_get(table_t* ht, char* key) {
    size_t pos;
    element_t *cur_elem;
    pos = index(ht, key);
    cur_elem = ht->entries[pos];

    while (cur_elem != NULL) {
        if (!strcmp(cur_elem->key, key)) {
            return cur_elem->value;
        }
        cur_elem = cur_elem->next;
    }
    return -1;
}

const element_t* put(table_t* ht, char* key, int val, int count) {
    size_t pos;
    element_t *head, *cur_elem;
    element_t *new_elem;

    pos = index(ht, key);
    head = ht->entries[pos];
    cur_elem = head;
    while (cur_elem != NULL) {
        if (!strcmp(cur_elem->key, key)) {
            //cur_elem->value = val;
            return cur_elem;
        }
        cur_elem = cur_elem->next;
    }
    new_elem = make_elem(key, val);
    new_elem->next = head;
    ht->entries[pos] = new_elem;
    ++(ht->num_elem);

    if (ht->num_elem == ht->size * MAX_LOAD) {
        resize(ht, ht->size * 2);
    }
    return head;
}

void get_attr(int id, char **key_name_ref, char **key_type_ref) {
    if (id > num_keys) {
         *key_name_ref = NULL;
         *key_type_ref = NULL;
    } else {
         *key_name_ref = strndup(key_names[id], strlen(key_names[id]));
         *key_type_ref = strndup(key_types[id], strlen(key_types[id]));
    }
}

int get_attribute_id(const char *keyname, const char *name) {
    int attr_id;
    char *attr;
    attr = (char*)calloc(strlen(keyname) + strlen(name) + 2, 1);
    sprintf(attr, "%s %s", keyname, name);
    if (attr_table) {
        attr_id = get(attr_table, attr);
        return attr_id;
    } else {
        return -1;
    }
}

int get_myattribute_id(const char *attr) {
    int attr_id;
    char *att;
    att = (char*)calloc(1,strlen(attr)+1);
    sprintf(att, "%s", attr);
    if (attr_table) {
        attr_id = my_get(attr_table, att);
        return attr_id;
    } else {
        return -1;
    }
}
int add_attribute(const char *keyname, const char *name)
{
    char *attr;
    attr = (char*)calloc(strlen(keyname) + strlen(name) + 2, 1);
    sprintf(attr, "%s %s", keyname, name);

    if (!attr_table) {
        attr_table = make_table();
    }
    char temp[10] = "1";
    put(attr_table, attr, num_keys++, 1);
    key_names = (char**)realloc(key_names, num_keys * sizeof(char**));
    key_types = (char**)realloc(key_types, num_keys * sizeof(char**));
    key_count = (int*)realloc(key_count, num_keys * sizeof(int));
    key_names[num_keys - 1] = strndup(keyname, strlen(keyname));
    key_types[num_keys - 1] = strndup(name, strlen(name));
    key_count[num_keys - 1] = 1;
    return num_keys - 1;
}
