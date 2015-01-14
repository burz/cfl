#include "cfl_eval.h"

#include <stdio.h>
#include <string.h>

void* cfl_eval_malloc(size_t size)
{
    void* result = malloc(size);

    if(!result)
    {
        fprintf(stderr, "MEMORY ERROR: Ran out of memory "
                        "during evaluation\n");
    }

    return result;
}

static bool cfl_complex_variable_contains(cfl_node* node, char* variable)
{
    if(node->type == CFL_NODE_VARIABLE)
        return !strcmp(node->data, variable);

    int i = 0;
    for( ; i < node->number_of_children; ++i)
        if(cfl_complex_variable_contains(node->children[i], variable))
            return true;

    return false;
}

bool cfl_substitute(cfl_node* target, char* variable, cfl_node* value)
{
    int i;

    switch(target->type)
    {
        case CFL_NODE_VARIABLE:
            if(!strcmp(target->data, variable))
            {
                cfl_node* result = cfl_copy_new_node(value);

                if(!result)
                    return false;

                cfl_delete_node(target);

                *target = *result;

                free(result);
            }
            break;
        case CFL_NODE_FUNCTION:
            if(!cfl_complex_variable_contains(target->children[0], variable))
                if(!cfl_substitute(target->children[1], variable, value))
                    return false;
            break;
        case CFL_NODE_LET_REC:
            if(strcmp(target->children[0]->data, variable))
            {
                if(!cfl_complex_variable_contains(target->children[1], variable))
                {
                    if(!cfl_substitute(target->children[2], variable, value) ||
                       !cfl_substitute(target->children[3], variable, value))
                        return false;
                }
                else if(!cfl_substitute(target->children[3], variable, value))
                    return false;
            }
            break;
        case CFL_NODE_CASE:
            if(!cfl_substitute(target->children[0], variable, value) ||
               !cfl_substitute(target->children[1], variable, value))
                return false;

            if(!cfl_complex_variable_contains(target->children[2], variable) &&
               strcmp(target->children[3]->data, variable))
                    if(!cfl_substitute(target->children[4], variable, value))
                        return false;
            break;
        default:
            for(i = 0; i < target->number_of_children; ++i)
                if(!cfl_substitute(target->children[i], variable, value))
                    return false;
            break;
    }

    return true;
}

bool cfl_complex_substitute(cfl_node* target, cfl_node* variable, cfl_node* value)
{
    if(variable->type == CFL_NODE_VARIABLE)
    {
        if(*((char*) variable->data) == '_')
            return true;

        return cfl_substitute(target, variable->data, value);
    }

    int i = 0;
    for( ; i < variable->number_of_children; ++i)
        if(!cfl_complex_substitute(target, variable->children[i], value->children[i]))
            return false;

    return true;
}

bool cfl_evaluate(cfl_node* node)
{
    if(node->type == CFL_NODE_LIST)
    {
        cfl_list_node* pos = node->data;

        while(pos)
        {
            if(!cfl_evaluate(pos->node))
                return false;

            pos = pos->next;
        }
    }
    else if(node->type == CFL_NODE_TUPLE)
    {
        int i = 0;
        for( ; i < node->number_of_children; ++i)
            if(!cfl_evaluate(node->children[i]))
                return false;
    }
    else if(node->type == CFL_NODE_AND)
    {
        if(!cfl_evaluate(node->children[0]))
            return false;

        if(!*((bool*) node->children[0]->data))
        {
            if(!cfl_reinitialize_node_bool(node, false))
                return false;

            return true;
        }

        if(!cfl_evaluate(node->children[1]))
            return false;

        bool result = *((bool*) node->children[1]->data);

        if(!cfl_reinitialize_node_bool(node, result))
            return false;
    }
    else if(node->type == CFL_NODE_OR)
    {
        if(!cfl_evaluate(node->children[0]))
            return false;

        if(*((bool*) node->children[0]->data))
        {
            if(!cfl_reinitialize_node_bool(node, true))
                return false;

            return true;
        }

        if(!cfl_evaluate(node->children[1]))
            return false;

        bool result = *((bool*) node->children[1]->data);

        if(!cfl_reinitialize_node_bool(node, result))
            return false;
    }
    else if(node->type == CFL_NODE_NOT)
    {
        if(!cfl_evaluate(node->children[0]))
            return false;

        bool result = !*((bool*) node->children[0]->data);

        if(!cfl_reinitialize_node_bool(node, result))
            return false;
    }
    else if(node->type == CFL_NODE_ADD)
    {
        if(!cfl_evaluate(node->children[0]) || !cfl_evaluate(node->children[1]))
            return false;

        int result = *((int*) node->children[0]->data) +
                     *((int*) node->children[1]->data);

        if(!cfl_reinitialize_node_integer(node, result))
            return false;
    }
    else if(node->type == CFL_NODE_MULTIPLY)
    {
        if(!cfl_evaluate(node->children[0]) || !cfl_evaluate(node->children[1]))
            return false;

        int result = *((int*) node->children[0]->data) *
                     *((int*) node->children[1]->data);

        if(!cfl_reinitialize_node_integer(node, result))
            return false;
    }
    else if(node->type == CFL_NODE_DIVIDE)
    {
        if(!cfl_evaluate(node->children[0]) || !cfl_evaluate(node->children[1]))
            return false;

        if(*((int*) node->children[1]->data) == 0)
        {
            fprintf(stderr, "EVALUATION ERROR: Division by zero\n");

            return false;
        }

        int result = *((int*) node->children[0]->data) /
                     *((int*) node->children[1]->data);

        if(!cfl_reinitialize_node_integer(node, result))
            return false;
    }
    else if(node->type == CFL_NODE_EQUAL)
    {
        if(!cfl_evaluate(node->children[0]) || !cfl_evaluate(node->children[1]))
            return false;

        bool result = *((int*) node->children[0]->data) ==
                      *((int*) node->children[1]->data);

        if(!cfl_reinitialize_node_bool(node, result))
            return false;
    }
    else if(node->type == CFL_NODE_LESS)
    {
        if(!cfl_evaluate(node->children[0]) || !cfl_evaluate(node->children[1]))
            return false;

        bool result = *((int*) node->children[0]->data) <
                      *((int*) node->children[1]->data);

        if(!cfl_reinitialize_node_bool(node, result))
            return false;
    }
    else if(node->type == CFL_NODE_APPLICATION)
    {
        if(!cfl_evaluate(node->children[0]) || !cfl_evaluate(node->children[1]))
            return false;

        if(!cfl_complex_substitute(node->children[0]->children[1],
                                   node->children[0]->children[0],
                                   node->children[1]))
            return false;

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
            return false;

        if(*((bool*) node->children[0]->data))
        {
            if(!cfl_evaluate(node->children[1]))
                return false;

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
                return false;

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
        cfl_node* temp0 = cfl_copy_new_node(node->children[0]);

        if(!temp0)
            return false;

        cfl_node* temp1 = cfl_copy_new_node(node->children[1]);

        if(!temp1)
        {
            cfl_free_node(temp0);

            return false;
        }

        cfl_node* temp2 = cfl_create_new_node_application(temp0, temp1);

        if(!temp2)
            return false;

        temp0 = cfl_copy_new_node(node->children[0]);

        if(!temp0)
        {
            cfl_free_node(temp2);

            return false;
        }

        temp1 = cfl_copy_new_node(node->children[1]);

        if(!temp1)
        {
            cfl_free_node(temp0);
            cfl_free_node(temp2);

            return false;
        }

        cfl_node* temp3 = cfl_copy_new_node(node->children[2]);

        if(!temp3)
        {
            cfl_free_node(temp0);
            cfl_free_node(temp1);
            cfl_free_node(temp2);

            return false;
        }

        cfl_node* temp4 = cfl_create_new_node_let_rec(temp0, temp1, temp3, temp2);

        if(!temp4)
            return false;

        temp0 = cfl_copy_new_node(node->children[1]);

        if(!temp0)
        {
            cfl_free_node(temp4);

            return false;
        }

        temp1 = cfl_create_new_node_function(temp0, temp4);

        if(!temp1)
            return false;

        if(!cfl_substitute(node->children[2], node->children[0]->data, temp1))
        {
            cfl_free_node(temp1);

            return false;
        }

        cfl_free_node(temp1);

        temp0 = cfl_create_new_node_function(node->children[1], node->children[2]);

        if(!temp0)
            return false;

        if(!cfl_substitute(node->children[3], node->children[0]->data, temp0))
        {
            cfl_free_node(temp0);
            cfl_free_node(node->children[0]);
            cfl_free_node(node->children[3]);
            free(node->children);
            node->number_of_children = 0;

            return false;
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
            return false;

        cfl_node* list_node = node->children[1];

        if(!list_node->data)
        {
            list_node->data = cfl_eval_malloc(sizeof(cfl_list_node));

            if(!list_node->data)
                return false;

            ((cfl_list_node*) list_node->data)->next = 0;
        }
        else
        {
            cfl_list_node* new_node = cfl_eval_malloc(sizeof(cfl_list_node));

            if(!new_node)
                return false;

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
            return false;

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
            return false;

        if(node->children[0]->data)
        {
            cfl_node* head = ((cfl_list_node*) node->children[0]->data)->node;

            cfl_list_node* tail_list =
                ((cfl_list_node*) node->children[0]->data)->next;

            cfl_node* tail = cfl_create_new_node_list(tail_list);

            if(!tail)
                return false;

            if(!cfl_complex_substitute(node->children[4], node->children[2], head) ||
               !cfl_substitute(node->children[4], node->children[3]->data, tail))
            {
                free(tail);

                return false;
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
                return false;

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

    return true;
}
