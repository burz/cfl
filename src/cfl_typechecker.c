#include "cfl_typechecker.h"

#include <stdlib.h>
#include <string.h>

int cfl_create_type_variable(cfl_type* node, unsigned int id)
{
    node->type = CFL_TYPE_VARIABLE;
    node->id = id;
    node->input = 0;
    node->output = 0;

    return 1;
}

int cfl_create_type_bool(cfl_type* node)
{
    node->type = CFL_TYPE_BOOL;
    node->id = 0;
    node->input = 0;
    node->output = 0;

    return 1;
}

int cfl_create_type_arrow(cfl_type* node, cfl_type* input, cfl_type* output)
{
    node->type = CFL_TYPE_ARROW;
    node->id = 0;
    node->input = input;
    node->output = output;

    return 1;
}

int cfl_compare_type(cfl_type* left, cfl_type* right)
{
    if(left->type != right->type)
        return 1;

    if(left->type == CFL_TYPE_VARIABLE)
    {
        if(left->id != right->id)
            return 1;
    }
    else if(left->type == CFL_TYPE_ARROW)
        if(cfl_compare_type(left->input, right->input) ||
           cfl_compare_type(left->output, right->output))
            return 1;

    return 0;
}

int cfl_copy_type(cfl_type* target, cfl_type* node)
{
    cfl_type* input;
    cfl_type* output;

    switch(node->type)
    {
        case CFL_TYPE_VARIABLE:
            return cfl_create_type_variable(target, node->id);
            break;
        case CFL_TYPE_BOOL:
            return cfl_create_type_bool(target);
            break;
        case CFL_TYPE_ARROW:
            input = malloc(sizeof(cfl_type));

            if(!input)
                return 0;

            output = malloc(sizeof(cfl_type));

            if(!output)
            {
                free(input);

                return 0;
            }

            if(!cfl_copy_type(input, node->input))
            {
                free(input);
                free(output);

                return 0;
            }

            if(!cfl_copy_type(output, node->output))
            {
                cfl_delete_type(input);
                free(input);
                free(output);

                return 0;
            }

            if(!cfl_create_type_arrow(target, input, output))
            {
                cfl_delete_type(input);
                free(input);
                cfl_delete_type(output);
                free(output);

                return 0;
            }

            break;
        default:
            break;
    }

    return 1;
}

void cfl_delete_type(cfl_type* node)
{
    if(node->input)
    {
        cfl_delete_type(node->input);
        free(node->input);
        cfl_delete_type(node->output);
        free(node->output);
    }
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
            return -1;

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

cfl_type* cfl_generate_type_equation_chain(
        cfl_type_equation_chain* equation_head,
        cfl_type_hypothesis_chain* hypothesis_head,
        cfl_node* node)
{
    return 0;
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
            chain->right->type == CFL_TYPE_ARROW) ||
           (chain->left->type == CFL_TYPE_ARROW &&
            chain->right->type == CFL_TYPE_BOOL))
            return 0;

    return 1;
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

    cfl_delete_type_equation_chain(chain.next);

    return result;
}
