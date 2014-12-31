#ifndef _CFL_AST_H_
#define _CFL_AST_H_

#include <stdbool.h>

#define MAX_IDENTIFIER_LENGTH 100

typedef enum {
    CFL_NODE_BOOL,
    CFL_NODE_VARIABLE,
    CFL_NODE_AND,
    CFL_NODE_OR,
    CFL_NODE_NOT
} cfl_node_type;

typedef struct cfl_node_t {
    int line_number;
    cfl_node_type type;
    int number_of_children;
    void* data;
    struct cfl_node_t** children;
} cfl_node;

int cfl_create_node_bool(cfl_node* node, bool value);
int cfl_create_node_variable(cfl_node* node, const char* string);
int cfl_create_node_and(cfl_node* node, cfl_node* left, cfl_node* right);
int cfl_create_node_or(cfl_node* node, cfl_node* left, cfl_node* right);
int cfl_create_node_not(cfl_node* node, cfl_node* child);

void cfl_delete_node(cfl_node* node);

#endif
