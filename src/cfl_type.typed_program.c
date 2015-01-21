#include "cfl_type.typed_program.h"
#include "cfl_type.h"

#include <string.h>

extern void* cfl_type_malloc(size_t size);

cfl_typed_node* cfl_generate_typed_node(
        cfl_type_equations* equations,
        cfl_type_hypothesis_chain* hypothesis_head,
        cfl_typed_definition_list* definitons,
        cfl_node* node)
{
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
        cfl_type_equations* equations,
        cfl_type_hypothesis_chain* hypothesis_head,
        cfl_program* program)
{
    cfl_typed_definition_list* typed_definitions = 
        cfl_generate_definition_types(hypothesis_head, program->definitions,
                                      equations->equation_hash_table_length);

    if(!typed_definitions)
    {
        cfl_free_program(program);
        cfl_free_type_equations(equations);

        return 0;
    }

    cfl_typed_node* typed_main = cfl_generate_typed_node(equations,
                                                         hypothesis_head,
                                                         typed_definitions,
                                                         program->main);

    if(!typed_main)
    {
        cfl_free_typed_definition_list(typed_definitions);
        cfl_free_program(program);
        cfl_free_type_equations(equations);

        return 0;
    }

    cfl_free_program(program);

    if(!cfl_simplify_typed_node(equations, typed_main))
    {
        cfl_free_typed_definition_list(typed_definitions);
        cfl_free_type_equations(equations);

        return 0;
    }

    cfl_typed_program* result = cfl_type_malloc(sizeof(cfl_typed_program));

    if(!result)
    {
        cfl_free_typed_node(typed_main);
        cfl_free_typed_definition_list(typed_definitions);
        cfl_free_type_equations(equations);

        return 0;
    }

    cfl_free_type_equations(equations);

    result->definitions = typed_definitions;
    result->main = typed_main;

    return result;
}
