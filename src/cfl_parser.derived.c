#include "cfl_parser.h"

#include <stdlib.h>
#include <string.h>

static int cfl_subtraction_transform(cfl_node* node, cfl_node* left, cfl_node* right)
{
    cfl_node* negative = malloc(sizeof(cfl_node));

    if(!negative)
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    if(!cfl_create_node_integer(negative, -1))
    {
        cfl_free_node(left);
        cfl_free_node(right);
        free(negative);

        return 0;
    }

    cfl_node* negation = malloc(sizeof(cfl_node));

    if(!negation)
    {
        cfl_free_node(left);
        cfl_free_node(right);
        cfl_free_node(negative);

        return 0;
    }

    if(!cfl_create_node_multiply(negation, negative, right))
    {
        cfl_free_node(left);
        cfl_free_node(right);
        cfl_free_node(negative);
        free(negation);

        return 0;
    }

    if(!cfl_create_node_add(node, left, negation))
    {
        cfl_free_node(left);
        cfl_free_node(negation);

        return 0;
    }

    return 1;
}

char* cfl_parse_subtract(cfl_node* node, char* start, char* end)
{
    cfl_node* left = malloc(sizeof(cfl_node));

    if(!left)
        return 0;

    cfl_node* right = malloc(sizeof(cfl_node));

    if(!right)
    {
        free(left);

        return 0;
    }

    start = cfl_parse_binary_operation(left,
                                       right,
                                       &cfl_parse_molecule,
                                       &cfl_parse_factor,
                                       1,
                                       "-",
                                       start,
                                       end);

    if(!start)
    {
        free(left);
        free(right);

        return 0;
    }

    if(!cfl_subtraction_transform(node, left, right))
        return 0;

    return start;
}

char* cfl_parse_mod(cfl_node* node, char* start, char* end)
{
    cfl_node* left = malloc(sizeof(cfl_node));

    if(!left)
        return 0;

    cfl_node* right = malloc(sizeof(cfl_node));

    if(!right)
    {
        free(left);

        return 0;
    }

    start = cfl_parse_binary_operation(left,
                                       right,
                                       &cfl_parse_factor,
                                       &cfl_parse_term,
                                       1,
                                       "%",
                                       start,
                                       end);

    if(!start)
    {
        free(left);
        free(right);

        return 0;
    }

    cfl_node* left_copy = malloc(sizeof(cfl_node));

    if(!left_copy)
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    if(!cfl_copy_node(left_copy, left))
    {
        cfl_free_node(left);
        cfl_free_node(right);
        free(left_copy);

        return 0;
    }

    cfl_node* right_copy = malloc(sizeof(cfl_node));

    if(!right_copy)
    {
        cfl_free_node(left);
        cfl_free_node(right);
        cfl_free_node(left_copy);

        return 0;
    }

    if(!cfl_copy_node(right_copy, right))
    {
        cfl_free_node(left);
        cfl_free_node(right);
        cfl_free_node(left_copy);
        free(right_copy);

        return 0;
    }

    cfl_node* factor = malloc(sizeof(cfl_node));

    if(!factor)
    {
        cfl_free_node(left);
        cfl_free_node(right);
        cfl_free_node(left_copy);
        cfl_free_node(right_copy);

        return 0;
    }

    if(!cfl_create_node_divide(factor, left_copy, right_copy))
    {
        cfl_free_node(left);
        cfl_free_node(right);
        cfl_free_node(left_copy);
        cfl_free_node(right_copy);
        free(factor);

        return 0;
    }

    cfl_node* reduction = malloc(sizeof(cfl_node));

    if(!reduction)
    {
        cfl_free_node(left);
        cfl_free_node(right);
        cfl_free_node(factor);

        return 0;
    }

    if(!cfl_create_node_multiply(reduction, factor, right))
    {
        cfl_free_node(left);
        cfl_free_node(right);
        cfl_free_node(factor);
        free(reduction);

        return 0;
    }

    if(!cfl_subtraction_transform(node, left, reduction))
        return 0;

    return start;
}

static int cfl_less_equal_transform(
        cfl_node* node,
        cfl_node* left,
        cfl_node* right)
{
    cfl_node* left_copy = malloc(sizeof(cfl_node));

    if(!left_copy)
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    if(!cfl_copy_node(left_copy, left))
    {
        cfl_free_node(left);
        cfl_free_node(right);
        free(left_copy);

        return 0;
    }

    cfl_node* right_copy = malloc(sizeof(cfl_node));

    if(!right_copy)
    {
        cfl_free_node(left);
        cfl_free_node(right);
        cfl_free_node(left_copy);

        return 0;
    }

    if(!cfl_copy_node(right_copy, right))
    {
        cfl_free_node(left);
        cfl_free_node(right);
        cfl_free_node(left_copy);
        free(right_copy);

        return 0;
    }

    cfl_node* less = malloc(sizeof(cfl_node));

    if(!less)
    {
        cfl_free_node(left);
        cfl_free_node(right);
        cfl_free_node(left_copy);
        cfl_free_node(right_copy);

        return 0;
    }

    if(!cfl_create_node_less(less, left, right))
    {
        cfl_free_node(left);
        cfl_free_node(right);
        cfl_free_node(left_copy);
        cfl_free_node(right_copy);
        free(less);

        return 0;
    }

    cfl_node* equal = malloc(sizeof(cfl_node));

    if(!equal)
    {
        cfl_free_node(less);
        cfl_free_node(left_copy);
        cfl_free_node(right_copy);

        return 0;
    }

    if(!cfl_create_node_equal(equal, left_copy, right_copy))
    {
        cfl_free_node(less);
        cfl_free_node(left_copy);
        cfl_free_node(right_copy);
        free(equal);

        return 0;
    }

    if(!cfl_create_node_or(node, less, equal))
    {
        cfl_free_node(less);
        cfl_free_node(equal);

        return 0;
    }

    return 1;
}

char* cfl_parse_less_equal(cfl_node* node, char* start, char* end)
{
    cfl_node* left = malloc(sizeof(cfl_node));

    if(!left)
        return 0;

    cfl_node* right = malloc(sizeof(cfl_node));

    if(!right)
    {
        free(left);

        return 0;
    }

    start = cfl_parse_binary_operation(left,
                                       right,
                                       &cfl_parse_molecule,
                                       &cfl_parse_molecule,
                                       2,
                                       "<=",
                                       start,
                                       end);

    if(!start)
    {
        free(left);
        free(right);

        return 0;
    }

    if(!cfl_less_equal_transform(node, left, right))
        return 0;

    return start;
}

char* cfl_parse_greater(cfl_node* node, char* start, char* end)
{
    cfl_node* left = malloc(sizeof(cfl_node));

    if(!left)
        return 0;

    cfl_node* right = malloc(sizeof(cfl_node));

    if(!right)
    {
        free(left);

        return 0;
    }

    start = cfl_parse_binary_operation(left,
                                       right,
                                       &cfl_parse_molecule,
                                       &cfl_parse_molecule,
                                       1,
                                       ">",
                                       start,
                                       end);

    if(!start)
    {
        free(left);
        free(right);

        return 0;
    }

    cfl_node* less_equal = malloc(sizeof(cfl_node));

    if(!less_equal)
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    if(!cfl_less_equal_transform(less_equal, left, right))
        return 0;

    if(!cfl_create_node_not(node, less_equal))
    {
        cfl_free_node(less_equal);

        return 0;
    }

    return start;
}

char* cfl_parse_greater_equal(cfl_node* node, char* start, char* end)
{
    cfl_node* left = malloc(sizeof(cfl_node));

    if(!left)
        return 0;

    cfl_node* right = malloc(sizeof(cfl_node));

    if(!right)
    {
        free(left);

        return 0;
    }

    start = cfl_parse_binary_operation(left,
                                       right,
                                       &cfl_parse_molecule,
                                       &cfl_parse_molecule,
                                       2,
                                       ">=",
                                       start,
                                       end);

    if(!start)
    {
        free(left);
        free(right);

        return 0;
    }

    cfl_node* less = malloc(sizeof(cfl_node));

    if(!less)
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    if(!cfl_create_node_less(less, left, right))
    {
        cfl_free_node(left);
        cfl_free_node(right);
        free(less);

        return 0;
    }

    if(!cfl_create_node_not(node, less))
    {
        cfl_free_node(less);

        return 0;
    }

    return start;
}

struct cfl_argument_chain_node {
    cfl_node* argument;
    struct cfl_argument_chain_node* next;
};

static void cfl_delete_argument_chain(struct cfl_argument_chain_node* start)
{
    while(start)
    {
        struct cfl_argument_chain_node* temp = start;

        start = start->next;

        cfl_free_node(temp->argument);
        free(temp);
    }
}

static int cfl_construct_function_chain(
        cfl_node* node,
        struct cfl_argument_chain_node* head,
        cfl_node* body,
        int is_recursive)
{
    struct cfl_argument_chain_node* pos = head->next;

    while(pos)
    {
        if(is_recursive && pos->next == 0)
            break;

        cfl_node* function = malloc(sizeof(cfl_node));

        if(!function)
        {
            while(pos)
            {
                struct cfl_argument_chain_node* temp = pos;

                pos = pos->next;

                cfl_free_node(temp->argument);
                free(temp);
            }

            cfl_free_node(body);

            return 0;
        }

        if(!cfl_create_node_function(function, pos->argument, body))
        {
            while(pos)
            {
                struct cfl_argument_chain_node* temp = pos;

                pos = pos->next;

                cfl_free_node(temp->argument);
                free(temp);
            }

            cfl_free_node(body);
            free(function);

            return 0;
        }

        body = function;

        struct cfl_argument_chain_node* temp = pos;

        pos = pos->next;

        head->next = pos;

        free(temp);
    }

    *node = *body;

    free(body);

    return 1;
}

static int cfl_is_free(char* name, cfl_node* body)
{
    int i;

    switch(body->type)
    {
        case CFL_NODE_VARIABLE:
            if(!strcmp(name, body->data))
                return 1;
            break;
        case CFL_NODE_FUNCTION:
            if(strcmp(name, body->children[0]->data))
                return cfl_is_free(name, body->children[1]);
            break;
        case CFL_NODE_LET_REC:
            if(strcmp(name, body->children[0]->data))
            {
                if(strcmp(name, body->children[1]->data))
                {
                    return cfl_is_free(name, body->children[2]) ||
                           cfl_is_free(name, body->children[3]);
                }
                else
                    return cfl_is_free(name, body->children[3]);
            }
            break;
        default:
            for(i = 0; i < body->number_of_children; ++i)
                if(cfl_is_free(name, body->children[i]))
                    return 1;
            break;
    }

    return 0;
}

char* cfl_parse_let(cfl_node* node, char* start, char* end)
{
    if(end - start < 3 || start[0] != 'l' ||
       start[1] != 'e' || start[2] != 't')
        return 0;

    start = cfl_parse_whitespace(start + 3, end);

    cfl_node* name = malloc(sizeof(cfl_node));

    if(!name)
        return 0;

    start = cfl_parse_variable(name, start, end);

    if(!start)
    {
        free(name);

        return 0;
    }

    struct cfl_argument_chain_node argument_chain;
    argument_chain.next = 0;

    while(start != end)
    {
        start = cfl_parse_whitespace(start, end);

        cfl_node* argument = malloc(sizeof(cfl_node));

        if(!argument)
        {
            free(name);
            cfl_delete_argument_chain(argument_chain.next);

            return 0;
        }

        char* pos = cfl_parse_variable(argument, start, end);

        if(!pos)
        {
            free(argument);

            break;
        }

        start = pos;

        struct cfl_argument_chain_node* temp = argument_chain.next;

        argument_chain.next = malloc(sizeof(struct cfl_argument_chain_node));

        if(!argument_chain.next)
        {
            free(name);
            free(argument);
            cfl_delete_argument_chain(temp);

            return 0;
        }

        argument_chain.next->argument = argument;
        argument_chain.next->next = temp;
    }

    if(end - start < 1 || *start != '=')
    {
        cfl_free_node(name);
        cfl_delete_argument_chain(argument_chain.next);

        return 0;
    }

    start = cfl_parse_whitespace(++start, end);

    char* in_pos = start;
    int depth = 1;

    while(in_pos != end)
    {
        if(end - in_pos > 2 && in_pos[0] == 'l' &&
           in_pos[1] == 'e' && in_pos[2] == 't')
            ++depth;
        else if(end - in_pos > 1 && in_pos[0] == 'i' && in_pos[1] == 'n')
            if(--depth == 0)
                break;

        ++in_pos;
    }

    if(in_pos == end)
    {
        cfl_free_node(name);
        cfl_delete_argument_chain(argument_chain.next);

        return 0;
    }

    cfl_node* value = malloc(sizeof(cfl_node));

    if(!value)
    {
        cfl_free_node(name);
        cfl_delete_argument_chain(argument_chain.next);

        return 0;
    }

    start = cfl_parse_expression(value, start, in_pos);

    if(!start)
    {
        cfl_free_node(name);
        cfl_delete_argument_chain(argument_chain.next);
        free(value);

        return 0;
    }

    start = cfl_parse_whitespace(start, in_pos);

    if(start != in_pos)
    {
        cfl_free_node(name);
        cfl_delete_argument_chain(argument_chain.next);
        cfl_free_node(value);

        return 0;
    }

    cfl_node* expanded_value = malloc(sizeof(cfl_node));

    if(!expanded_value)
    {
        cfl_free_node(name);
        cfl_delete_argument_chain(argument_chain.next);
        cfl_free_node(value);

        return 0;
    }

    if(!cfl_construct_function_chain(expanded_value,
                                     &argument_chain,
                                     value,
                                     cfl_is_free(name->data, value)))
    {
        cfl_free_node(name);
        free(expanded_value);

        return 0;
    }

    start = cfl_parse_whitespace(in_pos + 2, end);

    cfl_node* body = malloc(sizeof(cfl_node));

    if(!body)
    {
        cfl_free_node(name);
        cfl_delete_argument_chain(argument_chain.next);
        cfl_free_node(expanded_value);

        return 0;
    }

    start = cfl_parse_expression(body, start, end);

    if(!start)
    {
        cfl_free_node(name);
        cfl_delete_argument_chain(argument_chain.next);
        cfl_free_node(expanded_value);
        free(body);

        return 0;
    }

    if(!argument_chain.next)
    {
        cfl_node* function = malloc(sizeof(cfl_node));

        if(!function)
        {
            cfl_free_node(name);
            cfl_free_node(expanded_value);
            cfl_free_node(body);

            return 0;
        }

        if(!cfl_create_node_function(function, name, body))
        {
            cfl_free_node(name);
            cfl_free_node(expanded_value);
            cfl_free_node(body);
            free(function);

            return 0;
        }

        if(!cfl_create_node_application(node, function, expanded_value))
        {
            cfl_free_node(expanded_value);
            cfl_free_node(function);

            return 0;
        }
    }
    else
    {
        cfl_node* argument = argument_chain.next->argument;

        free(argument_chain.next);

        if(!cfl_create_node_let_rec(node,
                                    name,
                                    argument,
                                    expanded_value,
                                    body))
        {
            cfl_free_node(name);
            cfl_free_node(argument);
            cfl_free_node(expanded_value);
            cfl_free_node(body);

            return 0;
        }
    }

    return start;
}
