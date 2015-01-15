#ifndef _CFL_PARSER_TYPES_H_
#define _CFL_PARSER_TYPES_H_

#include "cfl_ast.types.h"

typedef struct cfl_token_list_t {
    char* start;
    char* end;
    struct cfl_token_list_t* next;
} cfl_token_list;

typedef cfl_node* cfl_node_parser(cfl_token_list** end,
                                  cfl_token_list* position,
                                  cfl_token_list* block);

#endif
