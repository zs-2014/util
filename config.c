#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <limits.h>

#include "hash.h"
#include "linklist.h"
#include "linkhash.h"
#include "string.h"
#include "config.h"

#define default_section_size 10

#ifndef null
#define null NULL
#endif


static size_t hash_for_section(const void *key)
{
   return BKDRHash((char *)key) ;
}

static int cmp_for_section(const void *key1, const void *key2)
{
    return strcmp(key1, key2) ; 
}

//section, section_name
static int cmp_for_config(const void *val1, const void *val2)
{
    ConfigSection *section1 = (ConfigSection *)val1 ; 
    if(section1 ->section_name == val2)
        return 0 ;
    return strcmp(section1 ->section_name, val2) ; 
}

static void free_for_config(void *val)
{
    ConfigSection *config_section = (ConfigSection *)val ;
    free(config_section ->section_name) ;
    free_link_hash_table(config_section ->lh_table) ;
    free(config_section) ;
}

Config *malloc_config(const char *file_name)
{
    Config *config = (Config *)calloc(1, sizeof(Config)) ;
    config ->fname = strdup(file_name) ; 
    config ->llst = malloc_linked_list() ;
    config ->llst ->free_value = free_for_config ;
    return config ;
}

ConfigSection *malloc_config_section()
{
    ConfigSection *config_section = (ConfigSection *)calloc(1, sizeof(ConfigSection)) ;
    config_section ->lh_table = malloc_link_hash_table(default_section_size) ; 
    config_section ->lh_table ->cmp = cmp_for_section ;
    config_section ->lh_table ->hash = hash_for_section ;
    return config_section ;
}

void free_config(Config *config)
{
    if(!config) 
        return ;
    if(config ->fname)
        free(config ->fname) ;
    if(config ->llst)
        free_linked_list(config ->llst) ;
    free(config) ;
}

//[test]
//ip = xxxx
static Config *parse_config(Config *config, const char *buff)
{
    LinkedList *llst = split(buff, '\n', -1, 1) ; 
    if(!llst)
        return null ;

    ConfigSection *null_section = malloc_config_section() ;
    append_to_linked_list(config ->llst, null_section) ;
    ConfigSection *section = null;
    char *value = null ;
    for_each_in_linked_list(llst, value)
    {
        int len = strlen(strip(value, ' ')) ;           
        if(len == 0 || value[0] == '#')
            continue ;
        char *s_nm = value ; 
        char *e_nm = null ;
        char *s_val = null ;
        //possible new section
        if(value[0] == '[')
        {
            //null section skip it
            if(len == 2 || value[1] == ']') 
            {
                section = null ;
                continue ;
            }
            char *end_section = strchr(value, ']') ;
            //a new_section and create it
            if(end_section)
            {
                section = malloc_config_section();
                section ->section_name = strip(strip(strndup(value+1, end_section-value-1), ' '), '\t') ; 
                if(strlen(section ->section_name) == 0)
                {
                    free_for_config(section) ;
                    section = null ;
                    continue ;
                }
                append_to_linked_list(config ->llst, section) ;
                continue ;
            }
        }
        e_nm = strchr(s_nm, '=') ;
        if(!e_nm || s_nm == e_nm)
            continue ;
        char *name = strip(strip(strndup(s_nm, e_nm - s_nm), ' '), '\t') ;
        if(strlen(name) == 0)
            continue ;
        value = strip(strip(strdup(e_nm + 1), ' '), '\t') ;
        if(section)    
            set(section ->lh_table, name, value) ;
        else
            set(null_section ->lh_table, name, value) ;
    }
    free_linked_list(llst) ;
    return null ;  
}

Config *read_config(const char *file_name) 
{
    if(!file_name)
        return null ;
    Config *config = malloc_config(file_name)  ;
    int fd = open(file_name, O_RDONLY) ;
    if(fd < 0)
    {
        snprintf(config ->errbuff, sizeof(config ->errbuff), "%s", strerror(errno)) ;
        return config ;
    }
    struct stat st ;
    if(fstat(fd, &st) != 0)
    {
        snprintf(config ->errbuff, sizeof(config ->errbuff), "%s", strerror(errno)) ;
        return config ;
    }
    char *buff = (char *)calloc(1, st.st_size+1) ;
    if(!buff)
    {
        snprintf(config ->errbuff, sizeof(config ->errbuff), "fail to calloc memory %d", st.st_size) ;
        close(fd) ;
        return config ;
    }
    if(read(fd, buff, st.st_size) != st.st_size)
    {
        snprintf(config ->errbuff, sizeof(config ->errbuff), "%s", strerror(errno)) ;
        close(fd) ;
        return config ;
    }
    close(fd) ;
    parse_config(config, buff) ;
    free(buff) ;
    return config ;
}

const char *get_value(Config *config, const char *section_name, const char *key)
{
    if(!config || !key) 
        return null ;
    ConfigSection *config_section = find_in_linked_list(config ->llst, section_name) ;
    if(!config_section)
        return null ;
    return get(config_section ->lh_table, key) ;
}

char *read_value(Config *config, const char *section_name, const char *key, char *value_buff)
{
    if(!config || !key || !value_buff) 
        return null ;

    value_buff[0] = '\0' ;
    const char *value = get_value(config, section_name, key) ;
    if(!value)
        return null ;
    strcpy(value_buff, value) ;
    return value_buff ;
}

int write_value(Config *config, const char *section_name, const char *key, const char *value)
{
    if(!config || !key || strlen(key) == 0 || !value)
        return 0  ;
    ConfigSection *config_section = find_in_linked_list(config ->llst, section_name) ;
    if(!config_section)
        return 0 ;
    return set(config_section ->lh_table, strdup(key), strdup(value)) != null ;
}

int save_as(Config *config, const char *file_name, const char *mode)
{
    if(!config || !file_name)
        return 0 ;
    FILE *fp = fopen(file_name, mode) ; 

    if(!fp)
    {
        snprintf(config ->errbuff, sizeof(config ->errbuff), "%s", strerror(errno)) ;
        return 0 ;
    }

    ConfigSection *section = null ;
    for_each_in_linked_list(config ->llst, section)
    {
        char *k = null ;
        char *v = null ;
        fprintf(fp, "[%s]\n", section ->section_name ? section->section_name : "") ;
        for_each_in_link_hash_table(section ->lh_table, k, v)
        {
            fprintf(fp, "%s=%s\n", k, v)  ;
        }
    }
    fclose(fp) ;
    return 1 ;
}


int config_is_ok(Config *config)
{
    return config ? config ->errbuff[0] == '\0':0 ;
}

#if 1
void test_string_split()
{    
    LinkedList *llst = split("a\nb\nc\n\ne\nf\nd", '\n', 0, 0) ;
    char *value = null;
    printf("[") ;
    for_each_in_linked_list(llst, value)
    {
        printf("%s\n", value) ;
    }
    printf("]\n") ;
}

int main(int argc, char *argv[])
{
    Config *config = read_config("test.ini") ;
    ConfigSection *section = null ;
    for_each_in_linked_list(config ->llst, section)
    {
        char *k = null ;
        char *v = null ;
        printf("[%s]\n", section ->section_name ? section->section_name : "") ;
        for_each_in_link_hash_table(section ->lh_table, k, v)
        {
            printf("%s=%s\n", k, v)  ;
        }
    }
    save_as(config, "test1.ini", "wb+") ;
    free_config(config) ;
    return 0;
}

#endif
