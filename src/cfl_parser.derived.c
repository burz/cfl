#include "cfl_parser.h"

extern void* cfl_parser_malloc(size_t size);

cfl_node* cfl_parse_string(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(*position->start != '"')
        return 0;

    char* pos = position->start + 1;

    cfl_list_node head;
    cfl_list_node* list_pos = &head;

    while(pos != position->end - 1)
    {
        if(*pos == '"')
            break;

        list_pos->next = cfl_parser_malloc(sizeof(cfl_list_node));

        if(!list_pos->next)
        {
            cfl_delete_list_nodes(head.next);

            return 0;
        }

        list_pos->next->node = cfl_create_new_node_char(*pos);

        if(!list_pos->next->node)
        {
            free(list_pos->next);

            list_pos->next = 0;

            cfl_delete_list_nodes(head.next);

            return 0;
        }

        list_pos = list_pos->next;

        ++pos;
    }

    list_pos->next = 0;

    return cfl_create_new_node_list(head.next);
}

static cfl_node* cfl_subtraction_transform(cfl_node* left, cfl_node* right)
{
    cfl_node* negative = cfl_create_new_node_integer(-1);

    if(!negative)
    {
        cfl_delete_node(left);
        cfl_delete_node(right);

        return 0;
    }

    cfl_node* negated = cfl_create_new_node_multiply(negative, right);

    if(!negated)
    {
        cfl_delete_node(left);

        return 0;
    }

    return cfl_create_new_node_add(left, negated);
}

cfl_node* cfl_parse_subtract(
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
                                   "-",
                                   position,
                                   block))
        return 0;

    return cfl_subtraction_transform(left, right);
}

cfl_node* cfl_parse_mod(
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
                                   "%",
                                   position,
                                   block))
        return 0;

    cfl_node* left_copy = cfl_copy_new_node(left);

    if(!left_copy)
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    cfl_node* right_copy = cfl_copy_new_node(right);

    if(!right_copy)
    {
        cfl_free_node(left);
        cfl_free_node(right);
        cfl_free_node(left_copy);

        return 0;
    }

    cfl_node* factor = cfl_create_new_node_divide(left_copy, right_copy);

    if(!factor)
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    cfl_node* multiple = cfl_create_new_node_multiply(factor, right);

    if(!multiple)
    {
        cfl_free_node(left);

        return 0;
    }

    return cfl_subtraction_transform(left, multiple);
}
