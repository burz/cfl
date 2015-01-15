#include "cfl_parser.h"

#include <stdio.h>
#include <string.h>

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

    *end = position->next;

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

static cfl_node* cfl_less_equal_transform(cfl_node* left, cfl_node* right)
{
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

    cfl_node* less = cfl_create_new_node_less(left_copy, right_copy);

    if(!less)
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    cfl_node* equal = cfl_create_new_node_equal(left, right);

    if(!equal)
    {
        cfl_free_node(equal);

        return 0;
    }

    return cfl_create_new_node_or(less, equal);
}

cfl_node* cfl_parse_less_equal(
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
                                   2,
                                   "<=",
                                   position,
                                   block))
        return 0;

    return cfl_less_equal_transform(left, right);
}

cfl_node* cfl_parse_greater(
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
                                   ">",
                                   position,
                                   block))
        return 0;

    cfl_node* less_equal = cfl_less_equal_transform(left, right);

    if(!less_equal)
        return 0;

    return cfl_create_new_node_not(less_equal);
}

cfl_node* cfl_parse_greater_equal(
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
                                   ">=",
                                   position,
                                   block))
        return 0;

    cfl_node* less = cfl_create_new_node_less(left, right);

    if(!less)
        return 0;

    return cfl_create_new_node_not(less);
}

cfl_node* cfl_parse_composition(
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
                                   &cfl_parse_function_factor,
                                   1,
                                   ".",
                                   position,
                                   block))
        return 0;

    static unsigned int next_id = 0;

    char buffer[100];

    int length = sprintf(buffer, "_C%d", next_id++);

    cfl_node* variable = cfl_create_new_node_variable_n(length, buffer);

    if(!variable)
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    cfl_node* inner_application = cfl_create_new_node_application(right, variable);

    if(!inner_application)
    {
        cfl_free_node(left);

        return 0;
    }

    cfl_node* outer_application = cfl_create_new_node_application(left, inner_application);

    if(!outer_application)
        return 0;

    variable = cfl_create_new_node_variable_n(length, buffer);

    if(!variable)
    {
        cfl_free_node(outer_application);

        return 0;
    }

    return cfl_create_new_node_function(variable, outer_application);
}

cfl_node* cfl_parse_applicative(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    cfl_node* left;
    cfl_node* right;

    if(!cfl_parse_binary_operation(end,
                                   &left,
                                   &right,
                                   &cfl_parse_function_factor,
                                   &cfl_parse_expression,
                                   1,
                                   "$",
                                   position,
                                   block))
        return 0;

    return cfl_create_new_node_application(left, right);
}

bool cfl_parse_def(cfl_token_list** end,
                   cfl_node** name,
                   cfl_list_node* argument_head,
                   cfl_node** definition,
                   cfl_token_list* position,
                   cfl_token_list* block)
{
    *name = cfl_parse_complex_variable(&position, position, block);

    if(!*name)
        return false;

    cfl_token_list* equal_pos = cfl_lookahead_for(1, "=", position, block);

    if(!equal_pos)
    {
        cfl_parse_error_no_equal_after_def((*name)->data);

        cfl_free_node(*name);

        return false;
    }

    while(position != equal_pos)
    {
        cfl_token_list* pos;
        cfl_node* argument = cfl_parse_complex_variable(&pos, position, equal_pos);

        if(!argument)
        {
            cfl_parse_error_bad_arguments_after_def((*name)->data);

            cfl_delete_list_nodes(argument_head->next);
            cfl_free_node(*name);

            return false;
        }

        cfl_list_node* new_node = cfl_parser_malloc(sizeof(cfl_list_node));

        if(!new_node)
        {
            cfl_delete_list_nodes(argument_head->next);
            cfl_free_node(argument);
            cfl_free_node(*name);

            return false;
        }

        new_node->node = argument;
        new_node->next = argument_head->next;

        argument_head->next = new_node;

        position = pos;
    }

    position = equal_pos->next;

    *definition = cfl_parse_expression(end, position, block);

    if(!*definition)
    {
        cfl_delete_list_nodes(argument_head->next);
        cfl_free_node(*name);

        return false;
    }

    return true;
}

static cfl_node* cfl_let_transform(
        cfl_node* name,
        cfl_list_node* arguments,
        cfl_node* definition,
        cfl_node* body)
{
    if(arguments)
    {
        if(name->type != CFL_NODE_VARIABLE)
        {
            cfl_parse_error_complex_function_name();

            cfl_free_node(name);
            cfl_delete_list_nodes(arguments);
            cfl_free_node(definition);
            cfl_free_node(body);

            return 0;
        }

        while(arguments->next)
        {
            cfl_list_node* temp = arguments;

            arguments = arguments->next;

            definition = cfl_create_new_node_function(temp->node, definition);

            if(!definition)
            {
                cfl_free_node(name);
                cfl_delete_list_nodes(arguments);
                cfl_free_node(body);

                return 0;
            }

            free(temp);
        }

        cfl_node* argument = arguments->node;

        free(arguments);

        if(cfl_is_free(name->data, definition))
            return cfl_create_new_node_let_rec(name, argument, definition, body);
        else
        {
            definition = cfl_create_new_node_function(argument, definition);

            if(!body)
            {
                cfl_free_node(name);
                cfl_free_node(body);

                return 0;
            }

            cfl_node* function = cfl_create_new_node_function(name, body);

            if(!function)
            {
                cfl_free_node(definition);

                return 0;
            }

            return cfl_create_new_node_application(function, definition);
        }
    }

    cfl_node* function = cfl_create_new_node_function(name, body);

    if(!function)
    {
        cfl_free_node(body);

        return 0;
    }

    return cfl_create_new_node_application(function, definition);
}

cfl_node* cfl_parse_let(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(cfl_token_string_compare(position, "let", 3))
        return 0;

    position = position->next;

    cfl_token_list* in_pos = cfl_lookahead_for(2, "in", position, block);

    if(!in_pos)
    {
        cfl_parse_error_expected("\"in\"", "\"let\"",
                                 position->start, position->end);

        return 0;
    }

    cfl_node* name;
    cfl_node* definition;
    cfl_token_list* pos;
    cfl_list_node argument_head;
    argument_head.next = 0;

    if(!cfl_parse_def(&pos, &name, &argument_head,
                      &definition, position, in_pos))
    {
        cfl_parse_error_expected("definition", "\"let\"",
                                 position->start, position->end);

        return 0;
    }

    if(pos != in_pos)
    {
        cfl_parse_error_expected("definition", "\"let\"",
                                 position->start, position->end);

        cfl_free_node(name);
        cfl_delete_list_nodes(argument_head.next);
        cfl_free_node(definition);

        return 0;
    }

    position = in_pos->next;

    cfl_node* body = cfl_parse_expression(end, position, block);

    if(!body)
    {
        cfl_parse_error_expected("expression", "\"in\"",
                                 position->start, position->end);

        cfl_free_node(name);
        cfl_delete_list_nodes(argument_head.next);
        cfl_free_node(definition);

        return 0;
    }

    return cfl_let_transform(name, argument_head.next, definition, body);
}

typedef struct cfl_program_list_t {
    cfl_node* name;
    cfl_list_node* arguments;
    cfl_node* definition;
    struct cfl_program_list_t* next;
} cfl_program_list;

static void cfl_delete_program_list(cfl_program_list* list)
{
    while(list)
    {
        cfl_program_list* temp = list;

        list = list->next;

        cfl_free_node(temp->name);
        cfl_delete_list_nodes(temp->arguments);
        cfl_free_node(temp->definition);
        free(temp);
    }
}

cfl_node* cfl_parse_program(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    cfl_program_list head;
    head.next = 0;

    bool semi = false;

    while(position != block)
    {
        cfl_node* name;
        cfl_node* definition;
        cfl_list_node argument_head;
        argument_head.next = 0;

        cfl_token_list* pos;

        if(!cfl_parse_def(&pos, &name, &argument_head,
                          &definition, position, block))
        {
            if(semi)
            {
                cfl_parse_error_expected("statement", "\";\"",
                                         position->start, position->end);

                cfl_delete_program_list(head.next);
            }
            else
                cfl_parse_error_partial_program();

            return 0;
        }

        position = pos;

        cfl_program_list* new_statement =
            cfl_parser_malloc(sizeof(cfl_program_list));

        if(!new_statement)
        {
            cfl_free_node(name);
            cfl_delete_list_nodes(argument_head.next);
            cfl_free_node(definition);
            cfl_delete_program_list(head.next);

            return 0;
        }

        new_statement->name = name;
        new_statement->arguments = argument_head.next;
        new_statement->definition = definition;
        new_statement->next = head.next;

        head.next = new_statement;

        if(position != block && !cfl_token_string_compare(position, ";", 1))
        {
            semi = true;

            position = position->next;
        }
        else
            break;
    }

    if(position != block)
    {
        cfl_parse_error_partial_program();

        cfl_delete_program_list(head.next);

        return 0;
    }
    else if(strncmp(head.next->name->data, "main", 4))
    {
        cfl_parse_error_missing_main();

        cfl_delete_program_list(head.next);

        return 0;
    }
    else if(head.next->arguments)
    {
        cfl_parse_error_main_has_arguments();

        cfl_delete_program_list(head.next);

        return 0;
    }

    cfl_program_list* temp = head.next;

    head.next = temp->next;

    cfl_node* body = temp->definition;

    cfl_free_node(temp->name);
    free(temp);

    while(head.next)
    {
        temp = head.next;

        head.next = temp->next;

        body = cfl_let_transform(temp->name,
                                 temp->arguments,
                                 temp->definition,
                                 body);

        free(temp);

        if(!body)
        {
            cfl_delete_program_list(head.next);

            return 0;
        }
    }

    *end = position;

    return body;
}
