#include "cfl_program.h"

void cfl_free_definition_list(cfl_definition_list* list)
{
    while(list)
    {
        cfl_definition_list* temp = list;

        list = list->next;

        cfl_free_node(temp->name);
        cfl_delete_list_nodes(temp->arguments);
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
}
