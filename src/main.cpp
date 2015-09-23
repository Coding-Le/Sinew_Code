#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include<sstream>
#include "transfer.h"
#include "catalog.h"
#include "type.h"
#include"catalog.h"
#include <assert.h>

int insert_to_binary(char* filename);
int insert_back_json(FILE *outfile);
void output_catalog(FILE* outfile);
void find_a_b(char *key, char* value, char* file);
int insert_file(char* filename);

char tempname[] = "insert_to_binary";
char tempname2[] = "insert_back_to_json";
char tempname3[] = "kmn";
char findresult[] = "find_result";
char array_for_8kb[8192];
int num8192 = 0;
char dbname[] = "nobench_data.json";
char schemafname[] = "catalog_file";
char outputname[] = "catalog_for_user";


int main(int argc, char** argv) {
    char filename[100], key[100], value[100], rawdata[100];
    FILE* catalogfile;
    int instruction_num, flag;
    flag = 0;
    printf("Please input your instruction number!\n");
    printf("1 for insert filename    2 for check catalog     3 for find value \n");
    printf("0 for exit \n");
    FILE* clearfile = fopen("insert to pdf form", "w");
    fclose(clearfile);
    while (scanf("%d", &instruction_num)) {
        switch(instruction_num) {
           case 1:
            printf("please input the filename(the complete path) for insert\n");
            printf("the default name is '(the current file path)/nobench_data.json'\n");
            scanf("%s", filename);
            if (insert_file(filename)) {
                flag = 1;
            }
            break;
           case 2:  if (flag) {
                catalogfile = fopen(outputname,"w");
                output_catalog(catalogfile);
                printf("The catalog has been written in the file 'catalog_for_user' \n"); break;
            } else {
                printf("Your haven't input json file path !!!\n");
                break;
            }
           case 3:
            printf("please input the key and value for search, for example 'find bool = true'\n");
            if (flag) {
                scanf("%s", rawdata);
                /*for (int i = 0; i < strlen(rawdata); i++) {
                        if (rawdata[i] == ' ' && i > 3) {
                            int k = 0;
                            while (rawdata[++i] != ' ' && rawdata[i] != '=') {
                                key[k++] = rawdata[i];
                            }
                            while (rawdata[++i] == ' ' || rawdata[i] == '=');
                            for (int j = 0; i < strlen(rawdata); i++, j++) {
                                value[j] = rawdata[i];
                            }
                        }
                }*/
                scanf("%s", key);
                scanf("%s", rawdata);
                scanf("%s", value);

                find_a_b(key, value, filename);
           } else {
                printf("Your haven't input json file path !!!\n");
             }
            break;
           case 0:  break;
           default: printf("invalid input!!! You should put 0 or 1 or 2 or 3\n");
        }
        if (!instruction_num) {
            break;
        }
    }
    return 0;
    //find_a_b("dyn2", "false");


    //FILE *addtemp = fopen(tempname2, "");

}

int insert_file(char* filename) {
    int flag = 0;
    if (insert_to_binary(filename)) {
        printf("the corresponding binary file is 'insert_to_binary'\n");
        printf("the corresponding form of demanded is 'insert to pdf form'\n");
            FILE *temp1 = fopen(tempname2, "w");
            if(insert_back_json(temp1)) {
            printf("Done! The json file transformed from 'insert_to_binary' back to 'insert_back_to_json'\n");
            } else {
                printf("unexpected error");
            }

    } else {
        printf("invalid path");
        return 0;
    }
}

int insert_to_binary(char* filename) {
    char *buffer, *binary, *doc;
    size_t len, read;
    size_t binsize;
    FILE *dbfile;
    FILE* infile;
    infile = fopen(filename, "r");
    dbfile = fopen(tempname, "w");
    buffer = NULL;
    len = 0;
    document c;
    std::stringstream stream;
    char result[10];
    char to_int32;
    while ((read = getline(&buffer, &len, infile)) != -1) {
        int flg = strlen(buffer);
        if(strlen(buffer) <= 2 && (buffer[0] == '[' || buffer[0] == ']')) {
            continue;
        }
        json_typeid tt = jsmn_get_type(jsmn_tokenize(buffer),buffer);
        binsize = to_binary(tt,buffer, &binary);
        int tempbinsize = binsize;
        //stream << tempbinsize;
        //stream >> result;
        //stream.clear();
        for (int i = 0; i < 4; i++) {
            result[i] = tempbinsize%256;
            tempbinsize = tempbinsize/256;
        }
        for (int i = 0; i < 4; i++) {
            array_for_8kb[num8192++] = result[i];
            if (num8192 >= 8192) {
                fwrite(array_for_8kb, num8192, 1, dbfile);
                num8192 = 0;
            }
        }
        for (int i = 0; i < binsize; i++) {
           array_for_8kb[num8192++] = binary[i];
           if (num8192 >= 8192) {
               fwrite(array_for_8kb, num8192, 1, dbfile);
               num8192 = 0;
           }

       }
        //fwrite(&tempbinsize, sizeof(tempbinsize), 1, dbfile);

        //fwrite(binary, binsize, 1, dbfile);
        free(buffer);
        free(binary);
        buffer = NULL;
        len = 0;
    }
    if (num8192 > 0) {
        fwrite(array_for_8kb, num8192, 1, dbfile);
        num8192 = 0;
    }

    fclose(dbfile);

    return 1;
}

int insert_back_json(FILE *outfile) {
    std::stringstream stream;
    FILE *dbfile, *extract_pointer;
    int binsize;
    char *binary, *json;
    char result[10];
    dbfile = fopen(tempname, "r");
    fprintf(stderr, "read schema\n");
    //fread(result, sizeof(binsize), 1, dbfile);
    //stream << result;
    //stream >> binsize;
    //stream.clear();
    //fread(result, sizeof(binsize), 1, dbfile);
    fread(&binsize, sizeof(binsize), 1, dbfile);
    //int i = 10;
    int i = 0;
    fprintf(outfile, "[\n");
    while (!feof(dbfile)) {
        /*i--;
        if (i == -2) {
            printf("\n");
            int c = fgetc(dbfile);
            while(!feof(dbfile)) {
                printf("%x",c);
                c = fgetc(dbfile);
            }
            return 1;
        }*/
        binary = (char*)malloc(binsize);
        fread(binary, binsize, 1, dbfile);

        // fprintf(stderr, "read object\n");
        json_typeid tt = jsmn_get_type(jsmn_tokenize(binary),binary);
        //int i = binsize;
        json = binary_document_to_string(binary);
        // fprintf(stderr, "converted to json\n");
        fprintf(outfile, "%s\n", json);
        fread(&binsize, sizeof(binsize), 1, dbfile);
        //fread(result, sizeof(binsize), 1, dbfile);
        //stream << result;
        //stream >> binsize;
        //stream.clear();
        if (feof(dbfile)) {
            fprintf(outfile, "]\n");
        }
        //if (i == 1)
        //char* extractchar = extract_key(json, "dyn1", "10005");
        //i++;
        fflush(outfile);
        free(json);
        free(binary);

    }

    return 1;
}

void output_catalog(FILE* outfile)
{
    char *keyname, *name, *attr;
    int tempcount;
    int i;
    char title[100] = "id        key_name            key_type            count\n";
    fwrite(title, 1, strlen(title), outfile);
    for (i = 0; i < num_keys; ++i) {
        keyname = key_names[i];
        name = key_types[i];
        /*
        attr = (char*)calloc(strlen(keyname) + strlen(name) + 6+2*sizeof(int)+2, 1);
        sprintf(attr, "%d\t\t%s\t\t%s\t\t%d\n", i+1, keyname, name, tempcount);
        fwrite(attr, 1, strlen(attr), outfile);
        free(attr);*/
        char *attrs;
        attrs = (char*)calloc(strlen(keyname) + strlen(name) + 4 + 2*sizeof(int), 1);
        sprintf(attrs, "%s %s", keyname, name);
        tempcount = get_count(attr_table, attrs);
        fprintf(outfile, "%-10d%-20s%-20s%-10d\n", i+1,keyname, name, tempcount);
    }
    fflush(outfile);
}

void find_a_b(char *key, char* value, char* file) {
    FILE *find = fopen(file, "r");
    char *buffer;
    int flag = 0;
    size_t len, read;
    FILE *output_result = fopen(findresult, "w");
    buffer = NULL;
    len = 0;
    document c;
    char *search_ab = (char*)malloc(strlen(key)+strlen(value)+5);
    char tempchar1[2] = ":";
    tempchar1[0] = 34;
    sprintf(search_ab,"%s%s%s: %s", tempchar1,key,tempchar1,value);
    while ((read = getline(&buffer, &len, find)) != -1) {
        int flg = strlen(buffer);
        if(strlen(buffer) <= 2 && (buffer[0] == '[' || buffer[0] == ']')) {
            continue;
        }
        if (strstr(buffer, search_ab)) {
            printf("%s\n", buffer);
            fprintf(output_result, "%s\n", buffer);
            flag = 1;
        }
        //json_typeid tt = jsmn_get_type(jsmn_tokenize(buffer),buffer);
        //binsize = to_binary(tt,buffer, &binary);
        //fwrite(&binsize, sizeof(binsize), 1, dbfile);
       // fwrite(binary, binsize, 1, dbfile);

        
        free(buffer);
        //free(binary);
        buffer = NULL;
        len = 0;
    }
    if (!flag) {
        printf("NONE\n");
        fprintf(output_result,"NONE\n");
    }
    fclose(output_result);
    fclose(find);
    printf("You can also see the result in the file 'find result'\n");
}
