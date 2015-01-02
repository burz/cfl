#ifndef _CFL_AST_H_
#define _CFL_AST_H_

#include <stdbool.h>

#define MAX_IDENTIFIER_LENGTH 100
#define NUMBER_OF_RESERVED_WORDS 6

extern char* reserved_words[];

typedef enum {
    CFL_NODE_VARIABLE,
    CFL_NODE_BOOL,
    CFL_NODE_FUNCTION,
    CFL_NODE_AND,
    CFL_NODE_OR,
    CFL_NODE_NOT,
    CFL_NODE_IF
} cfl_node_type;

typedef struct cfl_node_t {
    cfl_node_type type;
    unsigned int number_of_children;
    void* data;
    struct cfl_node_t** children;
} cfl_node;

int cfl_create_node_variable(cfl_node* node, const char* string);
int cfl_create_node_bool(cfl_node* node, bool value);
int cfl_create_node_function(cfl_node* node, cfl_node* argument, cfl_node* body);

int cfl_create_node_and(cfl_node* node, cfl_node* left, cfl_node* right);
int cfl_create_node_or(cfl_node* node, cfl_node* left, cfl_node* right);
int cfl_create_node_not(cfl_node* node, cfl_node* child);
int cfl_create_node_if(cfl_node* node,
                       cfl_node* condition,
                       cfl_node* then_node,
                       cfl_node* else_node);

void cfl_delete_node(cfl_node* node);

void cfl_print_node(cfl_node* node);

#endif
