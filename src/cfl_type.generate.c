#include "cfl_type.h"

#include <stdio.h>
#include <string.h>

extern void* cfl_type_malloc(size_t size);

static unsigned int next_id;

void cfl_reset_type_generator(void)
{
    next_id = 1;
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

static unsigned int cfl_generate_hypotheses(
        cfl_type** variable_type,
        cfl_type_hypothesis_chain* hypothesis_head,
        cfl_node* variable)
{
    if(variable->type == CFL_NODE_VARIABLE)
    {
        unsigned int id = next_id++;

        cfl_type_hypothesis_chain* hypothesis_chain_node =
            cfl_type_malloc(sizeof(cfl_type_hypothesis_chain));

        if(!hypothesis_chain_node)
            return 0;

        hypothesis_chain_node->name = variable->data;
        hypothesis_chain_node->id = id;
        hypothesis_chain_node->next = hypothesis_head->next;

        hypothesis_head->next = hypothesis_chain_node;

        *variable_type = cfl_type_malloc(sizeof(cfl_type));

        if(!*variable_type)
            return 0;

        cfl_create_type_variable(*variable_type, id);

        return 1;
    }

    cfl_type** children = cfl_type_malloc(sizeof(cfl_type*) *
                                          variable->number_of_children);

    if(!children)
        return 0;

    unsigned int hypothesis_count = 0;

    int i = 0;
    for( ; i < variable->number_of_children; ++i)
    {
        unsigned int change = cfl_generate_hypotheses(&children[i],
                                                      hypothesis_head,
                                                      variable->children[i]);

        if(!change)
        {
            int j = 0;
            for( ; j < i; ++j)
            {
                cfl_free_type(children[j]);

                cfl_type_hypothesis_chain* temp = hypothesis_head->next;

                hypothesis_head->next = temp->next;

                free(temp);
            }

            free(children);

            return 0;
        }

        hypothesis_count += change;
    }

    *variable_type = cfl_type_malloc(sizeof(cfl_type));

    if(!*variable_type)
    {
        for(i = 0; i < variable->number_of_children; ++i)
        {
            cfl_free_type(children[i]);

            cfl_type_hypothesis_chain* temp = hypothesis_head->next;

            hypothesis_head->next = temp->next;

            free(temp);
        }

        free(children);

        return 0;
    }

    cfl_create_type_tuple(*variable_type, variable->number_of_children, children);

    return hypothesis_count;
}

static bool cfl_generate_type_equation_chain_given_variable(
        cfl_type** variable_type,
        cfl_type** node_type,
        cfl_type_equation_chain* equation_head,
        cfl_type_hypothesis_chain* hypothesis_head,
        cfl_node* variable,
        cfl_node* node)
{
   unsigned int added_hypotheses = cfl_generate_hypotheses(variable_type,
                                                           hypothesis_head,
                                                           variable);

   *node_type = cfl_generate_type_equation_chain(equation_head,
                                                 hypothesis_head,
                                                 node);

    for( ; added_hypotheses > 0; --added_hypotheses)
    {
        cfl_type_hypothesis_chain* temp = hypothesis_head->next;

        hypothesis_head->next = temp->next;

        free(temp);
    }

    if(!*node_type)
        return false;

    return true;
}

cfl_type* cfl_generate_type_equation_chain(
        cfl_type_equation_chain* equation_head,
        cfl_type_hypothesis_chain* hypothesis_head,
        cfl_node* node)
{
    cfl_type* result = 0;

    int i;
    int j;
    unsigned int id0;
    unsigned int id1;
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
                cfl_type_error_undefined_variable(node->data);

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
        case CFL_NODE_CHAR:
            result = cfl_type_malloc(sizeof(cfl_type));

            if(!result)
                break;

            cfl_create_type_char(result);

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

                    child_type0 = cfl_generate_type_equation_chain(equation_head,
                                                                   hypothesis_head,
                                                                   node->children[i]);

                    temp_type0 = cfl_type_malloc(sizeof(cfl_type));

                    if(!temp_type0)
                    {
                        cfl_free_type(child_type0);

                        for(j = 0; j < i; ++j)
                            cfl_free_type(children[j]);

                        free(children);

                        return 0;
                    }

                    cfl_create_type_variable(temp_type0, id0);

                    if(!cfl_add_equation(equation_head, child_type0, temp_type0))
                    {
                        cfl_free_type(child_type0);
                        free(temp_type0);

                        for(j = 0; j < i; ++j)
                            cfl_free_type(children[j]);

                        free(children);

                        return 0;
                    }

                    children[i] = cfl_type_malloc(sizeof(cfl_type));

                    if(!children[i])
                    {
                        for(j = 0; j < i; ++j)
                            cfl_free_type(children[j]);

                        free(children);

                        return 0;
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
            if(!cfl_generate_type_equation_chain_given_variable(&temp_type0,
                                                                &child_type1,
                                                                equation_head,
                                                                hypothesis_head,
                                                                node->children[0],
                                                                node->children[1]))
                break;

            result = cfl_type_malloc(sizeof(cfl_type));

            if(!result)
            {
                cfl_free_type(temp_type0);
                cfl_free_type(child_type1);
            }

            cfl_create_type_arrow(result, temp_type0, child_type1);

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

            hypothesis_chain_node =
                cfl_type_malloc(sizeof(cfl_type_hypothesis_chain));

            if(!hypothesis_chain_node)
            {
                cfl_free_type(temp_type0);

                break;
            }

            hypothesis_chain_node->name = node->children[0]->data;
            hypothesis_chain_node->id = id0;
            hypothesis_chain_node->next = hypothesis_head->next;

            hypothesis_head->next = hypothesis_chain_node;

            result = cfl_generate_type_equation_chain(equation_head,
                                                      hypothesis_head,
                                                      node->children[3]);

            if(!cfl_generate_type_equation_chain_given_variable(&temp_type1,
                                                                &child_type0,
                                                                equation_head,
                                                                hypothesis_head,
                                                                node->children[1],
                                                                node->children[2]))
            {
                cfl_free_type(temp_type0);

                hypothesis_chain_node = hypothesis_head->next;

                hypothesis_head->next = hypothesis_chain_node->next;

                free(hypothesis_chain_node);

                return 0;
            }


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

            hypothesis_chain_node =
                cfl_type_malloc(sizeof(cfl_type_hypothesis_chain));

            if(!hypothesis_chain_node)
                break;

            hypothesis_chain_node->name = node->children[3]->data;
            hypothesis_chain_node->id = id0;
            hypothesis_chain_node->next = hypothesis_head->next;

            hypothesis_head->next = hypothesis_chain_node;

            bool success = cfl_generate_type_equation_chain_given_variable(
                    &temp_type0, &child_type0, equation_head,
                    hypothesis_head, node->children[2], node->children[4]);

            hypothesis_head->next = hypothesis_chain_node->next;

            free(hypothesis_chain_node);

            if(!success)
                return 0;

            temp_type1 = cfl_type_malloc(sizeof(cfl_type));

            if(!temp_type1)
            {
                cfl_free_type(child_type0);
                cfl_free_type(temp_type0);

                break;
            }

            cfl_create_type_variable(temp_type1, id1);

            if(!cfl_add_equation(equation_head, child_type0, temp_type1))
            {
                cfl_free_type(temp_type0);

                break;
            }

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
