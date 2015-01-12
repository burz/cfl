#ifndef _CFL_TYPE_H_
#define _CFL_TYPE_H_

#include "cfl_ast.h"

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

void cfl_create_type_variable(cfl_type* node, unsigned int id);
void cfl_create_type_bool(cfl_type* node);
void cfl_create_type_integer(cfl_type* node);
void cfl_create_type_char(cfl_type* node);
void cfl_create_type_list(cfl_type* node, cfl_type* content);
void cfl_create_type_tuple(cfl_type* node,
                           unsigned int number_of_children,
                           cfl_type** children);
void cfl_create_type_arrow(cfl_type* node, cfl_type* input, cfl_type* output);

int cfl_compare_type(cfl_type* left, cfl_type* right);

int cfl_copy_type(cfl_type* target, cfl_type* node);

void cfl_delete_type(cfl_type* node);
void cfl_free_type(cfl_type* node);

void cfl_print_type(cfl_type* node);

typedef struct cfl_type_equation_chain_t {
    cfl_type* left;
    cfl_type* right;
    struct cfl_type_equation_chain_t* next;
} cfl_type_equation_chain;

int cfl_add_equation(cfl_type_equation_chain* head, cfl_type* left, cfl_type* right);
int cfl_add_equation_from_copies(cfl_type_equation_chain* head,
                                 cfl_type* left,
                                 cfl_type* right);

typedef struct cfl_type_hypothesis_chain_t {
    char* name;
    unsigned int id;
    struct cfl_type_hypothesis_chain_t* next;
} cfl_type_hypothesis_chain;

unsigned int cfl_lookup_hypothesis(cfl_type_hypothesis_chain* chain, char* name);

cfl_type* cfl_generate_type_equation_chain(cfl_type_equation_chain* equation_head,
                                           cfl_type_hypothesis_chain* hypothesis_head,
                                           cfl_node* node);

int cfl_close_type_equation_chain(cfl_type_equation_chain* head);
int cfl_ensure_type_equation_chain_consistency(cfl_type_equation_chain* chain);

cfl_type* cfl_substitute_type(cfl_type_equation_chain* head, cfl_type* node);

void cfl_delete_type_equation_chain(cfl_type_equation_chain* chain);

cfl_type* cfl_typecheck(cfl_node* node);

#endif
