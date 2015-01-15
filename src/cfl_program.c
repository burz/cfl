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

    free(program);
}

static void cfl_reverse_print_definitions(cfl_definition_list* definitions)
{
    if(!definitions)
        return;

    cfl_reverse_print_definitions(definitions->next);

    printf("%s ::= ", (char*) definitions->name->data);

    cfl_print_node(definitions->definition);
}

void cfl_print_program(cfl_program* program)
{
    cfl_reverse_print_definitions(program->definitions);

    printf("main ::= ");

    cfl_print_node(program->main);
}
