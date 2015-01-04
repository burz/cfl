#ifndef _CFL_AST_H_
#define _CFL_AST_H_

#include <stdbool.h>

#define MAX_IDENTIFIER_LENGTH 100
#define NUMBER_OF_RESERVED_WORDS 8

extern char* reserved_words[];

typedef enum {
    CFL_NODE_VARIABLE,
    CFL_NODE_BOOL,
    CFL_NODE_INTEGER,
    CFL_NODE_FUNCTION,
    CFL_NODE_LIST,
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
    CFL_NODE_LET_REC
} cfl_node_type;

typedef struct cfl_node_t {
    cfl_node_type type;
    unsigned int number_of_children;
    void* data;
    struct cfl_node_t** children;
} cfl_node;

int cfl_create_node_variable(cfl_node* node, char* string);
int cfl_create_node_bool(cfl_node* node, bool value);
int cfl_create_node_integer(cfl_node* node, int value);
int cfl_create_node_function(cfl_node* node, cfl_node* argument, cfl_node* body);

typedef struct cfl_list_node_t {
    cfl_node* node;
    struct cfl_list_node_t* next;
} cfl_list_node;

void cfl_create_node_list(cfl_node* node, cfl_list_node* list);

int cfl_create_node_and(cfl_node* node, cfl_node* left, cfl_node* right);
int cfl_create_node_or(cfl_node* node, cfl_node* left, cfl_node* right);
int cfl_create_node_not(cfl_node* node, cfl_node* child);
int cfl_create_node_add(cfl_node* node, cfl_node* left, cfl_node* right);
int cfl_create_node_multiply(cfl_node* node, cfl_node* left, cfl_node* right);
int cfl_create_node_divide(cfl_node* node, cfl_node* left, cfl_node* right);
int cfl_create_node_equal(cfl_node* node, cfl_node* left, cfl_node* right);
int cfl_create_node_less(cfl_node* node, cfl_node* left, cfl_node* right);

int cfl_create_node_application(cfl_node* node,
                                cfl_node* function,
                                cfl_node* argument);
int cfl_create_node_if(cfl_node* node,
                       cfl_node* condition,
                       cfl_node* then_node,
                       cfl_node* else_node);
int cfl_create_node_let_rec(cfl_node* node,
                            cfl_node* name,
                            cfl_node* argument,
                            cfl_node* procedure,
                            cfl_node* body);

int cfl_copy_node(cfl_node* target, cfl_node* node);

void cfl_delete_node(cfl_node* node);

void cfl_print_node(cfl_node* node);

#endif
