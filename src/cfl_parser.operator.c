#include "cfl_parser.h"

#include <string.h>

extern void* cfl_parser_malloc(size_t size);

char* cfl_parse_binary_operation(
        cfl_node** left,
        cfl_node** right,
        cfl_node_parser left_parser,
        cfl_node_parser right_parser,
        int operand_length,
        char* operand,
        char* start,
        char* end)
{
    char* op_pos = start + 1;
    int depth = 1;

    if(*start == '(')
        ++depth;

    while(end - op_pos > operand_length + 1)
    {
        if(depth < 1)
            return 0;
        else if(*op_pos == ',' && depth == 1)
            return 0;
        if(*op_pos == '(' || *op_pos == '[')
            ++depth;
        else if(*op_pos == ')' || *op_pos == ']')
            --depth;
        else if(depth == 1 && cfl_is_whitespace(op_pos[-1]) &&
           !strncmp(op_pos, operand, operand_length) &&
           cfl_is_whitespace(op_pos[operand_length]))
            break;

        ++op_pos;
    }

    if(end - op_pos <= operand_length + 1 || depth != 1)
        return 0;

    *left = cfl_parser_malloc(sizeof(cfl_node));

    if(!left)
        return 0;

    start = (*left_parser)(*left, start, op_pos);

    if(!start)
    {
        free(*left);

        return 0;
    }

    start = cfl_parse_whitespace(start, op_pos);

    if(start != op_pos)
    {
        cfl_free_node(*left);

        return 0;
    }

    start = cfl_parse_whitespace(op_pos + operand_length, end);

    *right = cfl_parser_malloc(sizeof(cfl_node));

    if(!*right)
    {
        cfl_free_node(*left);

        return 0;
    }

    start = (*right_parser)(*right, start, end);

    if(!start)
    {
        cfl_free_node(*left);
        free(*right);

        return 0;
    }

    return start;
}

char* cfl_parse_and(cfl_node* node, char* start, char* end)
{
    cfl_node* left;
    cfl_node* right;

    start = cfl_parse_binary_operation(&left,
                                       &right,
                                       &cfl_parse_molecule,
                                       &cfl_parse_factor,
                                       2,
                                       "&&",
                                       start,
                                       end);

    if(!start)
        return 0;

    if(!cfl_create_node_and(node, left, right))
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    return start;
}

char* cfl_parse_or(cfl_node* node, char* start, char* end)
{
    cfl_node* left;
    cfl_node* right;

    start = cfl_parse_binary_operation(&left,
                                       &right,
                                       &cfl_parse_factor,
                                       &cfl_parse_term,
                                       2,
                                       "||",
                                       start,
                                       end);

    if(!start)
        return 0;

    if(!cfl_create_node_or(node, left, right))
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    return start;
}

char* cfl_parse_not(cfl_node* node, char* start, char* end)
{
    if(*(start++) != '!')
        return 0;

    cfl_node* child_node = cfl_parser_malloc(sizeof(cfl_node));

    if(!child_node)
        return 0;

    char* pos = cfl_parse_atom(child_node, start, end);

    if(!pos)
    {
        cfl_parse_error_expected("atom", "\"!\"", start, end);

        free(child_node);

        return 0;
    }

    if(!cfl_create_node_not(node, child_node))
    {
        cfl_free_node(child_node);

        return 0;
    }

    return pos;
}

char* cfl_parse_add(cfl_node* node, char* start, char* end)
{
    cfl_node* left;
    cfl_node* right;

    start = cfl_parse_binary_operation(&left,
                                       &right,
                                       &cfl_parse_factor,
                                       &cfl_parse_term,
                                       1,
                                       "+",
                                       start,
                                       end);

    if(!start)
        return 0;

    if(!cfl_create_node_add(node, left, right))
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    return start;
}

char* cfl_parse_multiply(cfl_node* node, char* start, char* end)
{
    cfl_node* left;
    cfl_node* right;

    start = cfl_parse_binary_operation(&left,
                                       &right,
                                       &cfl_parse_factor,
                                       &cfl_parse_term,
                                       1,
                                       "*",
                                       start,
                                       end);

    if(!start)
        return 0;

    if(!cfl_create_node_multiply(node, left, right))
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    return start;
}

char* cfl_parse_divide(cfl_node* node, char* start, char* end)
{
    cfl_node* left;
    cfl_node* right;

    start = cfl_parse_binary_operation(&left,
                                       &right,
                                       &cfl_parse_factor,
                                       &cfl_parse_term,
                                       1,
                                       "/",
                                       start,
                                       end);

    if(!start)
        return 0;

    if(!cfl_create_node_divide(node, left, right))
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    return start;
}

char* cfl_parse_equal(cfl_node* node, char* start, char* end)
{
    cfl_node* left;
    cfl_node* right;

    start = cfl_parse_binary_operation(&left,
                                       &right,
                                       &cfl_parse_molecule,
                                       &cfl_parse_molecule,
                                       2,
                                       "==",
                                       start,
                                       end);

    if(!start)
        return 0;

    if(!cfl_create_node_equal(node, left, right))
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    return start;
}

char* cfl_parse_less(cfl_node* node, char* start, char* end)
{
    cfl_node* left;
    cfl_node* right;

    start = cfl_parse_binary_operation(&left,
                                       &right,
                                       &cfl_parse_molecule,
                                       &cfl_parse_molecule,
                                       1,
                                       "<",
                                       start,
                                       end);

    if(!start)
        return 0;

    if(!cfl_create_node_less(node, left, right))
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    return start;
}

char* cfl_parse_push(cfl_node* node, char* start, char* end)
{
    cfl_node* left;
    cfl_node* right;

    start = cfl_parse_binary_operation(&left,
                                       &right,
                                       &cfl_parse_term,
                                       &cfl_parse_list_expression,
                                       1,
                                       ":",
                                       start,
                                       end);

    if(!start)
        return 0;

    if(!cfl_create_node_push(node, left, right))
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    return start;
}

char* cfl_parse_concatenate(cfl_node* node, char* start, char* end)
{
    cfl_node* left;
    cfl_node* right;

    start = cfl_parse_binary_operation(&left,
                                       &right,
                                       &cfl_parse_list_expression,
                                       &cfl_parse_list_expression,
                                       2,
                                       "++",
                                       start,
                                       end);

    if(!start)
        return 0;

    if(!cfl_create_node_concatenate(node, left, right))
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    return start;
}
