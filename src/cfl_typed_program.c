#include "cfl_typed_program.h"
#include "cfl_type.h"

extern void* cfl_ast_malloc(size_t size);

cfl_typed_node* cfl_create_typed_node(
        cfl_node_type node_type,
        cfl_type* resulting_type,
        unsigned int number_of_children,
        void* data,
        cfl_typed_node** children)
{
    cfl_typed_node* result = cfl_ast_malloc(sizeof(cfl_typed_node));

    if(!result)
    {
        cfl_free_type(resulting_type);

        if(data)
            free(data);

        int i = 0;
        for( ; i < number_of_children; ++i)
        {
            cfl_free_typed_node(children[i]);
        }

        free(children);

        return 0;
    }

    result->node_type = node_type;
    result->resulting_type = resulting_type;
    result->number_of_children = number_of_children;
    result->data = data;
    result->children = children;

    return result;
}

void cfl_free_typed_node(cfl_typed_node* node)
{
    cfl_free_type(node->resulting_type);

    if(node->data)
        free(node->data);

    int i = 0;
    for( ; i < node->number_of_children; ++i)
        cfl_free_typed_node(node->children[i]);

    free(node->children);
    free(node);
}

void cfl_free_typed_definition_list(cfl_typed_definition_list* list)
{
    while(list)
    {
        cfl_typed_definition_list* temp = list;

        list = list->next;

        free(temp->name);
        cfl_free_typed_node(temp->definition);
        free(temp);
    }
}

void cfl_free_typed_program(cfl_typed_program* program)
{
    cfl_free_typed_definition_list(program->definitions);
    cfl_free_typed_node(program->main);
    free(program);
}

static void cfl_print_typed_program_inner(cfl_typed_node* node)
{
}

void cfl_print_typed_program(cfl_typed_program* program)
{
    printf("ppoop\n");
}
