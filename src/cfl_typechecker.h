#ifndef _CFL_TYPECHECKER_H_
#define _CFL_TYPECHECKER_H_

#include "cfl_ast.h"

typedef enum {
    CFL_TYPE_VARIABLE,
    CFL_TYPE_BOOL,
    CFL_TYPE_ARROW
} cfl_type_type;

typedef struct cfl_type_t {
    cfl_type_type type;
    char* name;
    struct cfl_type_t* input;
    struct cfl_type_t* output;
} cfl_type;

int cfl_create_type_variable(cfl_type* node, char* name);
int cfl_create_type_bool(cfl_type* node);
int cfl_create_type_arrow(cfl_type* node, cfl_type* input, cfl_type* output);

int cfl_compare_type(cfl_type* left, cfl_type* right);

int cfl_copy_type(cfl_type* target, cfl_type* node);

void cfl_delete_type(cfl_type* node);

typedef struct cfl_type_equation_chain_t {
    cfl_type* left;
    cfl_type* right;
    struct cfl_type_equation_chain_t* next;
} cfl_type_equation_chain;

int cfl_add_equation(cfl_type_equation_chain* head, cfl_type* left, cfl_type* right);
int cfl_add_equation_from_copies(cfl_type_equation_chain* head,
                                 cfl_type* left,
                                 cfl_type* right);
cfl_type* cfl_generate_type_equation_chain(cfl_type_equation_chain* head,
                                           cfl_node* node);
int cfl_close_type_equation_chain(cfl_type_equation_chain* head);
int cfl_ensure_type_equation_chain_consistency(cfl_type_equation_chain* chain);

void cfl_delete_type_equation_chain(cfl_type_equation_chain* chain);

cfl_type* cfl_typecheck(cfl_node* node);

#endif
