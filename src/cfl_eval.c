#include "cfl_eval.h"

#include <stdio.h>
#include <stdlib.h>

int cfl_evaluate(cfl_node* node)
{
    if(node->type == CFL_NODE_AND)
    {
        cfl_evaluate(node->children[0]);
        cfl_evaluate(node->children[1]);

        if(node->children[0]->type != CFL_NODE_BOOL ||
           node->children[1]->type != CFL_NODE_BOOL)
        {
            fprintf(stderr, "ERROR: Encountered a type mismatch during evaluation\n");

            return 0;
        }

        bool result = *((bool*) node->children[0]->data) &&
                      *((bool*) node->children[1]->data);

        cfl_delete_node(node);

        cfl_create_node_bool(node, result);
    }
    else if(node->type == CFL_NODE_OR)
    {
        cfl_evaluate(node->children[0]);
        cfl_evaluate(node->children[1]);

        if(node->children[0]->type != CFL_NODE_BOOL ||
           node->children[1]->type != CFL_NODE_BOOL)
        {
            fprintf(stderr, "ERROR: Encountered a type mismatch during evaluation\n");

            return 0;
        }

        bool result = *((bool*) node->children[0]->data) ||
                      *((bool*) node->children[1]->data);

        cfl_delete_node(node);

        cfl_create_node_bool(node, result);
    }
    else if(node->type == CFL_NODE_NOT)
    {
        cfl_evaluate(node->children[0]);

        if(node->children[0]->type != CFL_NODE_BOOL)
        {
            fprintf(stderr, "ERROR: Encountered a type mismatch during evaluation\n");

            return 0;
        }

        bool result = !*((bool*) node->children[0]->data);

        cfl_delete_node(node);

        cfl_create_node_bool(node, result);
    }
    else if(node->type == CFL_NODE_IF)
    {
        cfl_evaluate(node->children[0]);

        if(node->children[0]->type != CFL_NODE_BOOL)
        {
            fprintf(stderr, "ERROR: Encountered a type mismatch during evaluation\n");

            return 0;
        }

        if(*((bool*) node->children[0]->data))
        {
            cfl_evaluate(node->children[1]);

            cfl_delete_node(node->children[0]);
            cfl_delete_node(node->children[2]);

            cfl_node** temp = node->children;

            free(node->children[0]);
            free(node->children[2]);

            *node = *temp[1];

            free(temp[1]);
            free(temp);
        }
        else
        {
            cfl_evaluate(node->children[2]);

            cfl_delete_node(node->children[0]);
            cfl_delete_node(node->children[1]);

            cfl_node** temp = node->children;

            free(node->children[0]);
            free(node->children[1]);

            *node = *temp[2];

            free(temp[2]);
            free(temp);
        }
    }

    return 1;
}
