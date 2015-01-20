#ifndef _CFL_AST_H_
#define _CFL_AST_H_

#include "cfl_ast.types.h"

#include <stdbool.h>
#include <stdlib.h>

#define MAX_IDENTIFIER_LENGTH 100
#define NUMBER_OF_RESERVED_WORDS 10

extern int reserved_word_size[];
extern char* reserved_words[];

void cfl_reset_ast_error_flag(void);
bool cfl_get_ast_error_flag(void);

cfl_node* cfl_create_new_node_variable(char* string);
cfl_node* cfl_create_new_node_variable_n(int string_length, char* string);
cfl_node* cfl_create_new_node_bool(bool value);
cfl_node* cfl_create_new_node_integer(int value);
cfl_node* cfl_create_new_node_char(char value);
cfl_node* cfl_create_new_node_function(cfl_node* argument, cfl_node* body);

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
