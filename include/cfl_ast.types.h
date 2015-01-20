#ifndef _CFL_AST_TYPES_H_
#define _CFL_AST_TYPES_H_

typedef enum {
    CFL_NODE_VARIABLE,
    CFL_NODE_BOOL,
    CFL_NODE_INTEGER,
    CFL_NODE_CHAR,
    CFL_NODE_FUNCTION,
    CFL_NODE_LIST,
    CFL_NODE_TUPLE,
    CFL_NODE_AND,
    CFL_NODE_OR,
    CFL_NODE_NOT,
    CFL_NODE_ADD,
    CFL_NODE_MULTIPLY,
    CFL_NODE_DIVIDE,
    CFL_NODE_EQUAL,
    CFL_NODE_LESS,
    CFL_NODE_APPLICATION,
    CFL_NODE_IF,
    CFL_NODE_LET_REC,
    CFL_NODE_PUSH,
    CFL_NODE_CONCATENATE,
    CFL_NODE_CASE
} cfl_node_type;

typedef struct cfl_node_t {
    cfl_node_type type;
    unsigned int number_of_children;
    void* data;
    struct cfl_node_t** children;
} cfl_node;

typedef struct cfl_list_node_t {
    cfl_node* node;
    struct cfl_list_node_t* next;
} cfl_list_node;

#endif
