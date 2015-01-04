#include "cfl_type.h"

#include <stdlib.h>
#include <string.h>

int cfl_add_equation(cfl_type_equation_chain* head, cfl_type* left, cfl_type* right)
{
    int found = 0;
    int found_reverse = 0;

    cfl_type_equation_chain* pos = head->next;

    while(pos)
    {
        if(!cfl_compare_type(left, pos->left) &&
           !cfl_compare_type(right, pos->right))
            found = 1;

        if(!cfl_compare_type(right, pos->left) &&
           !cfl_compare_type(left, pos->right))
            found_reverse = 1;

        if(found && found_reverse)
        {
            cfl_delete_type(left);
            free(left);
            cfl_delete_type(right);
            free(right);

            return -1;
        }

        pos = pos->next;
    }

    if(!found && !found_reverse)
    {
        cfl_type_equation_chain* chain_node = malloc(sizeof(cfl_type_equation_chain));

        if(!chain_node)
            return 0;

        chain_node->left = left;
        chain_node->right = right;
        chain_node->next = head->next;

        head->next = chain_node;

        chain_node = malloc(sizeof(cfl_type_equation_chain));

        if(!chain_node)
            return 0;

        chain_node->left = malloc(sizeof(cfl_type));

        if(!chain_node->left)
            return 0;

        if(!cfl_copy_type(chain_node->left, right))
        {
            free(chain_node->left);
            free(chain_node);

            return 0;
        }

        chain_node->right = malloc(sizeof(cfl_type));

        if(!chain_node->right)
        {
            cfl_delete_type(chain_node->left);
            free(chain_node->left);
            free(chain_node);

            return 0;
        }

        if(!cfl_copy_type(chain_node->right, left))
        {
            cfl_delete_type(chain_node->left);
            free(chain_node->left);
            free(chain_node->right);
            free(chain_node);

            return 0;
        }

        chain_node->next = head->next;

        head->next = chain_node;
    }
    else if(!found)
    {
        cfl_type_equation_chain* chain_node = malloc(sizeof(cfl_type_equation_chain));

        if(!chain_node)
            return 0;

        chain_node->left = left;
        chain_node->right = right;
        chain_node->next = head->next;

        head->next = chain_node;
    }
    else
    {
        cfl_type_equation_chain* chain_node = malloc(sizeof(cfl_type_equation_chain));

        if(!chain_node)
            return 0;

        chain_node->left = right;
        chain_node->right = left;
        chain_node->next = head->next;

        head->next = chain_node;
    }

    return 1;
}

int cfl_add_equation_from_copies(
        cfl_type_equation_chain* head,
        cfl_type* left,
        cfl_type* right)
{
    cfl_type* left_copy = malloc(sizeof(cfl_type));

    if(!left_copy)
        return 0;

    if(!cfl_copy_type(left_copy, left))
    {
        free(left_copy);

        return 0;
    }

    cfl_type* right_copy = malloc(sizeof(cfl_type));

    if(!right_copy)
    {
        cfl_delete_type(left_copy);
        free(left_copy);

        return 0;
    }

    if(!cfl_copy_type(right_copy, right))
    {
        cfl_delete_type(left_copy);
        free(left_copy);
        free(right_copy);

        return 0;
    }

    int result = cfl_add_equation(head, left_copy, right_copy);

    if(!result)
    {
        cfl_delete_type(left_copy);
        free(left_copy);
        cfl_delete_type(right_copy);
        free(right_copy);
    }

    return result;
}

unsigned int cfl_lookup_hypothesis(cfl_type_hypothesis_chain* chain, char* name)
{
    unsigned int result = 0;

    while(chain)
    {
        if(!strcmp(name, chain->name))
        {
            result = chain->id;

            break;
        }

        chain = chain->next;
    }

    return result;
}

cfl_type* cfl_generate_type_equation_chain(
        cfl_type_equation_chain* equation_head,
        cfl_type_hypothesis_chain* hypothesis_head,
        cfl_node* node)
{
    static unsigned int next_id = 1;

    cfl_type* result = 0;

    unsigned int id0;
    unsigned int id1;
    cfl_type_hypothesis_chain* hypothesis_chain_node;
    cfl_type* child_type0;
    cfl_type* child_type1;
    cfl_type* child_type2;
    cfl_type* temp_type0;
    cfl_type* temp_type1;
    cfl_type* temp_type2;

    switch(node->type)
    {
        case CFL_NODE_VARIABLE:
            id0 = cfl_lookup_hypothesis(hypothesis_head->next, node->data);

            if(!id0)
                break;

            result = malloc(sizeof(cfl_type));

            if(!result)
                break;

            cfl_create_type_variable(result, id0);

            break;
        case CFL_NODE_BOOL:
            result = malloc(sizeof(cfl_type));

            if(!result)
                break;

            cfl_create_type_bool(result);

            break;
        case CFL_NODE_INTEGER:
            result = malloc(sizeof(cfl_type));

            if(!result)
                break;

            cfl_create_type_integer(result);

            break;
        case CFL_NODE_FUNCTION:
            id0 = next_id++;

            child_type0 = malloc(sizeof(cfl_type));

            if(!child_type0)
                break;

            cfl_create_type_variable(child_type0, id0);

            hypothesis_chain_node = malloc(sizeof(cfl_type_hypothesis_chain));

            if(!hypothesis_chain_node)
            {
                cfl_delete_type(child_type0);
                free(child_type0);

                break;
            }

            hypothesis_chain_node->name = node->children[0]->data;
            hypothesis_chain_node->id = id0;
            hypothesis_chain_node->next = hypothesis_head->next;

            hypothesis_head->next = hypothesis_chain_node;

            child_type1 = cfl_generate_type_equation_chain(equation_head,
                                                           hypothesis_head,
                                                           node->children[1]);

            hypothesis_head->next = hypothesis_chain_node->next;

            free(hypothesis_chain_node);

            if(!child_type1)
            {
                cfl_delete_type(child_type0);
                free(child_type0);

                break;
            }

            result = malloc(sizeof(cfl_type));

            if(!result)
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(child_type1);
                free(child_type1);

                break;
            }

            cfl_create_type_arrow(result, child_type0, child_type1);

            break;
        case CFL_NODE_LIST:
            temp_type0 = malloc(sizeof(cfl_type));

            if(!temp_type0)
                break;

            id0 = next_id++;

            cfl_create_type_variable(temp_type0, id0);

            cfl_list_node* pos = node->data;

            while(pos)
            {
                temp_type1 = malloc(sizeof(cfl_type));

                if(!temp_type1)
                {
                    cfl_delete_type(temp_type0);
                    free(temp_type0);

                    return 0;
                }

                if(!cfl_copy_type(temp_type1, temp_type0))
                {
                    cfl_delete_type(temp_type0);
                    free(temp_type0);
                    free(temp_type1);

                    return 0;
                }

                child_type0 = cfl_generate_type_equation_chain(equation_head,
                                                               hypothesis_head,
                                                               pos->node);

                if(!child_type0)
                {
                    cfl_delete_type(temp_type0);
                    free(temp_type0);
                    cfl_delete_type(temp_type1);
                    free(temp_type1);

                    return 0;
                }

                if(!cfl_add_equation(equation_head, child_type0, temp_type1))
                {
                    cfl_delete_type(temp_type0);
                    free(temp_type0);
                    cfl_delete_type(temp_type1);
                    free(temp_type1);
                    cfl_delete_type(child_type0);
                    free(child_type0);

                    return 0;
                }

                pos = pos->next;
            }

            result = malloc(sizeof(cfl_type));

            if(!result)
            {
                cfl_delete_type(temp_type0);
                free(temp_type0);

                return 0;
            }

            cfl_create_type_list(result, temp_type0);

            break;
        case CFL_NODE_AND:
        case CFL_NODE_OR:
            child_type0 = cfl_generate_type_equation_chain(equation_head,
                                                           hypothesis_head,
                                                           node->children[0]);

            if(!child_type0)
                break;

            child_type1 = cfl_generate_type_equation_chain(equation_head,
                                                           hypothesis_head,
                                                           node->children[1]);

            if(!child_type1)
            {
                cfl_delete_type(child_type0);
                free(child_type0);

                break;
            }

            temp_type0 = malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(child_type1);
                free(child_type1);

                break;
            }

            cfl_create_type_bool(temp_type0);

            if(!cfl_add_equation(equation_head, child_type0, temp_type0))
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(child_type1);
                free(child_type1);
                cfl_delete_type(temp_type0);
                free(temp_type0);

                break;
            }

            temp_type0 = malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_delete_type(child_type1);
                free(child_type1);

                break;
            }

            cfl_create_type_bool(temp_type0);

            if(!cfl_add_equation(equation_head, child_type1, temp_type0))
            {
                cfl_delete_type(child_type1);
                free(child_type1);
                cfl_delete_type(temp_type0);
                free(temp_type0);

                break;
            }

            result = malloc(sizeof(cfl_type));

            if(!result)
                break;

            cfl_create_type_bool(result);

            break;
        case CFL_NODE_NOT:
            child_type0 = cfl_generate_type_equation_chain(equation_head,
                                                           hypothesis_head,
                                                           node->children[0]);

            if(!child_type0)
                break;

            temp_type0 = malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_delete_type(child_type0);
                free(child_type0);

                break;
            }

            cfl_create_type_bool(temp_type0);

            if(!cfl_add_equation(equation_head, child_type0, temp_type0))
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(temp_type0);
                free(temp_type0);

                break;
            }

            result = malloc(sizeof(cfl_type));

            if(!result)
                break;

            cfl_create_type_bool(result);

            break;
        case CFL_NODE_ADD:
        case CFL_NODE_MULTIPLY:
        case CFL_NODE_DIVIDE:
            child_type0 = cfl_generate_type_equation_chain(equation_head,
                                                           hypothesis_head,
                                                           node->children[0]);

            if(!child_type0)
                break;

            child_type1 = cfl_generate_type_equation_chain(equation_head,
                                                           hypothesis_head,
                                                           node->children[1]);

            if(!child_type1)
            {
                cfl_delete_type(child_type0);
                free(child_type0);

                break;
            }

            temp_type0 = malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(child_type1);
                free(child_type1);

                break;
            }

            cfl_create_type_integer(temp_type0);

            if(!cfl_add_equation(equation_head, child_type0, temp_type0))
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(child_type1);
                free(child_type1);
                cfl_delete_type(temp_type0);
                free(temp_type0);

                break;
            }

            temp_type0 = malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_delete_type(child_type1);
                free(child_type1);

                break;
            }

            cfl_create_type_integer(temp_type0);

            if(!cfl_add_equation(equation_head, child_type1, temp_type0))
            {
                cfl_delete_type(child_type1);
                free(child_type1);
                cfl_delete_type(temp_type0);
                free(temp_type0);

                break;
            }

            result = malloc(sizeof(cfl_type));

            if(!result)
                break;

            cfl_create_type_integer(result);

            break;
        case CFL_NODE_EQUAL:
        case CFL_NODE_LESS:
            child_type0 = cfl_generate_type_equation_chain(equation_head,
                                                           hypothesis_head,
                                                           node->children[0]);

            if(!child_type0)
                break;

            child_type1 = cfl_generate_type_equation_chain(equation_head,
                                                           hypothesis_head,
                                                           node->children[1]);

            if(!child_type1)
            {
                cfl_delete_type(child_type0);
                free(child_type0);

                break;
            }

            temp_type0 = malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(child_type1);
                free(child_type1);

                break;
            }

            cfl_create_type_integer(temp_type0);

            if(!cfl_add_equation(equation_head, child_type0, temp_type0))
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(child_type1);
                free(child_type1);
                cfl_delete_type(temp_type0);
                free(temp_type0);

                break;
            }

            temp_type0 = malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_delete_type(child_type1);
                free(child_type1);

                break;
            }

            cfl_create_type_integer(temp_type0);

            if(!cfl_add_equation(equation_head, child_type1, temp_type0))
            {
                cfl_delete_type(child_type1);
                free(child_type1);
                cfl_delete_type(temp_type0);
                free(temp_type0);

                break;
            }

            result = malloc(sizeof(cfl_type));

            if(!result)
                break;

            cfl_create_type_bool(result);

            break;
        case CFL_NODE_APPLICATION:
            child_type0 = cfl_generate_type_equation_chain(equation_head,
                                                           hypothesis_head,
                                                           node->children[0]);

            if(!child_type0)
                break;

            child_type1 = cfl_generate_type_equation_chain(equation_head,
                                                           hypothesis_head,
                                                           node->children[1]);

            if(!child_type1)
            {
                cfl_delete_type(child_type0);
                free(child_type0);

                break;
            }

            temp_type0 = malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(child_type1);
                free(child_type1);

                break;
            }

            id0 = next_id++;

            cfl_create_type_variable(temp_type0, id0);

            temp_type1 = malloc(sizeof(cfl_type));

            if(!temp_type1)
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(child_type1);
                free(child_type1);
                cfl_delete_type(temp_type0);
                free(temp_type0);

                break;
            }

            cfl_create_type_arrow(temp_type1, child_type1, temp_type0);

            if(!cfl_add_equation(equation_head, child_type0, temp_type1))
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(temp_type1);
                free(temp_type1);

                break;
            }

            result = malloc(sizeof(cfl_type));

            if(!result)
                break;

            cfl_create_type_variable(result, id0);

            break;
        case CFL_NODE_IF:
            child_type0 = cfl_generate_type_equation_chain(equation_head,
                                                           hypothesis_head,
                                                           node->children[0]);

            if(!child_type0)
                break;

            child_type1 = cfl_generate_type_equation_chain(equation_head,
                                                           hypothesis_head,
                                                           node->children[1]);

            if(!child_type1)
            {
                cfl_delete_type(child_type0);
                free(child_type0);

                break;
            }

            child_type2 = cfl_generate_type_equation_chain(equation_head,
                                                           hypothesis_head,
                                                           node->children[2]);

            if(!child_type2)
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(child_type1);
                free(child_type1);

                break;
            }

            temp_type0 = malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(child_type1);
                free(child_type1);
                cfl_delete_type(child_type2);
                free(child_type2);

                break;
            }

            if(!cfl_copy_type(temp_type0, child_type2))
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(child_type1);
                free(child_type1);
                cfl_delete_type(child_type2);
                free(child_type2);
                free(temp_type0);

                break;
            }

            if(!cfl_add_equation(equation_head, child_type1, child_type2))
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(child_type1);
                free(child_type1);
                cfl_delete_type(child_type2);
                free(child_type2);
                cfl_delete_type(temp_type0);
                free(temp_type0);

                break;
            }

            temp_type1 = malloc(sizeof(cfl_type));

            if(!temp_type1)
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(temp_type0);
                free(temp_type0);

                break;
            }

            cfl_create_type_bool(temp_type1);

            if(!cfl_add_equation(equation_head, child_type0, temp_type1))
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(temp_type0);
                free(temp_type0);
                cfl_delete_type(temp_type1);
                free(temp_type1);

                break;
            }

            temp_type1 = malloc(sizeof(cfl_type));

            if(!temp_type1)
            {
                cfl_delete_type(temp_type0);
                free(temp_type0);

                break;
            }

            id0 = next_id++;

            cfl_create_type_variable(temp_type1, id0);

            if(!cfl_add_equation(equation_head, temp_type0, temp_type1))
            {
                cfl_delete_type(temp_type0);
                free(temp_type0);
                cfl_delete_type(temp_type1);
                free(temp_type1);

                return 0;
            }

            result = malloc(sizeof(cfl_type));

            if(!result)
                break;

            cfl_create_type_variable(result, id0);

            break;
        case CFL_NODE_LET_REC:
            temp_type0 = malloc(sizeof(cfl_type));

            if(!temp_type0)
                break;

            id0 = next_id++;

            cfl_create_type_variable(temp_type0, id0);

            temp_type1 = malloc(sizeof(cfl_type));

            if(!temp_type1)
            {
                cfl_delete_type(temp_type0);
                free(temp_type0);

                break;
            }

            id1 = next_id++;

            cfl_create_type_variable(temp_type1, id1);

            hypothesis_chain_node = malloc(sizeof(cfl_type_hypothesis_chain));

            if(!hypothesis_chain_node)
            {
                cfl_delete_type(temp_type0);
                free(temp_type0);
                cfl_delete_type(temp_type1);
                free(temp_type1);

                break;
            }

            hypothesis_chain_node->name = node->children[0]->data;
            hypothesis_chain_node->id = id0;
            hypothesis_chain_node->next = hypothesis_head->next;

            hypothesis_head->next = hypothesis_chain_node;

            result = cfl_generate_type_equation_chain(equation_head,
                                                      hypothesis_head,
                                                      node->children[3]);

            hypothesis_chain_node = malloc(sizeof(cfl_type_hypothesis_chain));

            if(!hypothesis_chain_node)
            {
                cfl_delete_type(temp_type0);
                free(temp_type0);
                cfl_delete_type(temp_type1);
                free(temp_type1);

                hypothesis_chain_node = hypothesis_head->next;

                hypothesis_head->next = hypothesis_chain_node->next;

                free(hypothesis_chain_node);

                return 0;
            }

            hypothesis_chain_node->name = node->children[1]->data;
            hypothesis_chain_node->id = id1;
            hypothesis_chain_node->next = hypothesis_head->next;

            hypothesis_head->next = hypothesis_chain_node;

            child_type0 = cfl_generate_type_equation_chain(equation_head,
                                                           hypothesis_head,
                                                           node->children[2]);

            hypothesis_head->next = hypothesis_chain_node->next;

            free(hypothesis_chain_node);

            hypothesis_chain_node = hypothesis_head->next;

            hypothesis_head->next = hypothesis_chain_node->next;

            free(hypothesis_chain_node);

            if(!result)
            {
                cfl_delete_type(temp_type0);
                free(temp_type0);
                cfl_delete_type(temp_type1);
                free(temp_type1);

                if(child_type0)
                {
                    cfl_delete_type(child_type0);
                    free(child_type0);
                }

                break;
            }

            if(!child_type0)
            {
                cfl_delete_type(temp_type0);
                free(temp_type0);
                cfl_delete_type(temp_type1);
                free(temp_type1);
                cfl_delete_type(result);
                free(result);

                return 0;
            }

            temp_type2 = malloc(sizeof(cfl_type));

            if(!temp_type2)
            {
                cfl_delete_type(temp_type0);
                free(temp_type0);
                cfl_delete_type(temp_type1);
                free(temp_type1);
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(result);
                free(result);

                return 0;
            }

            cfl_create_type_arrow(temp_type2, temp_type1, child_type0);

            if(!cfl_add_equation(equation_head, temp_type0, temp_type2))
            {
                cfl_delete_type(temp_type0);
                free(temp_type0);
                cfl_delete_type(temp_type2);
                free(temp_type2);
                cfl_delete_type(result);
                free(result);

                return 0;
            }

            break;
        case CFL_NODE_PUSH:
            child_type0 = cfl_generate_type_equation_chain(equation_head,
                                                           hypothesis_head,
                                                           node->children[0]);

            if(!child_type0)
                break;

            child_type1 = cfl_generate_type_equation_chain(equation_head,
                                                           hypothesis_head,
                                                           node->children[1]);

            if(!child_type1)
            {
                cfl_delete_type(child_type0);
                free(child_type0);

                break;
            }

            temp_type0 = malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(child_type1);
                free(child_type1);

                break;
            }

            id0 = next_id++;

            cfl_create_type_variable(temp_type0, id0);

            if(!cfl_add_equation(equation_head, child_type0, temp_type0))
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(child_type1);
                free(child_type1);
                cfl_delete_type(temp_type0);
                free(temp_type0);

                break;
            }

            temp_type0 = malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_delete_type(child_type1);
                free(child_type1);

                break;
            }

            cfl_create_type_variable(temp_type0, id0);

            temp_type1 = malloc(sizeof(cfl_type));

            if(!temp_type1)
            {
                cfl_delete_type(child_type1);
                free(child_type1);
                cfl_delete_type(temp_type0);
                free(temp_type0);

                break;
            }

            cfl_create_type_list(temp_type1, temp_type0);

            result = malloc(sizeof(cfl_type));

            if(!result)
            {
                cfl_delete_type(child_type1);
                free(child_type1);
                cfl_delete_type(temp_type1);
                free(temp_type1);

                break;
            }

            if(!cfl_copy_type(result, temp_type1))
            {
                cfl_delete_type(child_type1);
                free(child_type1);
                cfl_delete_type(temp_type1);
                free(temp_type1);
                free(result);

                break;
            }

            if(!cfl_add_equation(equation_head, child_type1, temp_type1))
            {
                cfl_delete_type(child_type1);
                free(child_type1);
                cfl_delete_type(temp_type1);
                free(temp_type1);
                cfl_delete_type(result);
                free(result);

                return 0;
            }

            break;
        case CFL_NODE_CONCATENATE:
            child_type0 = cfl_generate_type_equation_chain(equation_head,
                                                           hypothesis_head,
                                                           node->children[0]);

            if(!child_type0)
                break;

            child_type1 = cfl_generate_type_equation_chain(equation_head,
                                                           hypothesis_head,
                                                           node->children[1]);

            if(!child_type1)
            {
                cfl_delete_type(child_type0);
                free(child_type0);

                break;
            }

            temp_type0 = malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(child_type1);
                free(child_type1);

                break;
            }

            id0 = next_id++;

            cfl_create_type_variable(temp_type0, id0);

            temp_type1 = malloc(sizeof(cfl_type));

            if(!temp_type1)
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(child_type1);
                free(child_type1);
                cfl_delete_type(temp_type0);
                free(temp_type0);

                break;
            }

            cfl_create_type_list(temp_type1, temp_type0);

            temp_type0 = malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(child_type1);
                free(child_type1);
                cfl_delete_type(temp_type1);
                free(temp_type1);

                break;
            }

            if(!cfl_copy_type(temp_type0, temp_type1))
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(child_type1);
                free(child_type1);
                free(temp_type0);
                cfl_delete_type(temp_type1);
                free(temp_type1);

                break;
            }

            if(!cfl_add_equation(equation_head, child_type0, temp_type1))
            {
                cfl_delete_type(child_type0);
                free(child_type0);
                cfl_delete_type(child_type1);
                free(child_type1);
                cfl_delete_type(temp_type0);
                free(temp_type0);
                cfl_delete_type(temp_type1);
                free(temp_type1);

                break;
            }

            result = malloc(sizeof(cfl_type));

            if(!result)
            {
                cfl_delete_type(child_type1);
                free(child_type1);
                cfl_delete_type(temp_type0);
                free(temp_type0);

                break;
            }

            if(!cfl_copy_type(result, temp_type0))
            {
                free(result);
                cfl_delete_type(child_type1);
                free(child_type1);
                cfl_delete_type(temp_type0);
                free(temp_type0);

                return 0;
            }

            if(!cfl_add_equation(equation_head, child_type1, temp_type0))
            {
                cfl_delete_type(result);
                free(result);
                cfl_delete_type(child_type1);
                free(child_type1);
                cfl_delete_type(temp_type0);
                free(temp_type0);

                return 0;
            }

            break;
        default:
            break;
    }

    return result;
}

int cfl_close_type_equation_chain(cfl_type_equation_chain* head)
{
    int changes = 1;

    while(changes)
    {
        changes = 0;

        cfl_type_equation_chain* focus = head->next;

        while(focus)
        {
            if(focus->left->type == CFL_TYPE_ARROW &&
               focus->right->type == CFL_TYPE_ARROW)
            {
                int result = cfl_add_equation_from_copies(head,
                                                          focus->left->input,
                                                          focus->right->input);

                if(!result)
                    return 0;
                else if(result > 0)
                    changes = 1;

                result = cfl_add_equation_from_copies(head,
                                                      focus->left->output,
                                                      focus->right->output);

                if(!result)
                    return 0;
                else if(result > 0)
                    changes = 1;
            }
            else if(focus->left->type == CFL_TYPE_LIST &&
                    focus->right->type == CFL_TYPE_LIST)
            {
                int result = cfl_add_equation_from_copies(head,
                                                          focus->left->input,
                                                          focus->right->input);

                if(!result)
                    return 0;
                else if(result > 0)
                    changes = 1;
            }

            cfl_type_equation_chain* pos = head->next;

            while(pos)
            {
                if(!cfl_compare_type(focus->right, pos->left))
                {
                    int result = cfl_add_equation_from_copies(head,
                                                              focus->left,
                                                              pos->right);

                    if(!result)
                        return 0;
                    else if(result > 0)
                        changes = 1;
                }

                pos = pos->next;
            }

            focus = focus->next;
        }
    }

    return 1;
}

int cfl_ensure_type_equation_chain_consistency(cfl_type_equation_chain* chain)
{
    for( ; chain; chain = chain->next)
        if((chain->left->type == CFL_TYPE_BOOL &&
            (chain->right->type == CFL_TYPE_INTEGER ||
             chain->right->type == CFL_TYPE_LIST ||
             chain->right->type == CFL_TYPE_ARROW)) ||
           (chain->left->type == CFL_TYPE_INTEGER &&
            (chain->right->type == CFL_TYPE_BOOL ||
             chain->right->type == CFL_TYPE_LIST ||
             chain->right->type == CFL_TYPE_ARROW)) ||
           (chain->left->type == CFL_TYPE_LIST &&
            (chain->right->type == CFL_TYPE_BOOL ||
             chain->right->type == CFL_TYPE_INTEGER ||
             chain->right->type == CFL_TYPE_ARROW)) ||
           (chain->left->type == CFL_TYPE_ARROW &&
            (chain->right->type == CFL_TYPE_BOOL ||
             chain->right->type == CFL_TYPE_INTEGER ||
             chain->right->type == CFL_TYPE_LIST)))
            return 0;

    return 1;
}

cfl_type* cfl_substitute_type(cfl_type_equation_chain* head, cfl_type* node)
{
    if(node->type == CFL_TYPE_BOOL)
    {
        cfl_type* result = malloc(sizeof(cfl_type));

        if(!result)
            return 0;

        cfl_create_type_bool(result);

        return result;
    }
    else if(node->type == CFL_TYPE_INTEGER)
    {
        cfl_type* result = malloc(sizeof(cfl_type));

        if(!result)
            return 0;

        cfl_create_type_integer(result);

        return result;
    }
    else if(node->type == CFL_TYPE_LIST)
    {
        cfl_type* new_content = cfl_substitute_type(head, node->input);

        if(!new_content)
            return 0;

        cfl_type* result = malloc(sizeof(cfl_type));

        if(!result)
        {
            cfl_delete_type(new_content);
            free(new_content);

            return 0;
        }

        cfl_create_type_list(result, new_content);

        return result;
    }
    else if(node->type == CFL_TYPE_ARROW)
    {
        cfl_type* new_input = cfl_substitute_type(head, node->input);

        if(!new_input)
            return 0;

        cfl_type* new_output = cfl_substitute_type(head, node->output);

        if(!new_output)
        {
            cfl_delete_type(new_input);
            free(new_input);

            return 0;
        }

        cfl_type* result = malloc(sizeof(cfl_type));

        if(!result)
        {
            cfl_delete_type(new_input);
            free(new_input);
            cfl_delete_type(new_output);
            free(new_output);

            return 0;
        }

        cfl_create_type_arrow(result, new_input, new_output);

        return result;
    }
    else
    {
        cfl_type_equation_chain* pos = head->next;
        cfl_type* focus = node;

        while(pos)
        {
            if(!cfl_compare_type(node, pos->left))
            {
                if(pos->right->type == CFL_TYPE_BOOL ||
                   pos->right->type == CFL_TYPE_INTEGER)
                {
                    focus = pos->right;

                    break;
                }
                else if(pos->right->type == CFL_TYPE_LIST)
                    return cfl_substitute_type(head, pos->right);
                else if(pos->right->type == CFL_TYPE_ARROW)
                    return cfl_substitute_type(head, pos->right);
                else if(focus->type == CFL_TYPE_VARIABLE &&
                        pos->right->type == CFL_TYPE_VARIABLE &&
                        focus->id > pos->right->id)
                    focus = pos->right;
            }

            pos = pos->next;
        }

        cfl_type* result = malloc(sizeof(cfl_type));

        if(!result)
            return 0;

        if(!cfl_copy_type(result, focus))
        {
            free(result);

            return 0;
        }

        return result;
    }
}

void cfl_delete_type_equation_chain(cfl_type_equation_chain* chain)
{
    while(chain)
    {
        cfl_type_equation_chain* temp = chain;

        chain = chain->next;

        cfl_delete_type(temp->left);
        free(temp->left);
        cfl_delete_type(temp->right);
        free(temp->right);
        free(temp);
    }
}

cfl_type* cfl_typecheck(cfl_node* node)
{
    cfl_type_equation_chain chain;
    chain.next = 0;

    cfl_type_hypothesis_chain hypotheses;
    hypotheses.next = 0;

    cfl_type* result = cfl_generate_type_equation_chain(&chain,
                                                        &hypotheses,
                                                        node);

    if(!result)
    {
        cfl_delete_type_equation_chain(chain.next);

        return 0;
    }

    if(!cfl_close_type_equation_chain(&chain))
    {
        cfl_delete_type_equation_chain(chain.next);
        cfl_delete_type(result);
        free(result);

        return 0;
    }

    if(!cfl_ensure_type_equation_chain_consistency(chain.next))
    {
        cfl_delete_type_equation_chain(chain.next);
        cfl_delete_type(result);
        free(result);

        return 0;
    }

    cfl_type* final_result = cfl_substitute_type(&chain, result);

    cfl_delete_type(result);
    free(result);

    cfl_delete_type_equation_chain(chain.next);

    return final_result;
}
