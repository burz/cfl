#include "cfl_ast.h"

#include <stdlib.h>

int cfl_create_node_bool(cfl_node* node, bool value)
{
    node->number_of_children = 0;
    node->data = malloc(sizeof(bool));

    if(!node->data)
        return 0;

    return 1;
}

int cfl_create_node_variable(cfl_node* node, const char* string)
{
    node->number_of_children = 0;
    node->data = malloc(sizeof(char) * MAX_IDENTIFIER_LENGTH);

    if(!node->data)
        return 0;

    return 1;
}

int cfl_create_node_and(cfl_node* node, cfl_node* left, cfl_node* right)
{
    node->number_of_children = 2;
    node->data = 0;
    node->children = malloc(sizeof(cfl_node*) * 2);

    if(!node->children)
        return 0;

    node->children[0] = left;
    node->children[0] = right;

    return 1;
}

int cfl_create_node_or(cfl_node* node, cfl_node* left, cfl_node* right)
{
    node->number_of_children = 2;
    node->data = 0;
    node->children = malloc(sizeof(cfl_node*) * 2);

    if(!node->children)
        return 0;

    node->children[0] = left;
    node->children[0] = right;

    return 1;
}

int cfl_create_node_not(cfl_node* node, cfl_node* child)
{
    node->number_of_children = 1;
    node->data = 0;
    node->children = malloc(sizeof(cfl_node*));

    if(!node->children)
        return 0;

    *node->children = child;

    return 1;
}

void cfl_delete_node(cfl_node* node)
{
    if(node->data)
        free(node->data);

    int i = 0;

    for( ; i < node->number_of_children; ++i)
    {
        cfl_delete_node(node->children[i]);

        free(node->children[i]);
    }

    free(node->children);
}
