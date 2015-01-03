#ifndef _CFL_PARSER_H_
#define _CFL_PARSER_H_

#include "cfl_ast.h"

typedef char* (*cfl_node_parser)(cfl_node *node, char* start, char* end);

char* cfl_parse_whitespace(char* start, char* end);
char* cfl_parse_parentheses(cfl_node* node,
                            cfl_node_parser parser,
                            char* start,
                            char* end);
char* cfl_parse_binary_operation(cfl_node* left,
                                 cfl_node* right,
                                 cfl_node_parser left_parser,
                                 cfl_node_parser right_parser,
                                 int operand_length,
                                 char* operand,
                                 char* start,
                                 char* end);

char* cfl_parse_variable(cfl_node* node, char* start, char* end);
char* cfl_parse_bool(cfl_node* node, char* start, char* end);
char* cfl_parse_function(cfl_node* node, char* start, char* end);

char* cfl_parse_and(cfl_node* node, char* start, char* end);
char* cfl_parse_or(cfl_node* node, char* start, char* end);
char* cfl_parse_not(cfl_node* node, char* start, char* end);
char* cfl_parse_application(cfl_node* node, char* start, char* end);

char* cfl_parse_let(cfl_node* node, char* start, char* end);
char* cfl_parse_if(cfl_node* node, char* start, char* end);

char* cfl_parse_atom(cfl_node* node, char* start, char* end);
char* cfl_parse_molecule(cfl_node* node, char* start, char* end);
char* cfl_parse_factor(cfl_node* node, char* start, char* end);
char* cfl_parse_term(cfl_node* node, char* start, char* end);
char* cfl_parse_expression(cfl_node* node, char* start, char* end);

int cfl_parse_file(cfl_node *node, char* filename);

#endif
