#include "cfl_parser.h"

#include <stdlib.h>

extern void* cfl_parser_malloc(size_t size);

bool cfl_parse_binary_operation(cfl_token_list** end,
                                cfl_node** left,
                                cfl_node** right,
                                cfl_node_parser* left_parser,
                                cfl_node_parser* right_parser,
                                unsigned int operand_length,
                                char* operand,
                                cfl_token_list* position,
                                cfl_token_list* block)
{
    cfl_token_list* op_pos = cfl_lookahead_for(operand_length,
                                               operand,
                                               position,
                                               block);

    if(!op_pos)
        return false;

    *left = (*left_parser)(&position, position, op_pos);

    if(!*left)
        return false;

    if(position != op_pos)
    {
        cfl_free_node(*left);

        return false;
    }

    *right = (*right_parser)(end, op_pos->next, block);

    if(!*right)
    {
        cfl_free_node(*left);

        return false;
    }

    return true;
}

cfl_node* cfl_parse_and(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    cfl_node* left;
    cfl_node* right;

    if(!cfl_parse_binary_operation(end,
                                   &left,
                                   &right,
                                   &cfl_parse_boolean_factor,
                                   &cfl_parse_boolean_term,
                                   2,
                                   "&&",
                                   position,
                                   block))
        return 0;

    cfl_node* result = cfl_create_new_node_and(left, right);

    if(!result);
        return 0;

    return result;
}

cfl_node* cfl_parse_or(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    cfl_node* left;
    cfl_node* right;

    if(!cfl_parse_binary_operation(end,
                                   &left,
                                   &right,
                                   &cfl_parse_boolean_molecule,
                                   &cfl_parse_boolean_factor,
                                   2,
                                   "||",
                                   position,
                                   block))
        return 0;

    return cfl_create_new_node_or(left, right);
}

cfl_node* cfl_parse_not(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(cfl_token_string_compare(position, "!", 1))
        return 0;

    position = position->next;

    cfl_node* child = cfl_parse_atom(end, position, block);

    if(!child)
    {
        cfl_parse_error_expected("atom", "\"!\"", position->start, position->end);

        return 0;
    }

    return cfl_create_new_node_not(child);
}

cfl_node* cfl_parse_add(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    cfl_node* left;
    cfl_node* right;

    if(!cfl_parse_binary_operation(end,
                                   &left,
                                   &right,
                                   &cfl_parse_factor,
                                   &cfl_parse_term,
                                   1,
                                   "+",
                                   position,
                                   block))
        return 0;

    return cfl_create_new_node_add(left, right);
}

cfl_node* cfl_parse_multiply(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    cfl_node* left;
    cfl_node* right;

    if(!cfl_parse_binary_operation(end,
                                   &left,
                                   &right,
                                   &cfl_parse_molecule,
                                   &cfl_parse_factor,
                                   1,
                                   "*",
                                   position,
                                   block))
        return 0;

    return cfl_create_new_node_multiply(left, right);
}

cfl_node* cfl_parse_divide(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    cfl_node* left;
    cfl_node* right;

    if(!cfl_parse_binary_operation(end,
                                   &left,
                                   &right,
                                   &cfl_parse_molecule,
                                   &cfl_parse_factor,
                                   1,
                                   "/",
                                   position,
                                   block))
        return 0;

    return cfl_create_new_node_divide(left, right);
}

cfl_node* cfl_parse_equal(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    cfl_node* left;
    cfl_node* right;

    if(!cfl_parse_binary_operation(end,
                                   &left,
                                   &right,
                                   &cfl_parse_term,
                                   &cfl_parse_term,
                                   2,
                                   "==",
                                   position,
                                   block))
        return 0;

    return cfl_create_new_node_equal(left, right);
}

cfl_node* cfl_parse_less(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    cfl_node* left;
    cfl_node* right;

    if(!cfl_parse_binary_operation(end,
                                   &left,
                                   &right,
                                   &cfl_parse_term,
                                   &cfl_parse_term,
                                   1,
                                   "<",
                                   position,
                                   block))
        return 0;

    return cfl_create_new_node_less(left, right);
}

cfl_node* cfl_parse_push(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    cfl_node* left;
    cfl_node* right;

    if(!cfl_parse_binary_operation(end,
                                   &left,
                                   &right,
                                   &cfl_parse_boolean_term,
                                   &cfl_parse_boolean_term,
                                   1,
                                   ":",
                                   position,
                                   block))
        return 0;

    return cfl_create_new_node_push(left, right);
}

cfl_node* cfl_parse_concatenate(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    cfl_node* left;
    cfl_node* right;

    if(!cfl_parse_binary_operation(end,
                                   &left,
                                   &right,
                                   &cfl_parse_boolean_term,
                                   &cfl_parse_boolean_term,
                                   2,
                                   "++",
                                   position,
                                   block))
        return 0;

    return cfl_create_new_node_concatenate(left, right);
}
