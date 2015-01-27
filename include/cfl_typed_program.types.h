#ifndef _CFL_TYPED_PROGRAM_TYPES_H_
#define _CFL_TYPED_PROGRAM_TYPES_H_

#include "cfl_ast.h"
#include "cfl_type.types.h"

typedef struct cfl_typed_node_t {
    cfl_node_type node_type;
    cfl_type* resulting_type;
    unsigned int number_of_children;
    void* data;
    struct cfl_typed_node_t** children;
} cfl_typed_node;

typedef struct cfl_typed_node_list_t {
    cfl_typed_node* node;
    struct cfl_typed_node_list_t* next;
} cfl_typed_node_list;

typedef struct cfl_typed_definition_list_t {
    char* name;
    cfl_typed_node* definition;
    struct cfl_typed_definition_list_t* next;
} cfl_typed_definition_list;

typedef struct {
    cfl_typed_definition_list* definitions;
    cfl_typed_node* main;
} cfl_typed_program;

#endif
