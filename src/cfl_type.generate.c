#include "cfl_type.h"

#include <stdio.h>
#include <string.h>

extern void* cfl_type_malloc(size_t size);

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
