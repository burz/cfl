#ifndef _CFL_AST_H_
#define _CFL_AST_H_

typedef enum {
    CFL_NODE_NULL = 0,
    CFL_NODE_BOOL,
    CFL_NODE_VARIABLE,
    CFL_NODE_AND,
    CFL_NODE_OR,
    CFL_NODE_NOT
} cfl_node_type;

typedef struct cfl_node_t {
    cfl_node_type type;
    void* data;
    int number_of_children;
    struct cfl_node_t *children;
} cfl_node;

void cfl_delete_node(cfl_node* node);

#endif
