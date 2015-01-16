#include "cfl_type.h"

#include <stdio.h>
#include <string.h>

extern void* cfl_type_malloc(size_t size);
extern unsigned int cfl_type_get_next_id(void);

static void cfl_delete_type_equation_chain_until(
        cfl_type_equation_chain* equation_head,
        cfl_type_equation_chain* block)
{
    while(equation_head->next != block)
    {
        cfl_type_equation_chain* temp = equation_head->next;

        equation_head->next = temp->next;

        cfl_free_type(temp->left);
        cfl_free_type(temp->right);
        free(temp);
    }
}

static cfl_type* cfl_run_hidden_typecheck(
        cfl_type_equation_chain* equation_head,
        cfl_type_hypothesis_chain* hypothesis_head,
        char* name,
        cfl_node* node)
{
    cfl_type_equation_chain* position = equation_head->next;

    cfl_type* result = cfl_generate_type_equation_chain(equation_head,
                                                        hypothesis_head,
                                                        node);

    if(!result)
        return 0;

    if(!cfl_close_type_equation_chain(equation_head))
    {
        cfl_free_type(result);

        return 0;
    }

    if(!cfl_ensure_type_equation_chain_consistency(equation_head->next))
    {
        cfl_type_error_bad_definition(name);

        cfl_free_type(result);

        return 0;
    }

    cfl_type* final_result = cfl_substitute_type(equation_head, result);

    cfl_delete_type_equation_chain_until(equation_head, position);
    cfl_free_type(result);

    return final_result;
}

unsigned int cfl_generate_global_hypotheses(
        cfl_type_equation_chain* equation_head,
        cfl_type_hypothesis_chain* hypothesis_head)
{
    cfl_type_hypothesis_chain* hypothesis =
            cfl_type_malloc(sizeof(cfl_type_hypothesis_chain));

    if(!hypothesis)
        return 0;

    unsigned int id = cfl_type_get_next_id();

    hypothesis->name = "random";
    hypothesis->id = id;
    hypothesis->next = hypothesis_head->next;

    hypothesis_head->next = hypothesis;

    cfl_type* integer_left = cfl_type_malloc(sizeof(cfl_type));

    if(!integer_left)
        return 0;

    cfl_create_type_integer(integer_left);

    cfl_type* integer_right = cfl_type_malloc(sizeof(cfl_type));

    if(!integer_right)
    {
        cfl_free_type(integer_left);

        return 0;
    }

    cfl_create_type_integer(integer_right);

    cfl_type* arrow = cfl_type_malloc(sizeof(cfl_type));

    if(!arrow)
    {
        cfl_free_type(integer_left);
        cfl_free_type(integer_right);

        return 0;
    }

    cfl_create_type_arrow(arrow, integer_left, integer_right);

    cfl_type* variable = cfl_type_malloc(sizeof(cfl_type));

    if(!variable)
    {
        cfl_free_type(arrow);

        return 0;
    }

    cfl_create_type_variable(variable, id);

    if(!cfl_add_equation(equation_head, variable, arrow))
    {
        cfl_free_type(arrow);
        cfl_free_type(variable);

        return 0;
    }

    return 1;
}

unsigned int cfl_setup_definitions(
        cfl_type_equation_chain* equation_head,
        cfl_type_hypothesis_chain* hypothesis_head,
        cfl_definition_list* definitions)
{
    unsigned int hypothesis_count = cfl_generate_global_hypotheses(equation_head,
                                                                   hypothesis_head);

    if(!hypothesis_count)
        return 0;

    cfl_definition_list* pos = definitions;

    while(pos)
    {
        pos->type = cfl_run_hidden_typecheck(equation_head,
                                             hypothesis_head,
                                             pos->name->data,
                                             pos->definition);

        if(!pos->type)
        {
            cfl_remove_n_hypotheses(hypothesis_head, hypothesis_count);

            return 0;
        }

        cfl_type_hypothesis_chain* hypothesis =
                cfl_type_malloc(sizeof(cfl_type_hypothesis_chain));

        if(!hypothesis)
        {
            cfl_remove_n_hypotheses(hypothesis_head, hypothesis_count);

            return 0;
        }

        unsigned int id = cfl_type_get_next_id();

        hypothesis->name = pos->name->data;
        hypothesis->id = id;
        hypothesis->next = hypothesis_head->next;

        hypothesis_head->next = hypothesis;

        ++hypothesis_count;

        cfl_type* result = cfl_type_malloc(sizeof(cfl_type));

        if(!result)
        {
            cfl_remove_n_hypotheses(hypothesis_head, hypothesis_count);

            return 0;
        }

        if(!cfl_copy_type(result, pos->type))
        {
            cfl_remove_n_hypotheses(hypothesis_head, hypothesis_count);
            free(result);

            return 0;
        }

        cfl_type* variable = cfl_type_malloc(sizeof(cfl_type));

        if(!variable)
        {
            cfl_remove_n_hypotheses(hypothesis_head, hypothesis_count);
            free(result);

            return 0;
        }

        cfl_create_type_variable(variable, id);

        if(!cfl_add_equation(equation_head, result, variable))
        {
            cfl_free_type(result);
            cfl_free_type(variable);

            return 0;
        }

        pos = pos->next;
    }

    return hypothesis_count;
}
