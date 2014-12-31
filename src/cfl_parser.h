#ifndef _CFL_PARSER_H_
#define _CFL_PARSER_H_

#include "cfl_ast.h"

typedef char* (*cfl_node_parser)(cfl_node *node, char* start, char* end);

char* cfl_parse_whitespace(char* start, char* end);
char* cfl_parse_parentheses(cfl_node *node, cfl_node_parser parser, char* start, char* end);

char* cfl_parse_bool(cfl_node *node, char* start, char* end);

char* cfl_parse_term(cfl_node *node, char* start, char* end);
char* cfl_parse_expression(cfl_node *node, char* start, char* end);

int cfl_parse_file(cfl_node *node, char* filename);

#endif
