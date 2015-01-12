#ifndef _CFL_PARSER_H_
#define _CFL_PARSER_H_

#include "cfl_ast.h"

void cfl_parse_error_unexpected_char(char x);
void cfl_parse_error_expected(char* expecting, char* after, char* start, char* end);
void cfl_parse_error_bad_division(void);
void cfl_parse_error_partial_program(void);
void cfl_parse_error_missing_main(void);
void cfl_parse_error_main_has_arguments(void);
void cfl_parse_error_unparseable_file(char* filename);

bool cfl_is_whitespace(char x);

typedef struct cfl_token_chain_t {
    char* start;
    char* end;
    struct cfl_token_chain_t* next;
} cfl_token_chain;

cfl_token_chain* cfl_create_token_chain_node(char* start, unsigned int length);
bool cfl_generate_token_chain(cfl_token_chain* head, char* start, char* end);
void cfl_print_token_chain(cfl_token_chain* chain);
void cfl_delete_token_chain(cfl_token_chain* chain);

typedef char* (*cfl_node_parser)(cfl_node *node, char* start, char* end);

char* cfl_parse_parentheses(cfl_node* node,
                            cfl_node_parser parser,
                            char* start,
                            char* end);
char* cfl_parse_binary_operation(cfl_node** left,
                                 cfl_node** right,
                                 cfl_node_parser left_parser,
                                 cfl_node_parser right_parser,
                                 int operand_length,
                                 char* operand,
                                 char* start,
                                 char* end);

char* cfl_parse_variable(cfl_node* node, char* start, char* end);
char* cfl_parse_bool(cfl_node* node, char* start, char* end);
char* cfl_parse_integer(cfl_node* node, char* start, char* end);
char* cfl_parse_char(cfl_node* node, char* start, char* end);
char* cfl_parse_string(cfl_node* node, char* start, char* end);
char* cfl_parse_list(cfl_node* node, char* start, char* end);
char* cfl_parse_tuple(cfl_node* node, char* start, char* end);
char* cfl_parse_function(cfl_node* node, char* start, char* end);

char* cfl_parse_and(cfl_node* node, char* start, char* end);
char* cfl_parse_or(cfl_node* node, char* start, char* end);
char* cfl_parse_not(cfl_node* node, char* start, char* end);
char* cfl_parse_add(cfl_node* node, char* start, char* end);
char* cfl_parse_subtract(cfl_node* node, char* start, char* end);
char* cfl_parse_multiply(cfl_node* node, char* start, char* end);
char* cfl_parse_divide(cfl_node* node, char* start, char* end);
char* cfl_parse_mod(cfl_node* node, char* start, char* end);
char* cfl_parse_equal(cfl_node* node, char* start, char* end);
char* cfl_parse_less(cfl_node* node, char* start, char* end);
char* cfl_parse_less_equal(cfl_node* node, char* start, char* end);
char* cfl_parse_greater(cfl_node* node, char* start, char* end);
char* cfl_parse_greater_equal(cfl_node* node, char* start, char* end);
char* cfl_parse_application(cfl_node* node, char* start, char* end);

char* cfl_parse_let(cfl_node* node, char* start, char* end);
char* cfl_parse_if(cfl_node* node, char* start, char* end);

char* cfl_parse_push(cfl_node* node, char* start, char* end);
char* cfl_parse_concatenate(cfl_node* node, char* start, char* end);
char* cfl_parse_case(cfl_node* node, char* start, char* end);

char* cfl_parse_atom(cfl_node* node, char* start, char* end);
char* cfl_parse_molecule(cfl_node* node, char* start, char* end);
char* cfl_parse_factor(cfl_node* node, char* start, char* end);
char* cfl_parse_term(cfl_node* node, char* start, char* end);
char* cfl_parse_list_expression(cfl_node* node, char* start, char* end);
char* cfl_parse_expression(cfl_node* node, char* start, char* end);

char* cfl_parse_program(cfl_node* node, char* start, char* end);

int cfl_parse_file(cfl_node *node, char* filename);

#endif
