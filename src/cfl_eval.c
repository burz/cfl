#include "cfl_eval.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cfl_substitute(cfl_node* target, char* variable, cfl_node* value)
{
    int i;

    switch(target->type)
    {
        case CFL_NODE_VARIABLE:
            if(!strcmp(target->data, variable))
            {
                cfl_delete_node(target);

                if(!cfl_copy_node(target, value))
                    return 0;
            }
            break;
        case CFL_NODE_FUNCTION:
            if(strcmp(target->children[0]->data, variable))
                if(!cfl_substitute(target->children[1], variable, value))
                    return 0;
            break;
        default:
            for(i = 0; i < target->number_of_children; ++i)
                if(!cfl_substitute(target->children[i], variable, value))
                    return 0;
            break;
    }

    return 1;
}

int cfl_evaluate(cfl_node* node)
{
    if(node->type == CFL_NODE_AND)
    {
        if(!cfl_evaluate(node->children[0]) || !cfl_evaluate(node->children[1]))
            return 0;

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
        if(!cfl_evaluate(node->children[0]) || !cfl_evaluate(node->children[1]))
            return 0;

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
        if(!cfl_evaluate(node->children[0]))
            return 0;

        if(node->children[0]->type != CFL_NODE_BOOL)
        {
            fprintf(stderr, "ERROR: Encountered a type mismatch during evaluation\n");

            return 0;
        }

        bool result = !*((bool*) node->children[0]->data);

        cfl_delete_node(node);

        cfl_create_node_bool(node, result);
    }
    else if(node->type == CFL_NODE_APPLICATION)
    {
        if(!cfl_evaluate(node->children[0]) || !cfl_evaluate(node->children[1]))
            return 0;

        if(node->children[0]->type != CFL_NODE_FUNCTION)
        {
            fprintf(stderr, "ERROR: Encountered a type mismatch during evaluation\n");

            return 0;
        }

        if(!cfl_substitute(node->children[0]->children[1],
                           node->children[0]->children[0]->data,
                           node->children[1]))
            return 0;

        cfl_delete_node(node->children[0]->children[0]);
        free(node->children[0]->children[0]);
        cfl_delete_node(node->children[1]);
        free(node->children[1]);

        cfl_node* result = node->children[0]->children[1];

        free(node->children[0]->children);
        free(node->children[0]);
        free(node->children);

        *node = *result;

        free(result);

        return cfl_evaluate(node);
    }
    else if(node->type == CFL_NODE_IF)
    {
        if(!cfl_evaluate(node->children[0]))
            return 0;

        if(node->children[0]->type != CFL_NODE_BOOL)
        {
            fprintf(stderr, "ERROR: Encountered a type mismatch during evaluation\n");

            return 0;
        }

        if(*((bool*) node->children[0]->data))
        {
            if(!cfl_evaluate(node->children[1]))
                return 0;

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
            if(!cfl_evaluate(node->children[2]))
                return 0;

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
