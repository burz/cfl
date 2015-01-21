#ifndef _CFL_TYPE_TYPES_H_
#define _CFL_TYPE_TYPES_H_

typedef enum {
    CFL_TYPE_VARIABLE,
    CFL_TYPE_BOOL,
    CFL_TYPE_INTEGER,
    CFL_TYPE_CHAR,
    CFL_TYPE_LIST,
    CFL_TYPE_TUPLE,
    CFL_TYPE_ARROW
} cfl_type_type;

typedef struct cfl_type_t {
    cfl_type_type type;
    unsigned int id;
    void* input;
    void* output;
} cfl_type;

typedef struct cfl_type_list_element_t {
    cfl_type* type;
    struct cfl_type_list_element_t* next;
} cfl_type_list_element;

typedef struct cfl_type_hash_element_t {
    cfl_type* type;
    cfl_type_list_element variable_head;
    cfl_type_list_element typed_head;
    struct cfl_type_hash_element_t* next;
} cfl_type_hash_element;

typedef struct {
    unsigned int equation_hash_table_length;
    cfl_type_hash_element* hash_table;
} cfl_type_equations;

typedef struct cfl_type_hypothesis_chain_t {
    char* name;
    unsigned int id;
    struct cfl_type_hypothesis_chain_t* next;
} cfl_type_hypothesis_chain;

typedef struct cfl_hypothesis_load_list_t {
    cfl_type* left;
    cfl_type* right;
    struct cfl_hypothesis_load_list_t* next;
} cfl_hypothesis_load_list;

#endif
