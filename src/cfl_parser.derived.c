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
                                   &cfl_parse_sub_term,
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

cfl_node* cfl_parse_not_equal(
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
                                   "!=",
                                   position,
                                   block))
        return 0;

    cfl_node* equal = cfl_create_new_node_equal(left, right);

    if(!equal)
        return 0;

    return cfl_create_new_node_not(equal);
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

    int length = sprintf(buffer, "~C%d", next_id++);

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

static bool cfl_partial_let_transform(
        cfl_node** argument,
        cfl_node** new_definition,
        cfl_node* name,
        cfl_list_node* arguments,
        cfl_node* definition)
{
    *new_definition = definition;

    if(arguments)
    {
        if(name->type != CFL_NODE_VARIABLE)
        {
            cfl_parse_error_complex_function_name();

            cfl_free_node(name);
            cfl_delete_list_nodes(arguments);
            cfl_free_node(definition);

            return false;
        }

        while(arguments->next)
        {
            cfl_list_node* temp = arguments;

            arguments = arguments->next;

            *new_definition = cfl_create_new_node_function(temp->node, *new_definition);

            if(!*new_definition)
            {
                cfl_free_node(name);
                cfl_delete_list_nodes(arguments);

                return false;
            }

            free(temp);
        }

        *argument = arguments->node;

        free(arguments);
    }
    else 
        *argument = 0;

    return true;
}

static cfl_node* cfl_let_transform(
        cfl_node* name,
        cfl_list_node* arguments,
        cfl_node* definition,
        cfl_node* body)
{
    cfl_node* argument;
    cfl_node* new_definition;

    if(!cfl_partial_let_transform(&argument, &new_definition, name, arguments, definition))
    {
        cfl_free_node(body);

        return 0; 
    }

    if(arguments)
    {
        if(cfl_is_free(name->data, new_definition))
            return cfl_create_new_node_let_rec(name, argument, new_definition, body);
        else
        {
            definition = cfl_create_new_node_function(argument, new_definition);

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
    else
    {
        cfl_node* function = cfl_create_new_node_function(name, body);

        if(!function)
        {
            cfl_free_node(body);

            return 0;
        }

        return cfl_create_new_node_application(function, definition);
    }
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

static cfl_definition_list* cfl_create_definition_list_node(
        cfl_node* name,
        cfl_node* definition)
{
    cfl_definition_list* result = cfl_parser_malloc(sizeof(cfl_definition_list));

    if(!result)
    {
        cfl_free_node(name);
        cfl_free_node(definition);

        return 0;
    }

    result->name = name;
    result->definition = definition;
    result->type = 0;

    return result;
}

static cfl_node* cfl_function_transform(
        cfl_node* name,
        cfl_list_node* arguments,
        cfl_node* definition)
{
    cfl_node* argument;
    cfl_node* new_definition;

    if(!cfl_partial_let_transform(&argument, &new_definition, name, arguments, definition))
        return 0;

    if(argument)
    {
        if(cfl_is_free(name->data, new_definition))
        {
            cfl_node* name_copy = cfl_create_new_node_variable(name->data);

            if(!name_copy)
            {
                cfl_free_node(name);
                cfl_free_node(argument);
                cfl_free_node(new_definition);

                return 0;
            }

            static unsigned int next_id = 0;

            char buffer[100];

            sprintf(buffer, "~F%d", next_id++);

            cfl_node* variable = cfl_create_new_node_variable(buffer);

            if(!variable)
            {
                cfl_free_node(name);
                cfl_free_node(argument);
                cfl_free_node(new_definition);
                cfl_free_node(name_copy);

                return 0;
            }

            cfl_node* body = cfl_create_new_node_application(name_copy, variable);

            if(!body)
            {
                cfl_free_node(name);
                cfl_free_node(argument);
                cfl_free_node(new_definition);

                return 0;
            }

            cfl_node* let_rec = cfl_create_new_node_let_rec(name,
                                                            argument,
                                                            new_definition,
                                                            body);

            if(!let_rec)
                return 0;

            variable = cfl_create_new_node_variable(buffer);

            if(!variable)
            {
                cfl_free_node(let_rec);

                return 0;
            }

            return cfl_create_new_node_function(variable, let_rec);
        }

        new_definition = cfl_create_new_node_function(argument, new_definition);

        if(!new_definition)
        {
            cfl_free_node(name);

            return 0;
        }
    }

    cfl_free_node(name);

    return new_definition;
}

cfl_program* cfl_parse_program(cfl_token_list* position, cfl_token_list* block)
{
    cfl_definition_list head;
    cfl_definition_list* definition_list_pos = &head;
    cfl_node* main_body = 0;

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

                definition_list_pos->next = 0;

                cfl_free_definition_list(head.next);
            }
            else
                cfl_parse_error_partial_program();

            return 0;
        }

        definition_list_pos->next = 0;

        cfl_definition_list* def_pos = head.next;

        for( ; def_pos; def_pos = def_pos->next)
            if(!strcmp(name->data, def_pos->name->data))
            {
                cfl_parse_error_redeclaration(name->data);

                cfl_free_node(name);
                cfl_delete_list_nodes(argument_head.next);
                cfl_free_node(definition);

                cfl_free_definition_list(head.next);

                return 0;
            }

        position = pos;

        if(!strcmp(name->data, "main"))
        {
            if(argument_head.next)
            {
                cfl_parse_error_main_has_arguments();

                cfl_free_node(name);
                cfl_delete_list_nodes(argument_head.next);
                cfl_free_node(definition);

                cfl_free_definition_list(head.next);
    
                return 0;
            }

            if(main_body)
            {
                cfl_parse_error_redeclaration("main");

                cfl_free_node(name);
                cfl_delete_list_nodes(argument_head.next);
                cfl_free_node(definition);

                cfl_free_definition_list(head.next);

                return 0;
            }

            main_body = definition;

            cfl_free_node(name);

            break;
        }

        cfl_node* name_copy = cfl_create_new_node_variable(name->data);

        if(!name_copy)
        {
            cfl_free_node(name);
            cfl_delete_list_nodes(argument_head.next);
            cfl_free_node(definition);

            cfl_free_definition_list(head.next);

            return 0;
        }

        cfl_node* new_definition = cfl_function_transform(name_copy,
                                                          argument_head.next,
                                                          definition);

        if(!new_definition)
        {
            cfl_free_node(name);

            cfl_free_definition_list(head.next);

            return 0;
        }

        definition_list_pos->next =
                cfl_create_definition_list_node(name, new_definition);

        if(!definition_list_pos->next)
        {
            cfl_free_definition_list(head.next);

            return 0;
        }

        definition_list_pos = definition_list_pos->next;

        if(position != block && !cfl_token_string_compare(position, ";", 1))
        {
            semi = true;

            position = position->next;
        }
        else
            break;
    }

    definition_list_pos->next = 0;

    if(position != block)
    {
        cfl_parse_error_partial_program();

        cfl_free_definition_list(head.next);

        if(main_body)
            cfl_free_node(main_body);

        return 0;
    }
    else if(!main_body)
    {
        cfl_parse_error_missing_main();

        cfl_free_definition_list(head.next);

        return 0;
    }

    cfl_program* result = cfl_parser_malloc(sizeof(cfl_program));

    result->definitions = head.next;
    result->main = main_body;
    result->type = 0;

    return result;
}
