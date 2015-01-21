#include "cfl_type.typed_program.h"
#include "cfl_typed_program.h"
#include "cfl_type.h"

#include <string.h>

extern void* cfl_type_malloc(size_t size);
extern unsigned int cfl_type_get_next_id(void);

static unsigned int cfl_lookup_variable(
        cfl_type_equations* equations,
        cfl_type_hypothesis_chain* hypotheses,
        cfl_typed_definition_list* definitions,
        char* name)
{
    return 0;
}

cfl_typed_node* cfl_generate_typed_node(
        cfl_type_equations* equations,
        cfl_type_hypothesis_chain* hypothesis_head,
        cfl_typed_definition_list* definitions,
        cfl_node* node)
{
    if(node->type == CFL_NODE_VARIABLE)
    {
        unsigned int id = cfl_lookup_variable(equations,
                                              hypothesis_head->next,
                                              definitions,
                                              node->data);

        if(!id)
        {
            cfl_type_error_undefined_variable(node->data);

            return 0;
        }

        cfl_type* type = cfl_create_new_type_variable(id);

        if(!type)
            return 0;

        cfl_typed_node* result =
            cfl_create_typed_node(CFL_NODE_VARIABLE, type, 0, node->data, 0);

        node->data = 0;

        return result;
    }
    else if(node->type == CFL_NODE_BOOL)
    {
        cfl_type* type = cfl_create_new_type_bool();

        if(!type)
            return 0;

        cfl_typed_node* result =
            cfl_create_typed_node(CFL_NODE_BOOL, type, 0, node->data, 0);

        node->data = 0;

        return result;
    }
    else if(node->type == CFL_NODE_INTEGER)
    {
        cfl_type* type = cfl_create_new_type_integer();

        if(!type)
            return 0;

        cfl_typed_node* result =
            cfl_create_typed_node(CFL_NODE_INTEGER, type, 0, node->data, 0);

        node->data = 0;

        return result;
    }
    else if(node->type == CFL_NODE_CHAR)
    {
        cfl_type* type = cfl_create_new_type_char();

        if(!type)
            return 0;

        cfl_typed_node* result =
            cfl_create_typed_node(CFL_NODE_CHAR, type, 0, node->data, 0);

        node->data = 0;

        return result;
    }
    else if(node->type == CFL_NODE_LIST)
    {
        unsigned int id = cfl_type_get_next_id();

        cfl_typed_node_list typed_list;
        cfl_typed_node_list* typed_pos = &typed_list;

        cfl_list_node* pos = node->data;

        node->data = 0;

        while(pos)
        {
            cfl_typed_node* child_node = cfl_generate_typed_node(
                equations, hypothesis_head, definitions, pos->node);

            cfl_list_node* temp = pos;

            pos = pos->next;

            free(temp->node);
            free(temp);

            if(!child_node)
            {
                typed_pos->next = 0;

                cfl_free_typed_node_list(typed_list.next);
                cfl_delete_list_nodes(pos);

                return 0;
            }

            cfl_type* child_type = cfl_copy_new_type(child_node->resulting_type);

            if(!child_type)
            {
                cfl_free_typed_node(child_node);

                typed_pos->next = 0;

                cfl_free_typed_node_list(typed_list.next);
                cfl_delete_list_nodes(pos);

                return 0;
            }

            cfl_type* variable = cfl_create_new_type_variable(id);

            if(!variable)
            {
                cfl_free_type(child_type);
                cfl_free_typed_node(child_node);

                typed_pos->next = 0;

                cfl_free_typed_node_list(typed_list.next);
                cfl_delete_list_nodes(pos);

                return 0;
            }

            if(!cfl_add_type_equations(equations, child_type, variable))
            {
                cfl_free_typed_node(child_node);

                typed_pos->next = 0;

                cfl_free_typed_node_list(typed_list.next);
                cfl_delete_list_nodes(pos);

                return 0;
            }

            typed_pos->next = cfl_type_malloc(sizeof(cfl_typed_node_list));

            if(!typed_pos->next)
            {
                cfl_free_typed_node(child_node);
                cfl_free_typed_node_list(typed_list.next);
                cfl_delete_list_nodes(pos);

                return 0;
            }

            typed_pos->next->node = child_node;

            typed_pos = typed_pos->next;
        }

        typed_pos->next = 0;

        cfl_type* variable = cfl_create_new_type_variable(id);

        if(!variable)
        {
            cfl_free_typed_node_list(typed_list.next);

            return 0;
        }

        cfl_type* list_type = cfl_create_new_type_list(variable);

        if(!list_type)
        {
            cfl_free_typed_node_list(typed_list.next);

            return 0;
        }

        return cfl_create_typed_node(CFL_NODE_LIST, list_type, 0, typed_list.next, 0);
    }

    return 0;
}

bool cfl_simplify_typed_node(cfl_type_equations* equations, cfl_typed_node* node)
{
    return true;
}

static cfl_typed_definition_list* cfl_generate_definition_type(
        cfl_type_hypothesis_chain* hypothesis_head,
        cfl_typed_definition_list* definitions,
        unsigned int equation_hash_table_length,
        cfl_definition_list* definition)
{
    cfl_typed_definition_list* result = cfl_type_malloc(sizeof(cfl_typed_definition_list));

    if(!result)
        return 0;

    result->name = cfl_type_malloc(sizeof(char) * MAX_IDENTIFIER_LENGTH);

    if(!result->name)
    {
        free(result);

        return 0;
    }

    strcpy(result->name, definition->name->data);

    cfl_type_equations equations;
    cfl_initialize_type_equations(&equations, equation_hash_table_length);

    result->definition = cfl_generate_typed_node(&equations,
                                                 hypothesis_head,
                                                 definitions,
                                                 definition->definition);

    if(!result->definition)
    {
        cfl_delete_type_equations(&equations);
        free(result->name);
        free(result);

        return 0;
    }

    if(!cfl_close_type_equations(&equations))
    {
        cfl_delete_type_equations(&equations);
        free(result->name);
        cfl_free_typed_node(result->definition);
        free(result);

        return 0;
    }

    if(!cfl_are_type_equations_consistent(&equations))
    {
        cfl_type_error_bad_definition(result->name);

        cfl_delete_type_equations(&equations);
        free(result->name);
        cfl_free_typed_node(result->definition);
        free(result);

        return 0;
    }

    if(!cfl_simplify_typed_node(&equations, result->definition))
    {
        cfl_delete_type_equations(&equations);
        free(result->name);
        cfl_free_typed_node(result->definition);
        free(result);

        return 0;
    }

    cfl_delete_type_equations(&equations);

    return result;
}

cfl_typed_definition_list* cfl_generate_definition_types(
        cfl_type_hypothesis_chain* hypothesis_head,
        cfl_definition_list* definitions,
        unsigned int equation_hash_table_length)
{
    cfl_typed_definition_list definition_head;
    cfl_typed_definition_list* pos = &definition_head;

    while(definitions)
    {
        pos->next = cfl_generate_definition_type(hypothesis_head,
                                                 definition_head.next,
                                                 equation_hash_table_length,
                                                 definitions);

        if(!pos->next)
        {
            cfl_free_typed_definition_list(definition_head.next);

            return 0;
        }

        pos = pos->next;

        pos->next = 0;

        definitions = definitions->next;
    }

    return definition_head.next;
}

cfl_typed_program* cfl_generate_typed_program(
        cfl_program* program,
        unsigned int equation_hash_table_length)
{
    cfl_type_equations equations;

    if(!cfl_initialize_type_equations(&equations, equation_hash_table_length))
        return false;

    cfl_type_hypothesis_chain hypothesis_head;
    hypothesis_head.next = 0;

    cfl_typed_definition_list*  typed_definitions = 0;

    if(program->definitions)
    {
        cfl_typed_definition_list* typed_definitions =  cfl_generate_definition_types(
            &hypothesis_head, program->definitions, equation_hash_table_length);

        if(!typed_definitions)
        {
            cfl_free_program(program);
            cfl_delete_type_equations(&equations);

            return 0;
        }
    }

    cfl_typed_node* typed_main = cfl_generate_typed_node(&equations,
                                                         &hypothesis_head,
                                                         typed_definitions,
                                                         program->main);

    if(!typed_main)
    {
        cfl_free_typed_definition_list(typed_definitions);
        cfl_free_program(program);
        cfl_delete_type_equations(&equations);

        return 0;
    }

    cfl_free_program(program);

    if(!cfl_close_type_equations(&equations))
    {
        cfl_free_typed_node(typed_main);
        cfl_free_typed_definition_list(typed_definitions);
        cfl_delete_type_equations(&equations);

        return 0;
    }

    if(!cfl_are_type_equations_consistent(&equations))
    {
        cfl_type_error_failure();

        cfl_free_typed_node(typed_main);
        cfl_free_typed_definition_list(typed_definitions);
        cfl_delete_type_equations(&equations);

        return 0;
    }

    if(!cfl_simplify_typed_node(&equations, typed_main))
    {
        cfl_free_typed_node(typed_main);
        cfl_free_typed_definition_list(typed_definitions);
        cfl_delete_type_equations(&equations);

        return 0;
    }

    cfl_typed_program* result = cfl_type_malloc(sizeof(cfl_typed_program));

    if(!result)
    {
        cfl_free_typed_node(typed_main);
        cfl_free_typed_definition_list(typed_definitions);
        cfl_delete_type_equations(&equations);

        return 0;
    }

    cfl_delete_type_equations(&equations);

    result->definitions = typed_definitions;
    result->main = typed_main;

    return result;
}
