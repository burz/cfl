#ifndef _CFL_PROGRAM_H_
#define _CFL_PROGRAM_H_

#include "cfl_ast.h"
#include "cfl_type.h"

typedef struct cfl_definition_list_t {
    cfl_node* name;
    cfl_node* definition;
    cfl_type* type;
    struct cfl_definition_list_t* next;
} cfl_definition_list;

void cfl_free_definition_list(cfl_definition_list* list);

typedef struct {
    cfl_definition_list* definitions;
    cfl_node* main;
    cfl_type* type;
} cfl_program;

void cfl_free_program(cfl_program* program);

void cfl_print_program(cfl_program* program);

void cfl_print_program_type(cfl_program* program);

#endif
