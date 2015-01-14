#include "cfl_type.h"

#include <stdio.h>
#include <string.h>

extern void* cfl_type_malloc(size_t size);
extern void cfl_reset_type_generator(void);

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
                    int result = cfl_add_equation_from_copies(head,
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
             chain->right->type == CFL_TYPE_CHAR ||
             chain->right->type == CFL_TYPE_LIST ||
             chain->right->type == CFL_TYPE_TUPLE ||
             chain->right->type == CFL_TYPE_ARROW)) ||
           (chain->left->type == CFL_TYPE_INTEGER &&
            (chain->right->type == CFL_TYPE_BOOL ||
             chain->right->type == CFL_TYPE_CHAR ||
             chain->right->type == CFL_TYPE_LIST ||
             chain->right->type == CFL_TYPE_TUPLE ||
             chain->right->type == CFL_TYPE_ARROW)) ||
           (chain->left->type == CFL_TYPE_CHAR &&
            (chain->right->type == CFL_TYPE_BOOL ||
             chain->right->type == CFL_TYPE_INTEGER ||
             chain->right->type == CFL_TYPE_LIST ||
             chain->right->type == CFL_TYPE_TUPLE ||
             chain->right->type == CFL_TYPE_ARROW)) ||
           (chain->left->type == CFL_TYPE_LIST &&
            (chain->right->type == CFL_TYPE_BOOL ||
             chain->right->type == CFL_TYPE_INTEGER ||
             chain->right->type == CFL_TYPE_CHAR ||
             chain->right->type == CFL_TYPE_TUPLE ||
             chain->right->type == CFL_TYPE_ARROW)) ||
           (chain->left->type == CFL_TYPE_TUPLE &&
            (chain->right->type == CFL_TYPE_BOOL ||
             chain->right->type == CFL_TYPE_INTEGER ||
             chain->right->type == CFL_TYPE_CHAR ||
             chain->right->type == CFL_TYPE_LIST ||
             chain->right->type == CFL_TYPE_ARROW)) ||
           (chain->left->type == CFL_TYPE_ARROW &&
            (chain->right->type == CFL_TYPE_BOOL ||
             chain->right->type == CFL_TYPE_INTEGER ||
             chain->right->type == CFL_TYPE_CHAR ||
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
    else if(node->type == CFL_TYPE_CHAR)
    {
        cfl_type* result = cfl_type_malloc(sizeof(cfl_type));

        if(!result)
            return 0;

        cfl_create_type_char(result);

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
                   pos->right->type == CFL_TYPE_INTEGER ||
                   pos->right->type == CFL_TYPE_CHAR)
                {
                    focus = pos->right;

                    break;
                }
                else if(pos->right->type == CFL_TYPE_LIST)
                    return cfl_substitute_type(head, pos->right);
                else if(pos->right->type == CFL_TYPE_TUPLE)
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
    cfl_reset_type_error_flag();
    cfl_reset_type_generator();

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
        if(!cfl_get_type_error_flag())
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
