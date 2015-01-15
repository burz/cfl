#include "cfl_program.h"

#include <stdio.h>

void cfl_free_definition_list(cfl_definition_list* list)
{
    while(list)
    {
        cfl_definition_list* temp = list;

        list = list->next;

        cfl_free_node(temp->name);
        cfl_free_node(temp->definition);

        if(temp->type)
            cfl_free_type(temp->type);

        free(temp);
    }
}

void cfl_free_program(cfl_program* program)
{
    cfl_free_definition_list(program->definitions);

    cfl_free_node(program->main);

    if(program->type)
        cfl_free_type(program->type);

    free(program);
}

static void cfl_print_definitions(cfl_definition_list* definitions)
{
    if(!definitions)
        return;

    printf("%s ::= ", (char*) definitions->name->data);

    cfl_print_node(definitions->definition);

    cfl_print_definitions(definitions->next);
}

void cfl_print_program(cfl_program* program)
{
    cfl_print_definitions(program->definitions);

    printf("main ::= ");

    cfl_print_node(program->main);
}

static void cfl_print_definition_types(cfl_definition_list* definitions)
{
    if(!definitions)
        return;

    printf("%s => ", (char*) definitions->name->data);

    cfl_print_type(definitions->type);

    cfl_print_definition_types(definitions->next);
}

void cfl_print_program_type(cfl_program* program)
{
    cfl_print_definition_types(program->definitions);

    printf("main => ");

    cfl_print_type(program->type);
}
