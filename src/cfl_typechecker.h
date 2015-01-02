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

int cfl_type_compare(cfl_type* left, cfl_type* right);

void cfl_delete_type(cfl_type* node);

#endif
