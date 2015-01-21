#include "cfl_type.hypothesis.h"
#include "cfl_type.h"

#include <string.h>

extern void* cfl_type_malloc(size_t size);
extern unsigned int cfl_type_get_next_id(void);

bool cfl_push_hypothesis(
        cfl_type_hypothesis_chain* hypothesis_head,
        char* name,
        unsigned int id)
{
    cfl_type_hypothesis_chain* new_hypothesis =
        cfl_type_malloc(sizeof(cfl_type_hypothesis_chain));

    if(!new_hypothesis)
        return false;

    new_hypothesis->name = name;
    new_hypothesis->id = id;
    new_hypothesis->next = hypothesis_head->next;

    hypothesis_head->next = new_hypothesis;

    return true;
}

void cfl_pop_hypothesis(cfl_type_hypothesis_chain* hypothesis_head)
{
    cfl_type_hypothesis_chain* temp = hypothesis_head->next;

    hypothesis_head->next = temp->next;

    free(temp);
}

void cfl_free_hypothesis_load_list(cfl_hypothesis_load_list* load_list)
{
    while(load_list)
    {
        cfl_hypothesis_load_list* temp = load_list;

        load_list = load_list->next;

        cfl_free_type(temp->left);
        cfl_free_type(temp->right);
        free(temp);
    }
}

bool cfl_load_global_hypotheses(
        unsigned int* hypotheses_added,
        cfl_hypothesis_load_list** load_list,
        cfl_type_hypothesis_chain* hypothesis_head)
{
    unsigned int id = cfl_type_get_next_id();

    if(!cfl_push_hypothesis(hypothesis_head, "random", id))
        return false;

    *hypotheses_added = 1;

    cfl_type* input = cfl_create_new_type_integer();

    if(!input)
        return false;

    cfl_type* output = cfl_create_new_type_integer();

    if(!output)
    {
        cfl_free_type(input);

        return false;
    }

    cfl_type* arrow = cfl_create_new_type_arrow(input, output);

    if(!arrow)
        return false;

    cfl_type* variable = cfl_create_new_type_variable(id);

    if(!variable)
    {
        cfl_free_type(variable);
        cfl_free_type(arrow);

        return false;
    }

    *load_list = cfl_type_malloc(sizeof(cfl_hypothesis_load_list));

    if(!*load_list)
    {
        cfl_free_type(variable);
        cfl_free_type(arrow);

        return false;
    }

    (*load_list)->left = arrow;
    (*load_list)->right = variable;
    (*load_list)->next = 0;

    return true;
}

bool cfl_load_hypotheses_equations(
        cfl_type_equations* equations,
        cfl_hypothesis_load_list* load_list)
{
    while(load_list)
    {
        cfl_type* left = cfl_copy_new_type(load_list->left);

        if(!left)
            return false;

        cfl_type* right = cfl_copy_new_type(load_list->right);

        if(!right)
        {
            cfl_free_type(left);

            return false;
        }

        if(!cfl_add_type_equations(equations, left, right))
            return false;

        load_list = load_list->next;
    }

    return true;
}
