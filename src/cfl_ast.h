#ifndef _CFL_AST_H_
#define _CFL_AST_H_

#include <stdbool.h>
#include <stdlib.h>

#define MAX_IDENTIFIER_LENGTH 100
#define NUMBER_OF_RESERVED_WORDS 10

extern int reserved_word_size[];
extern char* reserved_words[];

void cfl_reset_ast_error_flag(void);
bool cfl_get_ast_error_flag(void);

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

cfl_node* cfl_create_new_node_variable(char* string);
cfl_node* cfl_create_new_node_variable_n(int string_length, char* string);
cfl_node* cfl_create_new_node_bool(bool value);
cfl_node* cfl_create_new_node_integer(int value);
cfl_node* cfl_create_new_node_char(char value);
cfl_node* cfl_create_new_node_function(cfl_node* argument, cfl_node* body);

typedef struct cfl_list_node_t {
    cfl_node* node;
    struct cfl_list_node_t* next;
} cfl_list_node;

void cfl_delete_list_nodes(cfl_list_node* list);

cfl_node* cfl_create_new_node_list(cfl_list_node* list);
cfl_node* cfl_create_new_node_tuple(unsigned int number_of_children, cfl_node** children);

cfl_node* cfl_create_new_node_and(cfl_node* left, cfl_node* right);
cfl_node* cfl_create_new_node_or(cfl_node* left, cfl_node* right);
cfl_node* cfl_create_new_node_not(cfl_node* child);
cfl_node* cfl_create_new_node_add(cfl_node* left, cfl_node* right);
cfl_node* cfl_create_new_node_multiply(cfl_node* left, cfl_node* right);
cfl_node* cfl_create_new_node_divide(cfl_node* left, cfl_node* right);
cfl_node* cfl_create_new_node_equal(cfl_node* left, cfl_node* right);
cfl_node* cfl_create_new_node_less(cfl_node* left, cfl_node* right);

cfl_node* cfl_create_new_node_application(cfl_node* function, cfl_node* argument);
cfl_node* cfl_create_new_node_if(cfl_node* condition,
                                 cfl_node* then_node,
                                 cfl_node* else_node);
cfl_node* cfl_create_new_node_let_rec(cfl_node* name,
                                      cfl_node* argument,
                                      cfl_node* procedure,
                                      cfl_node* body);

cfl_node* cfl_create_new_node_push(cfl_node* left, cfl_node* right);
cfl_node* cfl_create_new_node_concatenate(cfl_node* left, cfl_node* right);
cfl_node* cfl_create_new_node_case(cfl_node* list,
                                   cfl_node* empty,
                                   cfl_node* head,
                                   cfl_node* tail,
                                   cfl_node* nonempty);

cfl_node* cfl_copy_new_node(cfl_node* node);

bool cfl_reinitialize_node_bool(cfl_node* node, bool value);
bool cfl_reinitialize_node_integer(cfl_node* node, int value);

void cfl_delete_node(cfl_node* node);
void cfl_free_node(cfl_node* node);

bool cfl_is_free(char* name, cfl_node* node);

void cfl_print_node(cfl_node* node);

#endif
