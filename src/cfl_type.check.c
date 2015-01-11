#include "cfl_type.h"

#include <stdio.h>
#include <string.h>

extern int cfl_type_error;
extern void* cfl_type_malloc(size_t size);

static void cfl_type_error_failure(void)
{
    fprintf(stderr, "TYPE ERROR: Could not type expression\n");
}

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
            cfl_free_type(left);
            cfl_free_type(right);

            return -1;
        }

        pos = pos->next;
    }

    if(!found && !found_reverse)
    {
        cfl_type_equation_chain* chain_node =
            cfl_type_malloc(sizeof(cfl_type_equation_chain));

        if(!chain_node)
            return 0;

        chain_node->left = left;
        chain_node->right = right;
        chain_node->next = head->next;

        head->next = chain_node;

        chain_node = cfl_type_malloc(sizeof(cfl_type_equation_chain));

        if(!chain_node)
            return 0;

        chain_node->left = cfl_type_malloc(sizeof(cfl_type));

        if(!chain_node->left)
            return 0;

        if(!cfl_copy_type(chain_node->left, right))
        {
            free(chain_node->left);
            free(chain_node);

            return 0;
        }

        chain_node->right = cfl_type_malloc(sizeof(cfl_type));

        if(!chain_node->right)
        {
            cfl_free_type(chain_node->left);
            free(chain_node);

            return 0;
        }

        if(!cfl_copy_type(chain_node->right, left))
        {
            cfl_free_type(chain_node->left);
            free(chain_node->right);
            free(chain_node);

            return 0;
        }

        chain_node->next = head->next;

        head->next = chain_node;
    }
    else if(!found)
    {
        cfl_type_equation_chain* chain_node =
            cfl_type_malloc(sizeof(cfl_type_equation_chain));

        if(!chain_node)
            return 0;

        chain_node->left = left;
        chain_node->right = right;
        chain_node->next = head->next;

        head->next = chain_node;
    }
    else
    {
        cfl_type_equation_chain* chain_node =
            cfl_type_malloc(sizeof(cfl_type_equation_chain));

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
    cfl_type* left_copy = cfl_type_malloc(sizeof(cfl_type));

    if(!left_copy)
        return 0;

    if(!cfl_copy_type(left_copy, left))
    {
        free(left_copy);

        return 0;
    }

    cfl_type* right_copy = cfl_type_malloc(sizeof(cfl_type));

    if(!right_copy)
    {
        cfl_free_type(left_copy);

        return 0;
    }

    if(!cfl_copy_type(right_copy, right))
    {
        cfl_free_type(left_copy);
        free(right_copy);

        return 0;
    }

    int result = cfl_add_equation(head, left_copy, right_copy);

    if(!result)
    {
        cfl_free_type(left_copy);
        cfl_free_type(right_copy);
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

    int i;
    int j;
    unsigned int id0;
    unsigned int id1;
    unsigned int id2;
    cfl_type_hypothesis_chain* hypothesis_chain_node;
    cfl_type* child_type0;
    cfl_type* child_type1;
    cfl_type* child_type2;
    cfl_type* temp_type0;
    cfl_type* temp_type1;
    cfl_type* temp_type2;
    cfl_type** children;

    switch(node->type)
    {
        case CFL_NODE_VARIABLE:
            id0 = cfl_lookup_hypothesis(hypothesis_head->next, node->data);

            if(!id0)
            {
                fprintf(stderr, "TYPE ERROR: The variable \"%s\" "
                        "must be defined before it is used\n",
                        (char*) node->data);

                break;
            }

            result = cfl_type_malloc(sizeof(cfl_type));

            if(!result)
                break;

            cfl_create_type_variable(result, id0);

            break;
        case CFL_NODE_BOOL:
            result = cfl_type_malloc(sizeof(cfl_type));

            if(!result)
                break;

            cfl_create_type_bool(result);

            break;
        case CFL_NODE_INTEGER:
            result = cfl_type_malloc(sizeof(cfl_type));

            if(!result)
                break;

            cfl_create_type_integer(result);

            break;
        case CFL_NODE_LIST:
            temp_type0 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type0)
                break;

            id0 = next_id++;

            cfl_create_type_variable(temp_type0, id0);

            cfl_list_node* pos = node->data;

            while(pos)
            {
                temp_type1 = cfl_type_malloc(sizeof(cfl_type));

                if(!temp_type1)
                {
                    cfl_free_type(temp_type0);

                    return 0;
                }

                if(!cfl_copy_type(temp_type1, temp_type0))
                {
                    cfl_free_type(temp_type0);
                    free(temp_type1);

                    return 0;
                }

                child_type0 = cfl_generate_type_equation_chain(equation_head,
                                                               hypothesis_head,
                                                               pos->node);

                if(!child_type0)
                {
                    cfl_free_type(temp_type0);
                    cfl_free_type(temp_type1);

                    return 0;
                }

                if(!cfl_add_equation(equation_head, child_type0, temp_type1))
                {
                    cfl_free_type(temp_type0);
                    cfl_free_type(temp_type1);
                    cfl_free_type(child_type0);

                    return 0;
                }

                pos = pos->next;
            }

            result = cfl_type_malloc(sizeof(cfl_type));

            if(!result)
            {
                cfl_free_type(temp_type0);

                return 0;
            }

            cfl_create_type_list(result, temp_type0);

            break;
        case CFL_NODE_TUPLE:
            children = 0;

            if(node->number_of_children)
            {
                children = cfl_type_malloc(sizeof(cfl_type*) * node->number_of_children);

                for(i = 0; i < node->number_of_children; ++i)
                {
                    id0 = next_id++;

                    hypothesis_chain_node =
                        cfl_type_malloc(sizeof(cfl_type_hypothesis_chain));

                    if(!hypothesis_chain_node)
                    {
                        for(j = 0; j < i; ++j)
                            cfl_free_type(children[j]);

                        free(children);

                        break;
                    }

                    hypothesis_chain_node->name = node->children[0]->data;
                    hypothesis_chain_node->id = id0;
                    hypothesis_chain_node->next = hypothesis_head->next;

                    hypothesis_head->next = hypothesis_chain_node;

                    child_type0 = cfl_generate_type_equation_chain(equation_head,
                                                                   hypothesis_head,
                                                                   node->children[i]);

                    hypothesis_head->next = hypothesis_chain_node->next;

                    free(hypothesis_chain_node);

                    temp_type0 = cfl_type_malloc(sizeof(cfl_type));

                    if(!temp_type0)
                    {
                        cfl_free_type(child_type0);

                        for(j = 0; j < i; ++j)
                            cfl_free_type(children[j]);

                        free(children);

                        break;
                    }

                    cfl_create_type_variable(temp_type0, id0);

                    if(!cfl_add_equation(equation_head, child_type0, temp_type0))
                    {
                        cfl_free_type(child_type0);
                        free(temp_type0);

                        for(j = 0; j < i; ++j)
                            cfl_free_type(children[j]);

                        free(children);

                        break;
                    }

                    children[i] = cfl_type_malloc(sizeof(cfl_type));

                    if(!children[i])
                    {
                        for(j = 0; j < i; ++j)
                            cfl_free_type(children[j]);

                        free(children);

                        break;
                    }

                    cfl_create_type_variable(children[i], id0);
                }
            }

            result = cfl_type_malloc(sizeof(cfl_type));

            if(!result)
            {
                for(i = 0; i < node->number_of_children; ++i)
                    cfl_free_type(children[i]);

                free(children);

                break;
            }

            cfl_create_type_tuple(result, node->number_of_children, children);

            break;
        case CFL_NODE_FUNCTION:
            id0 = next_id++;

            child_type0 = cfl_type_malloc(sizeof(cfl_type));

            if(!child_type0)
                break;

            cfl_create_type_variable(child_type0, id0);

            hypothesis_chain_node =
                cfl_type_malloc(sizeof(cfl_type_hypothesis_chain));

            if(!hypothesis_chain_node)
            {
                cfl_free_type(child_type0);

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
                cfl_free_type(child_type0);

                break;
            }

            result = cfl_type_malloc(sizeof(cfl_type));

            if(!result)
            {
                cfl_free_type(child_type0);
                cfl_free_type(child_type1);

                break;
            }

            cfl_create_type_arrow(result, child_type0, child_type1);

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
                cfl_free_type(child_type0);

                break;
            }

            temp_type0 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_free_type(child_type0);
                cfl_free_type(child_type1);

                break;
            }

            cfl_create_type_bool(temp_type0);

            if(!cfl_add_equation(equation_head, child_type0, temp_type0))
            {
                cfl_free_type(child_type0);
                cfl_free_type(child_type1);
                cfl_free_type(temp_type0);

                break;
            }

            temp_type0 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_free_type(child_type1);

                break;
            }

            cfl_create_type_bool(temp_type0);

            if(!cfl_add_equation(equation_head, child_type1, temp_type0))
            {
                cfl_free_type(child_type1);
                cfl_free_type(temp_type0);

                break;
            }

            result = cfl_type_malloc(sizeof(cfl_type));

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

            temp_type0 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_free_type(child_type0);

                break;
            }

            cfl_create_type_bool(temp_type0);

            if(!cfl_add_equation(equation_head, child_type0, temp_type0))
            {
                cfl_free_type(child_type0);
                cfl_free_type(temp_type0);

                break;
            }

            result = cfl_type_malloc(sizeof(cfl_type));

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
                cfl_free_type(child_type0);

                break;
            }

            temp_type0 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_free_type(child_type0);
                cfl_free_type(child_type1);

                break;
            }

            cfl_create_type_integer(temp_type0);

            if(!cfl_add_equation(equation_head, child_type0, temp_type0))
            {
                cfl_free_type(child_type0);
                cfl_free_type(child_type1);
                cfl_free_type(temp_type0);

                break;
            }

            temp_type0 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_free_type(child_type1);

                break;
            }

            cfl_create_type_integer(temp_type0);

            if(!cfl_add_equation(equation_head, child_type1, temp_type0))
            {
                cfl_free_type(child_type1);
                cfl_free_type(temp_type0);

                break;
            }

            result = cfl_type_malloc(sizeof(cfl_type));

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
                cfl_free_type(child_type0);

                break;
            }

            temp_type0 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_free_type(child_type0);
                cfl_free_type(child_type1);

                break;
            }

            cfl_create_type_integer(temp_type0);

            if(!cfl_add_equation(equation_head, child_type0, temp_type0))
            {
                cfl_free_type(child_type0);
                cfl_free_type(child_type1);
                cfl_free_type(temp_type0);

                break;
            }

            temp_type0 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_free_type(child_type1);

                break;
            }

            cfl_create_type_integer(temp_type0);

            if(!cfl_add_equation(equation_head, child_type1, temp_type0))
            {
                cfl_free_type(child_type1);
                cfl_free_type(temp_type0);

                break;
            }

            result = cfl_type_malloc(sizeof(cfl_type));

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
                cfl_free_type(child_type0);

                break;
            }

            temp_type0 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_free_type(child_type0);
                cfl_free_type(child_type1);

                break;
            }

            id0 = next_id++;

            cfl_create_type_variable(temp_type0, id0);

            temp_type1 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type1)
            {
                cfl_free_type(child_type0);
                cfl_free_type(child_type1);
                cfl_free_type(temp_type0);

                break;
            }

            cfl_create_type_arrow(temp_type1, child_type1, temp_type0);

            if(!cfl_add_equation(equation_head, child_type0, temp_type1))
            {
                cfl_free_type(child_type0);
                cfl_free_type(temp_type1);

                break;
            }

            result = cfl_type_malloc(sizeof(cfl_type));

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
                cfl_free_type(child_type0);

                break;
            }

            child_type2 = cfl_generate_type_equation_chain(equation_head,
                                                           hypothesis_head,
                                                           node->children[2]);

            if(!child_type2)
            {
                cfl_free_type(child_type0);
                cfl_free_type(child_type1);

                break;
            }

            temp_type0 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_free_type(child_type0);
                cfl_free_type(child_type1);
                cfl_free_type(child_type2);

                break;
            }

            if(!cfl_copy_type(temp_type0, child_type2))
            {
                cfl_free_type(child_type0);
                cfl_free_type(child_type1);
                cfl_free_type(child_type2);
                free(temp_type0);

                break;
            }

            if(!cfl_add_equation(equation_head, child_type1, child_type2))
            {
                cfl_free_type(child_type0);
                cfl_free_type(child_type1);
                cfl_free_type(child_type2);
                cfl_free_type(temp_type0);

                break;
            }

            temp_type1 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type1)
            {
                cfl_free_type(child_type0);
                cfl_free_type(temp_type0);

                break;
            }

            cfl_create_type_bool(temp_type1);

            if(!cfl_add_equation(equation_head, child_type0, temp_type1))
            {
                cfl_free_type(child_type0);
                cfl_free_type(temp_type0);
                cfl_free_type(temp_type1);

                break;
            }

            temp_type1 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type1)
            {
                cfl_free_type(temp_type0);

                break;
            }

            id0 = next_id++;

            cfl_create_type_variable(temp_type1, id0);

            if(!cfl_add_equation(equation_head, temp_type0, temp_type1))
            {
                cfl_free_type(temp_type0);
                cfl_free_type(temp_type1);

                return 0;
            }

            result = cfl_type_malloc(sizeof(cfl_type));

            if(!result)
                break;

            cfl_create_type_variable(result, id0);

            break;
        case CFL_NODE_LET_REC:
            temp_type0 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type0)
                break;

            id0 = next_id++;

            cfl_create_type_variable(temp_type0, id0);

            temp_type1 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type1)
            {
                cfl_free_type(temp_type0);

                break;
            }

            id1 = next_id++;

            cfl_create_type_variable(temp_type1, id1);

            hypothesis_chain_node =
                cfl_type_malloc(sizeof(cfl_type_hypothesis_chain));

            if(!hypothesis_chain_node)
            {
                cfl_free_type(temp_type0);
                cfl_free_type(temp_type1);

                break;
            }

            hypothesis_chain_node->name = node->children[0]->data;
            hypothesis_chain_node->id = id0;
            hypothesis_chain_node->next = hypothesis_head->next;

            hypothesis_head->next = hypothesis_chain_node;

            result = cfl_generate_type_equation_chain(equation_head,
                                                      hypothesis_head,
                                                      node->children[3]);

            hypothesis_chain_node =
                cfl_type_malloc(sizeof(cfl_type_hypothesis_chain));

            if(!hypothesis_chain_node)
            {
                cfl_free_type(temp_type0);
                cfl_free_type(temp_type1);

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
                cfl_free_type(temp_type0);
                cfl_free_type(temp_type1);

                if(child_type0)
                    cfl_free_type(child_type0);

                break;
            }

            if(!child_type0)
            {
                cfl_free_type(temp_type0);
                cfl_free_type(temp_type1);
                cfl_free_type(result);

                return 0;
            }

            temp_type2 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type2)
            {
                cfl_free_type(temp_type0);
                cfl_free_type(temp_type1);
                cfl_free_type(child_type0);
                cfl_free_type(result);

                return 0;
            }

            cfl_create_type_arrow(temp_type2, temp_type1, child_type0);

            if(!cfl_add_equation(equation_head, temp_type0, temp_type2))
            {
                cfl_free_type(temp_type0);
                cfl_free_type(temp_type2);
                cfl_free_type(result);

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
                cfl_free_type(child_type0);

                break;
            }

            temp_type0 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_free_type(child_type0);
                cfl_free_type(child_type1);

                break;
            }

            id0 = next_id++;

            cfl_create_type_variable(temp_type0, id0);

            if(!cfl_add_equation(equation_head, child_type0, temp_type0))
            {
                cfl_free_type(child_type0);
                cfl_free_type(child_type1);
                cfl_free_type(temp_type0);

                break;
            }

            temp_type0 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_free_type(child_type1);

                break;
            }

            cfl_create_type_variable(temp_type0, id0);

            temp_type1 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type1)
            {
                cfl_free_type(child_type1);
                cfl_free_type(temp_type0);

                break;
            }

            cfl_create_type_list(temp_type1, temp_type0);

            result = cfl_type_malloc(sizeof(cfl_type));

            if(!result)
            {
                cfl_free_type(child_type1);
                cfl_free_type(temp_type1);

                break;
            }

            if(!cfl_copy_type(result, temp_type1))
            {
                cfl_free_type(child_type1);
                cfl_free_type(temp_type1);
                free(result);

                break;
            }

            if(!cfl_add_equation(equation_head, child_type1, temp_type1))
            {
                cfl_free_type(child_type1);
                cfl_free_type(temp_type1);
                cfl_free_type(result);

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
                cfl_free_type(child_type0);

                break;
            }

            temp_type0 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_free_type(child_type0);
                cfl_free_type(child_type1);

                break;
            }

            id0 = next_id++;

            cfl_create_type_variable(temp_type0, id0);

            temp_type1 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type1)
            {
                cfl_free_type(child_type0);
                cfl_free_type(child_type1);
                cfl_free_type(temp_type0);

                break;
            }

            cfl_create_type_list(temp_type1, temp_type0);

            temp_type0 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_free_type(child_type0);
                cfl_free_type(child_type1);
                cfl_free_type(temp_type1);

                break;
            }

            if(!cfl_copy_type(temp_type0, temp_type1))
            {
                cfl_free_type(child_type0);
                cfl_free_type(child_type1);
                free(temp_type0);
                cfl_free_type(temp_type1);

                break;
            }

            if(!cfl_add_equation(equation_head, child_type0, temp_type1))
            {
                cfl_free_type(child_type0);
                cfl_free_type(child_type1);
                cfl_free_type(temp_type0);
                cfl_free_type(temp_type1);

                break;
            }

            result = cfl_type_malloc(sizeof(cfl_type));

            if(!result)
            {
                cfl_free_type(child_type1);
                cfl_free_type(temp_type0);

                break;
            }

            if(!cfl_copy_type(result, temp_type0))
            {
                free(result);
                cfl_free_type(child_type1);
                cfl_free_type(temp_type0);

                return 0;
            }

            if(!cfl_add_equation(equation_head, child_type1, temp_type0))
            {
                cfl_free_type(result);
                cfl_free_type(child_type1);
                cfl_free_type(temp_type0);

                return 0;
            }

            break;
        case CFL_NODE_CASE:
            child_type0 = cfl_generate_type_equation_chain(equation_head,
                                                           hypothesis_head,
                                                           node->children[0]);

            if(!child_type0)
                break;

            temp_type0 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_free_type(child_type0);

                break;
            }

            id0 = next_id++;

            cfl_create_type_variable(temp_type0, id0);

            if(!cfl_add_equation(equation_head, child_type0, temp_type0))
            {
                cfl_free_type(child_type0);
                cfl_free_type(temp_type0);

                return 0;
            }

            child_type0 = cfl_generate_type_equation_chain(equation_head,
                                                           hypothesis_head,
                                                           node->children[1]);

            if(!child_type0)
                break;

            temp_type0 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_free_type(child_type0);

                break;
            }

            id1 = next_id++;

            cfl_create_type_variable(temp_type0, id1);

            if(!cfl_add_equation(equation_head, child_type0, temp_type0))
            {
                cfl_free_type(child_type0);
                cfl_free_type(temp_type0);

                return 0;
            }

            id2 = next_id++;

            hypothesis_chain_node =
                cfl_type_malloc(sizeof(cfl_type_hypothesis_chain));

            if(!hypothesis_chain_node)
                break;

            hypothesis_chain_node->name = node->children[2]->data;
            hypothesis_chain_node->id = id2;
            hypothesis_chain_node->next = hypothesis_head->next;

            hypothesis_head->next = hypothesis_chain_node;

            hypothesis_chain_node =
                cfl_type_malloc(sizeof(cfl_type_hypothesis_chain));

            if(!hypothesis_chain_node)
            {
                hypothesis_chain_node = hypothesis_head->next;

                hypothesis_head->next = hypothesis_chain_node->next;

                free(hypothesis_chain_node);

                break;
            }

            hypothesis_chain_node->name = node->children[3]->data;
            hypothesis_chain_node->id = id0;
            hypothesis_chain_node->next = hypothesis_head->next;

            hypothesis_head->next = hypothesis_chain_node;

            child_type0 = cfl_generate_type_equation_chain(equation_head,
                                                           hypothesis_head,
                                                           node->children[4]);

            hypothesis_head->next = hypothesis_chain_node->next;

            free(hypothesis_chain_node);

            hypothesis_chain_node = hypothesis_head->next;

            hypothesis_head->next = hypothesis_chain_node->next;

            free(hypothesis_chain_node);

            if(!child_type0)
                break;

            temp_type0 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_free_type(child_type0);

                break;
            }

            cfl_create_type_variable(temp_type0, id1);

            if(!cfl_add_equation(equation_head, child_type0, temp_type0))
            {
                cfl_free_type(child_type0);
                cfl_free_type(temp_type0);

                break;
            }

            temp_type0 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type0)
                break;

            cfl_create_type_variable(temp_type0, id2);

            temp_type1 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type1)
            {
                cfl_free_type(temp_type0);

                break;
            }

            cfl_create_type_list(temp_type1, temp_type0);

            temp_type0 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type0)
            {
                cfl_free_type(temp_type1);

                break;
            }

            cfl_create_type_variable(temp_type0, id0);

            if(!cfl_add_equation(equation_head, temp_type1, temp_type0))
            {
                cfl_free_type(temp_type0);
                cfl_free_type(temp_type1);

                break;
            }

            result = cfl_type_malloc(sizeof(cfl_type));

            if(!result)
                break;

            cfl_create_type_variable(result, id1);

            break;
        default:
            break;
    }

    return result;
}

int cfl_close_type_equation_chain(cfl_type_equation_chain* head)
{
    int changes = 1;
    cfl_type_equation_chain* last_head = 0;

    while(changes)
    {
        changes = 0;

        cfl_type_equation_chain* this_head = head->next;
        cfl_type_equation_chain* focus = head->next;

        while(focus && (!last_head || focus != last_head))
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
            else if(focus->left->type == CFL_TYPE_TUPLE &&
                    focus->right->type == CFL_TYPE_TUPLE)
            {
                if(focus->left->id != focus->right->id)
                {
                    fprintf(stderr, "TYPE ERROR: The sizes of the tuples "
                                    "do not match.");

                    return 0;
                }

                int i = 0;
                for( ; i < focus->left->id; ++i)
                {
                    int result = !cfl_add_equation_from_copies(head,
                        ((cfl_type**) focus->left->input)[i],
                        ((cfl_type**) focus->right->input)[i]);

                    if(!result)
                        return 0;
                    else if(result > 0)
                        changes = 1;
                }
            }

            cfl_type_equation_chain* pos = focus->next;

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

        last_head = this_head;
    }

    return 1;
}

int cfl_ensure_type_equation_chain_consistency(cfl_type_equation_chain* chain)
{
    for( ; chain; chain = chain->next)
        if((chain->left->type == CFL_TYPE_BOOL &&
            (chain->right->type == CFL_TYPE_INTEGER ||
             chain->right->type == CFL_TYPE_LIST ||
             chain->right->type == CFL_TYPE_TUPLE ||
             chain->right->type == CFL_TYPE_ARROW)) ||
           (chain->left->type == CFL_TYPE_INTEGER &&
            (chain->right->type == CFL_TYPE_BOOL ||
             chain->right->type == CFL_TYPE_LIST ||
             chain->right->type == CFL_TYPE_TUPLE ||
             chain->right->type == CFL_TYPE_ARROW)) ||
           (chain->left->type == CFL_TYPE_LIST &&
            (chain->right->type == CFL_TYPE_BOOL ||
             chain->right->type == CFL_TYPE_INTEGER ||
             chain->right->type == CFL_TYPE_TUPLE ||
             chain->right->type == CFL_TYPE_ARROW)) ||
           (chain->left->type == CFL_TYPE_TUPLE &&
            (chain->right->type == CFL_TYPE_BOOL ||
             chain->right->type == CFL_TYPE_INTEGER ||
             chain->right->type == CFL_TYPE_LIST ||
             chain->right->type == CFL_TYPE_ARROW)) ||
           (chain->left->type == CFL_TYPE_ARROW &&
            (chain->right->type == CFL_TYPE_BOOL ||
             chain->right->type == CFL_TYPE_INTEGER ||
             chain->right->type == CFL_TYPE_LIST ||
             chain->right->type == CFL_TYPE_TUPLE)))
            return 0;

    return 1;
}

cfl_type* cfl_substitute_type(cfl_type_equation_chain* head, cfl_type* node)
{
    if(node->type == CFL_TYPE_BOOL)
    {
        cfl_type* result = cfl_type_malloc(sizeof(cfl_type));

        if(!result)
            return 0;

        cfl_create_type_bool(result);

        return result;
    }
    else if(node->type == CFL_TYPE_INTEGER)
    {
        cfl_type* result = cfl_type_malloc(sizeof(cfl_type));

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

        cfl_type* result = cfl_type_malloc(sizeof(cfl_type));

        if(!result)
        {
            cfl_free_type(new_content);

            return 0;
        }

        cfl_create_type_list(result, new_content);

        return result;
    }
    else if(node->type == CFL_TYPE_TUPLE)
    {
        cfl_type** new_content = 0;

        if(node->id)
        {
            new_content = cfl_type_malloc(sizeof(cfl_type*) * node->id);

            if(!new_content)
                return 0;

            int i = 0;
            for( ; i < node->id; ++i)
            {
                new_content[i] = cfl_substitute_type(head, ((cfl_type**) node->input)[i]);

                if(!new_content[i])
                {
                    int j = 0;
                    for( ; j < i; ++i)
                        cfl_free_type(new_content[j]);

                    free(new_content);
                }
            }
        }

        cfl_type* result = cfl_type_malloc(sizeof(cfl_type));

        if(!result)
        {
            if(new_content)
            {
                int i = 0;
                for( ; i < node->id; ++i)
                    cfl_free_type(new_content[i]);

                free(new_content);
            }

            return 0;
        }

        cfl_create_type_tuple(result, node->id, new_content);

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
            cfl_free_type(new_input);

            return 0;
        }

        cfl_type* result = cfl_type_malloc(sizeof(cfl_type));

        if(!result)
        {
            cfl_free_type(new_input);
            cfl_free_type(new_output);

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

        cfl_type* result = cfl_type_malloc(sizeof(cfl_type));

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

        cfl_free_type(temp->left);
        cfl_free_type(temp->right);
        free(temp);
    }
}

cfl_type* cfl_typecheck(cfl_node* node)
{
    cfl_type_error = 0;

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
        cfl_free_type(result);

        return 0;
    }

    if(!cfl_ensure_type_equation_chain_consistency(chain.next))
    {
        if(!cfl_type_error)
            cfl_type_error_failure();

        cfl_delete_type_equation_chain(chain.next);
        cfl_free_type(result);

        return 0;
    }

    cfl_type* final_result = cfl_substitute_type(&chain, result);

    cfl_free_type(result);
    cfl_delete_type_equation_chain(chain.next);

    return final_result;
}
