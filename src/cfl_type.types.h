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

typedef struct cfl_type_equation_chain_t {
    cfl_type* left;
    cfl_type* right;
    struct cfl_type_equation_chain_t* next;
} cfl_type_equation_chain;

typedef struct cfl_type_equation_chain_list_t {
    cfl_type_equation_chain* link;
    struct cfl_type_equation_chain_list_t* next;
} cfl_type_equation_chain_list;

typedef struct {
    cfl_type_equation_chain equation_head;
    unsigned int equation_hash_length;
    cfl_type_equation_chain_list** equation_hash;
} cfl_type_equations;

typedef struct cfl_type_hypothesis_chain_t {
    char* name;
    unsigned int id;
    struct cfl_type_hypothesis_chain_t* next;
} cfl_type_hypothesis_chain;

#endif
