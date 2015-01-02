#include "cfl_ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cfl_create_node_bool(cfl_node* node, bool value)
{
    node->type = CFL_NODE_BOOL;
    node->number_of_children = 0;
    node->data = malloc(sizeof(bool));

    if(!node->data)
        return 0;

    *((bool*) node->data) = value;

    return 1;
}

int cfl_create_node_variable(cfl_node* node, const char* string)
{
    node->type = CFL_NODE_VARIABLE;
    node->number_of_children = 0;
    node->data = malloc(sizeof(char) * MAX_IDENTIFIER_LENGTH);

    if(!node->data)
        return 0;

    strncpy(node->data, string, MAX_IDENTIFIER_LENGTH);

    ((char*) node->data)[MAX_IDENTIFIER_LENGTH - 1] = 0;

    return 1;
}

int cfl_create_node_and(cfl_node* node, cfl_node* left, cfl_node* right)
{
    node->type = CFL_NODE_AND;
    node->number_of_children = 2;
    node->data = 0;
    node->children = malloc(sizeof(cfl_node*) * 2);

    if(!node->children)
        return 0;

    node->children[0] = left;
    node->children[1] = right;

    return 1;
}

int cfl_create_node_or(cfl_node* node, cfl_node* left, cfl_node* right)
{
    node->type = CFL_NODE_OR;
    node->number_of_children = 2;
    node->data = 0;
    node->children = malloc(sizeof(cfl_node*) * 2);

    if(!node->children)
        return 0;

    node->children[0] = left;
    node->children[1] = right;

    return 1;
}

int cfl_create_node_not(cfl_node* node, cfl_node* child)
{
    node->type = CFL_NODE_NOT;
    node->number_of_children = 1;
    node->data = 0;
    node->children = malloc(sizeof(cfl_node*));

    if(!node->children)
        return 0;

    *node->children = child;

    return 1;
}

int cfl_create_node_if(
        cfl_node* node,
        cfl_node* condition,
        cfl_node* then_node,
        cfl_node* else_node)
{
    node->type = CFL_NODE_IF;
    node->number_of_children = 3;
    node->data = 0;
    node->children = malloc(sizeof(cfl_node*) * 3);

    if(!node->children)
        return 0;

    node->children[0] = condition;
    node->children[1] = then_node;
    node->children[2] = else_node;

    return 1;
}

void cfl_delete_node(cfl_node* node)
{
    if(node->data)
        free(node->data);

    if(node->number_of_children)
    {
        int i = 0;

        for( ; i < node->number_of_children; ++i)
        {
            cfl_delete_node(node->children[i]);

            free(node->children[i]);
        }

        free(node->children);
    }
}

void cfl_print_node_inner(cfl_node* node)
{
    switch(node->type)
    {
        case CFL_NODE_BOOL:
            printf(*((bool*) node->data) ? "true" : "false");
            break;
        case CFL_NODE_VARIABLE:
            printf("%s", (char*) node->data);
            break;
        case CFL_NODE_AND:
            printf("(");
            cfl_print_node_inner(node->children[0]);
            printf(") && (");
            cfl_print_node_inner(node->children[1]);
            printf(")");
            break;
        case CFL_NODE_OR:
            printf("(");
            cfl_print_node_inner(node->children[0]);
            printf(") || (");
            cfl_print_node_inner(node->children[1]);
            printf(")");
            break;
        case CFL_NODE_NOT:
            printf("!(");
            cfl_print_node_inner(node->children[0]);
            printf(")");
            break;
        case CFL_NODE_IF:
            printf("if (");
            cfl_print_node_inner(node->children[0]);
            printf(") then (");
            cfl_print_node_inner(node->children[1]);
            printf(") else (");
            cfl_print_node_inner(node->children[2]);
            printf(")");
            break;
        default:
            break;
    }
}

void cfl_print_node(cfl_node* node)
{
    cfl_print_node_inner(node);

    printf("\n");
}
