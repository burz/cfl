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
        case CFL_NODE_LET_REC:
            if(strcmp(target->children[0]->data, variable))
            {
                if(strcmp(target->children[1]->data, variable))
                {
                    if(!cfl_substitute(target->children[2], variable, value) ||
                       !cfl_substitute(target->children[3], variable, value))
                        return 0;
                }
                else if(!cfl_substitute(target->children[3], variable, value))
                    return 0;
            }
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
    if(node->type == CFL_NODE_LIST)
    {
        cfl_list_node* pos = node->data;

        while(pos)
        {
            if(!cfl_evaluate(pos->node))
                return 0;

            pos = pos->next;
        }
    }
    else if(node->type == CFL_NODE_AND)
    {
        if(!cfl_evaluate(node->children[0]))
            return 0;

        if(node->children[0]->type != CFL_NODE_BOOL)
        {
            fprintf(stderr, "ERROR: Encountered a type mismatch during evaluation\n");

            return 0;
        }

        if(!*((bool*) node->children[0]->data))
        {
            cfl_delete_node(node);

            cfl_create_node_bool(node, false);

            return 1;
        }

        if(!cfl_evaluate(node->children[1]))
            return 0;

        if(node->children[1]->type != CFL_NODE_BOOL)
        {
            fprintf(stderr, "ERROR: Encountered a type mismatch during evaluation\n");

            return 0;
        }

        cfl_delete_node(node);

        cfl_create_node_bool(node, *((bool*) node->children[1]->data));
    }
    else if(node->type == CFL_NODE_OR)
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
            cfl_delete_node(node);

            cfl_create_node_bool(node, true);

            return 1;
        }

        if(!cfl_evaluate(node->children[1]))
            return 0;

        if(node->children[1]->type != CFL_NODE_BOOL)
        {
            fprintf(stderr, "ERROR: Encountered a type mismatch during evaluation\n");

            return 0;
        }

        cfl_delete_node(node);

        cfl_create_node_bool(node, *((bool*) node->children[1]->data));
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
    else if(node->type == CFL_NODE_ADD)
    {
        if(!cfl_evaluate(node->children[0]) || !cfl_evaluate(node->children[1]))
            return 0;

        if(node->children[0]->type != CFL_NODE_INTEGER ||
           node->children[1]->type != CFL_NODE_INTEGER)
        {
            fprintf(stderr, "ERROR: Encountered a type mismatch during evaluation\n");

            return 0;
        }

        int result = *((int*) node->children[0]->data) +
                     *((int*) node->children[1]->data);

        cfl_delete_node(node);

        cfl_create_node_integer(node, result);
    }
    else if(node->type == CFL_NODE_MULTIPLY)
    {
        if(!cfl_evaluate(node->children[0]) || !cfl_evaluate(node->children[1]))
            return 0;

        if(node->children[0]->type != CFL_NODE_INTEGER ||
           node->children[1]->type != CFL_NODE_INTEGER)
        {
            fprintf(stderr, "ERROR: Encountered a type mismatch during evaluation\n");

            return 0;
        }

        int result = *((int*) node->children[0]->data) *
                     *((int*) node->children[1]->data);

        cfl_delete_node(node);

        cfl_create_node_integer(node, result);
    }
    else if(node->type == CFL_NODE_DIVIDE)
    {
        if(!cfl_evaluate(node->children[0]) || !cfl_evaluate(node->children[1]))
            return 0;

        if(node->children[0]->type != CFL_NODE_INTEGER ||
           node->children[1]->type != CFL_NODE_INTEGER)
        {
            fprintf(stderr, "ERROR: Encountered a type mismatch during evaluation\n");

            return 0;
        }

        if(*((int*) node->children[1]->data) == 0)
        {
            fprintf(stderr, "ERROR: Division by zero\n");

            return 0;
        }

        int result = *((int*) node->children[0]->data) /
                     *((int*) node->children[1]->data);

        cfl_delete_node(node);

        cfl_create_node_integer(node, result);
    }
    else if(node->type == CFL_NODE_EQUAL)
    {
        if(!cfl_evaluate(node->children[0]) || !cfl_evaluate(node->children[1]))
            return 0;

        if(node->children[0]->type != CFL_NODE_INTEGER ||
           node->children[1]->type != CFL_NODE_INTEGER)
        {
            fprintf(stderr, "ERROR: Encountered a type mismatch during evaluation\n");

            return 0;
        }

        bool result = *((int*) node->children[0]->data) ==
                      *((int*) node->children[1]->data);

        cfl_delete_node(node);

        cfl_create_node_bool(node, result);
    }
    else if(node->type == CFL_NODE_LESS)
    {
        if(!cfl_evaluate(node->children[0]) || !cfl_evaluate(node->children[1]))
            return 0;

        if(node->children[0]->type != CFL_NODE_INTEGER ||
           node->children[1]->type != CFL_NODE_INTEGER)
        {
            fprintf(stderr, "ERROR: Encountered a type mismatch during evaluation\n");

            return 0;
        }

        bool result = *((int*) node->children[0]->data) <
                      *((int*) node->children[1]->data);

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

        cfl_free_node(node->children[0]->children[0]);
        cfl_free_node(node->children[1]);

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

            cfl_node** temp = node->children;

            cfl_free_node(node->children[0]);
            cfl_free_node(node->children[2]);

            *node = *temp[1];

            free(temp[1]);
            free(temp);
        }
        else
        {
            if(!cfl_evaluate(node->children[2]))
                return 0;

            cfl_node** temp = node->children;

            cfl_free_node(node->children[0]);
            cfl_free_node(node->children[1]);

            *node = *temp[2];

            free(temp[2]);
            free(temp);
        }
    }
    else if(node->type == CFL_NODE_LET_REC)
    {
        cfl_node* temp0 = malloc(sizeof(cfl_node));

        if(!temp0)
            return 0;

        if(!cfl_copy_node(temp0, node->children[0]))
        {
            free(temp0);

            return 0;
        }

        cfl_node* temp1 = malloc(sizeof(cfl_node));

        if(!temp1)
        {
            cfl_free_node(temp0);

            return 0;
        }

        if(!cfl_copy_node(temp1, node->children[1]))
        {
            cfl_free_node(temp0);
            free(temp1);

            return 0;
        }

        cfl_node* temp2 = malloc(sizeof(cfl_node));

        if(!temp2)
        {
            cfl_free_node(temp0);
            cfl_free_node(temp1);

            return 0;
        }

        if(!cfl_create_node_application(temp2, temp0, temp1))
        {
            cfl_free_node(temp0);
            cfl_free_node(temp1);
            free(temp2);

            return 0;
        }

        temp0 = malloc(sizeof(cfl_node));

        if(!temp0)
        {
            cfl_free_node(temp2);

            return 0;
        }

        if(!cfl_copy_node(temp0, node->children[0]))
        {
            free(temp0);
            cfl_free_node(temp2);

            return 0;
        }

        temp1 = malloc(sizeof(cfl_node));

        if(!temp1)
        {
            cfl_free_node(temp0);
            cfl_free_node(temp2);

            return 0;
        }

        if(!cfl_copy_node(temp1, node->children[1]))
        {
            cfl_free_node(temp0);
            free(temp1);
            cfl_free_node(temp2);

            return 0;
        }

        cfl_node* temp3 = malloc(sizeof(cfl_node));

        if(!temp3)
        {
            cfl_free_node(temp0);
            cfl_free_node(temp1);
            cfl_free_node(temp2);

            return 0;
        }

        if(!cfl_copy_node(temp3, node->children[2]))
        {
            cfl_free_node(temp0);
            cfl_free_node(temp1);
            cfl_free_node(temp2);
            free(temp3);

            return 0;
        }

        cfl_node* temp4 = malloc(sizeof(cfl_node));

        if(!temp4)
        {
            cfl_free_node(temp0);
            cfl_free_node(temp1);
            cfl_free_node(temp2);
            cfl_free_node(temp3);

            return 0;
        }

        if(!cfl_create_node_let_rec(temp4, temp0, temp1, temp3, temp2))
        {
            cfl_free_node(temp0);
            cfl_free_node(temp1);
            cfl_free_node(temp2);
            cfl_free_node(temp3);
            free(temp4);

            return 0;
        }

        temp0 = malloc(sizeof(cfl_node));

        if(!temp0)
        {
            cfl_free_node(temp4);

            return 0;
        }

        if(!cfl_copy_node(temp0, node->children[1]))
        {
            free(temp0);
            cfl_free_node(temp4);

            return 0;
        }

        temp1 = malloc(sizeof(cfl_node));

        if(!temp1)
        {
            cfl_free_node(temp0);
            cfl_free_node(temp4);

            return 0;
        }

        if(!cfl_create_node_function(temp1, temp0, temp4))
        {
            cfl_free_node(temp0);
            free(temp1);
            cfl_free_node(temp4);

            return 0;
        }

        if(!cfl_substitute(node->children[2], node->children[0]->data, temp1))
        {
            cfl_free_node(temp1);

            return 0;
        }

        cfl_free_node(temp1);

        temp0 = malloc(sizeof(cfl_node));

        if(!temp0)
            return 0;

        if(!cfl_create_node_function(temp0, node->children[1], node->children[2]))
        {
            free(temp0);

            return 0;
        }

        if(!cfl_substitute(node->children[3], node->children[0]->data, temp0))
        {
            cfl_free_node(temp0);
            cfl_free_node(node->children[0]);
            cfl_free_node(node->children[3]);
            free(node->children);
            node->number_of_children = 0;

            return 0;
        }

        cfl_free_node(temp0);

        temp0 = node->children[3];

        cfl_free_node(node->children[0]);
        free(node->children);

        *node = *temp0;

        free(temp0);

        return cfl_evaluate(node);
    }
    else if(node->type == CFL_NODE_PUSH)
    {
        if(!cfl_evaluate(node->children[0]) || !cfl_evaluate(node->children[1]))
            return 0;

        if(node->children[1]->type != CFL_NODE_LIST)
        {
            fprintf(stderr, "ERROR: Encountered a type mismatch during evaluation\n");

            return 0;
        }

        cfl_node* list_node = node->children[1];

        if(!list_node->data)
        {
            list_node->data = malloc(sizeof(cfl_list_node));

            if(!list_node->data)
                return 0;

            ((cfl_list_node*) list_node->data)->next = 0;
        }
        else
        {
            cfl_list_node* new_node = malloc(sizeof(cfl_list_node));

            if(!new_node)
                return 0;

            new_node->next = list_node->data;

            list_node->data = new_node;
        }

        ((cfl_list_node*) list_node->data)->node = node->children[0];

        free(node->children);

        *node = *list_node;

        free(list_node);
    }
    else if(node->type == CFL_NODE_CONCATENATE)
    {
        if(!cfl_evaluate(node->children[0]) || !cfl_evaluate(node->children[1]))
            return 0;

        if(node->children[0]->type != CFL_NODE_LIST ||
           node->children[1]->type != CFL_NODE_LIST)
        {
            fprintf(stderr, "ERROR: Encountered a type mismatch during evaluation\n");

            return 0;
        }

        cfl_node* list_node = node->children[0];

        if(!list_node->data)
        {
            list_node->data = node->children[1]->data;
        }
        else
        {
            cfl_list_node* pos = list_node->data;

            while(pos->next != 0)
                pos = pos->next;

            pos->next = node->children[1]->data;
        }

        free(node->children[1]);
        free(node->children);

        *node = *list_node;

        free(list_node);
    }
    else if(node->type == CFL_NODE_CASE)
    {
        if(!cfl_evaluate(node->children[0]))
            return 0;

        if(node->children[0]->type != CFL_NODE_LIST)
        {
            fprintf(stderr, "ERROR: Encountered a type mismatch during evaluation\n");

            return 0;
        }

        if(node->children[0]->data)
        {
            cfl_node* head = ((cfl_list_node*) node->children[0]->data)->node;

            cfl_list_node* tail_list =
                ((cfl_list_node*) node->children[0]->data)->next;

            cfl_node* tail = malloc(sizeof(cfl_node));

            if(!tail)
                return 0;

            cfl_create_node_list(tail, tail_list);

            if(!cfl_substitute(node->children[4], node->children[2]->data, head) ||
               !cfl_substitute(node->children[4], node->children[3]->data, tail))
            {
                free(tail);

                return 0;
            }

            cfl_node* temp = node->children[4];

            free(tail);
            cfl_free_node(node->children[0]);
            cfl_free_node(node->children[1]);
            cfl_free_node(node->children[2]);
            cfl_free_node(node->children[3]);
            free(node->children);

            *node = *temp;

            free(temp);

            return cfl_evaluate(node);
        }
        else
        {
            if(!cfl_evaluate(node->children[1]))
                return 0;

            cfl_node* temp = node->children[1];

            cfl_free_node(node->children[0]);
            cfl_free_node(node->children[2]);
            cfl_free_node(node->children[3]);
            cfl_free_node(node->children[4]);
            free(node->children);

            *node = *temp;

            free(temp);
        }
    }

    return 1;
}
