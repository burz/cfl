#include "cfl_type.typed_program.h"
#include "cfl_typed_program.h"
#include "cfl_type.h"

#include <string.h>
#include <stdio.h>

extern void* cfl_type_malloc(size_t size);
extern unsigned int cfl_type_get_next_id(void);
extern void cfl_reset_type_generator(void);

static cfl_type* cfl_lookup_variable(
        cfl_type_equations* equations,
        cfl_type_hypothesis_chain* hypotheses,
        cfl_typed_definition_list* definitions,
        char* name)
{
    while(hypotheses)
    {
        if(!strcmp(name, hypotheses->name))
            return cfl_create_new_type_variable(hypotheses->id);

        hypotheses = hypotheses->next;
    }

    while(definitions)
    {
        if(!strcmp(name, definitions->name))
            return cfl_copy_new_type(definitions->definition->resulting_type);

        definitions = definitions->next;
    }

    if(!strcmp(name, "random"))
    {
        cfl_type* integer0 = cfl_create_new_type_integer();

        if(!integer0)
            return 0;

        cfl_type* integer1 = cfl_create_new_type_integer();

        if(!integer1)
        {
            cfl_free_type(integer0);

            return 0;
        }

        return cfl_create_new_type_arrow(integer0, integer1);
    }

    cfl_type_error_undefined_variable(name);

    return 0;
}

static bool cfl_generate_typed_node_for_complex_variable(
        unsigned int* added_hypotheses,
        cfl_typed_node** typed_node,
        cfl_type_equations* equations,
        cfl_type_hypothesis_chain* hypothesis_head,
        cfl_typed_definition_list* definitions,
        cfl_node* node)
{
    *added_hypotheses = 0;

    if(node->type == CFL_NODE_TUPLE)
    {
        cfl_typed_node** children =
            cfl_type_malloc(sizeof(cfl_typed_node*) * node->number_of_children);

        if(!children)
            return false;

        cfl_type** child_types =
            cfl_type_malloc(sizeof(cfl_type*) * node->number_of_children);

        if(!child_types)
        {
            free(children);

            return false;
        }

        int i = 0;
        for( ; i < node->number_of_children; ++i)
        {
            unsigned int added;

            if(!cfl_generate_typed_node_for_complex_variable(
                    &added, &children[i], equations, hypothesis_head,
                    definitions, node->children[i]))
            {
                int j = 0;
                for( ; j < i; ++j)
                {
                    cfl_free_typed_node(children[j]);
                    cfl_free_type(child_types[j]);
                }

                free(children);
                free(child_types);

                for( ; i < node->number_of_children; ++i)
                    cfl_free_node(node->children[i]);

                free(node->children);

                node->number_of_children = 0;

                for(i = 0; i < *added_hypotheses; ++i)
                    cfl_pop_hypothesis(hypothesis_head);

                return false;
            }

            child_types[i] = cfl_copy_new_type(children[i]->resulting_type);

            if(!child_types[i])
            {
                int j = 0;
                for( ; j < i; ++j)
                {
                    cfl_free_typed_node(children[j]);
                    cfl_free_type(child_types[j]);
                }

                free(children[i]);
                free(children);
                free(child_types);

                for( ; i < node->number_of_children; ++i)
                    cfl_free_node(node->children[i]);

                free(node->children);

                node->number_of_children = 0;

                for(i = 0; i < *added_hypotheses; ++i)
                    cfl_pop_hypothesis(hypothesis_head);

                return false;
            }

            *added_hypotheses += added;

            free(node->children[i]);
        }

        free(node->children);

        unsigned int number_of_children = node->number_of_children;

        node->number_of_children = 0;

        cfl_type* tuple =
            cfl_create_new_type_tuple(number_of_children, child_types);

        if(!tuple)
        {
            for(i = 0; i < number_of_children; ++i)
            {
                cfl_free_typed_node(children[i]);
                cfl_free_type(child_types[i]);
            }

            free(children);
            free(child_types);

            return false;
        }

        *typed_node = cfl_create_typed_node(
            CFL_NODE_TUPLE, tuple, number_of_children, 0, children);
    }
    else
    {
        unsigned int id = cfl_type_get_next_id();

        cfl_type* variable = cfl_create_new_type_variable(id);

        *typed_node =
            cfl_create_typed_node(CFL_NODE_VARIABLE, variable, 0, node->data, 0);

        if(!*typed_node)
            return false;

        if(strcmp(node->data, "_"))
        {
            if(!cfl_push_hypothesis(hypothesis_head, (*typed_node)->data, id))
            {
                cfl_free_typed_node(*typed_node);

                return false;
            }

            ++(*added_hypotheses);
        }

        node->data = 0;
    }

    return true;
}

static cfl_typed_node* cfl_generate_typed_node_for_binary_expression(
        cfl_type_equations* equations,
        cfl_type_hypothesis_chain* hypothesis_head,
        cfl_typed_definition_list* definitions,
        cfl_type* (*child_type_constructor)(void),
        cfl_type* (*result_type_constructor)(void),
        cfl_node* node)
{
    cfl_typed_node* left_child = cfl_generate_typed_node(
        equations, hypothesis_head, definitions, node->children[0]);

    if(!left_child)
        return 0;

    cfl_type* left_type = cfl_copy_new_type(left_child->resulting_type);

    if(!left_type)
    {
        cfl_free_typed_node(left_child);

        return 0;
    }

    cfl_type* child_type = (*child_type_constructor)();

    if(!child_type)
    {
        cfl_free_type(left_type);
        cfl_free_typed_node(left_child);

        return 0;
    }

    if(!cfl_add_type_equations(equations, left_type, child_type))
    {
        cfl_free_typed_node(left_child);

        return 0;
    }

    cfl_typed_node* right_child = cfl_generate_typed_node(
        equations, hypothesis_head, definitions, node->children[1]);

    if(!right_child)
    {
        cfl_free_typed_node(left_child);

        return 0;
    }

    free(node->children[0]);
    free(node->children[1]);
    free(node->children);

    node->number_of_children = 0;

    cfl_type* right_type = cfl_copy_new_type(right_child->resulting_type);

    if(!right_type)
    {
        cfl_free_typed_node(right_child);
        cfl_free_typed_node(left_child);

        return 0;
    }

    child_type = (*child_type_constructor)();

    if(!child_type)
    {
        cfl_free_type(right_type);
        cfl_free_typed_node(right_child);
        cfl_free_typed_node(left_child);

        return 0;
    }

    if(!cfl_add_type_equations(equations, right_type, child_type))
    {
        cfl_free_typed_node(right_child);
        cfl_free_typed_node(left_child);

        return 0;
    }

    cfl_type* type = (*result_type_constructor)();

    if(!type)
    {
        cfl_free_typed_node(right_child);
        cfl_free_typed_node(left_child);

        return 0;
    }

    cfl_typed_node** children = cfl_type_malloc(sizeof(cfl_typed_node*) * 2);

    if(!children)
    {
        cfl_free_typed_node(right_child);
        cfl_free_typed_node(left_child);

        return 0;
    }

    children[0] = left_child;
    children[1] = right_child;

    return cfl_create_typed_node(node->type, type, 2, 0, children);
}

cfl_typed_node* cfl_generate_typed_node(
        cfl_type_equations* equations,
        cfl_type_hypothesis_chain* hypothesis_head,
        cfl_typed_definition_list* definitions,
        cfl_node* node)
{
    if(node->type == CFL_NODE_VARIABLE)
    {
        cfl_type* type = cfl_lookup_variable(equations,
                                             hypothesis_head->next,
                                             definitions,
                                             node->data);

        if(!type)
            return 0;

        cfl_typed_node* result =
            cfl_create_typed_node(CFL_NODE_VARIABLE, type, 0, node->data, 0);

        node->data = 0;

        return result;
    }
    else if(node->type == CFL_NODE_BOOL)
    {
        cfl_type* type = cfl_create_new_type_bool();

        if(!type)
            return 0;

        cfl_typed_node* result =
            cfl_create_typed_node(CFL_NODE_BOOL, type, 0, node->data, 0);

        node->data = 0;

        return result;
    }
    else if(node->type == CFL_NODE_INTEGER)
    {
        cfl_type* type = cfl_create_new_type_integer();

        if(!type)
            return 0;

        cfl_typed_node* result =
            cfl_create_typed_node(CFL_NODE_INTEGER, type, 0, node->data, 0);

        node->data = 0;

        return result;
    }
    else if(node->type == CFL_NODE_CHAR)
    {
        cfl_type* type = cfl_create_new_type_char();

        if(!type)
            return 0;

        cfl_typed_node* result =
            cfl_create_typed_node(CFL_NODE_CHAR, type, 0, node->data, 0);

        node->data = 0;

        return result;
    }
    else if(node->type == CFL_NODE_LIST)
    {
        unsigned int id = cfl_type_get_next_id();

        cfl_typed_node_list typed_list;
        cfl_typed_node_list* typed_pos = &typed_list;

        cfl_list_node* pos = node->data;

        node->data = 0;

        while(pos)
        {
            cfl_typed_node* child_node = cfl_generate_typed_node(
                equations, hypothesis_head, definitions, pos->node);

            cfl_list_node* temp = pos;

            pos = pos->next;

            free(temp->node);
            free(temp);

            if(!child_node)
            {
                typed_pos->next = 0;

                cfl_free_typed_node_list(typed_list.next);
                cfl_delete_list_nodes(pos);

                return 0;
            }

            cfl_type* child_type = cfl_copy_new_type(child_node->resulting_type);

            if(!child_type)
            {
                cfl_free_typed_node(child_node);

                typed_pos->next = 0;

                cfl_free_typed_node_list(typed_list.next);
                cfl_delete_list_nodes(pos);

                return 0;
            }

            cfl_type* variable = cfl_create_new_type_variable(id);

            if(!variable)
            {
                cfl_free_type(child_type);
                cfl_free_typed_node(child_node);

                typed_pos->next = 0;

                cfl_free_typed_node_list(typed_list.next);
                cfl_delete_list_nodes(pos);

                return 0;
            }

            if(!cfl_add_type_equations(equations, child_type, variable))
            {
                cfl_free_typed_node(child_node);

                typed_pos->next = 0;

                cfl_free_typed_node_list(typed_list.next);
                cfl_delete_list_nodes(pos);

                return 0;
            }

            typed_pos->next = cfl_type_malloc(sizeof(cfl_typed_node_list));

            if(!typed_pos->next)
            {
                cfl_free_typed_node(child_node);
                cfl_free_typed_node_list(typed_list.next);
                cfl_delete_list_nodes(pos);

                return 0;
            }

            typed_pos->next->node = child_node;

            typed_pos = typed_pos->next;
        }

        typed_pos->next = 0;

        cfl_type* variable = cfl_create_new_type_variable(id);

        if(!variable)
        {
            cfl_free_typed_node_list(typed_list.next);

            return 0;
        }

        cfl_type* list_type = cfl_create_new_type_list(variable);

        if(!list_type)
        {
            cfl_free_typed_node_list(typed_list.next);

            return 0;
        }

        return cfl_create_typed_node(CFL_NODE_LIST, list_type, 0, typed_list.next, 0);
    }
    else if(node->type == CFL_NODE_TUPLE)
    {
        cfl_typed_node** children =
            cfl_type_malloc(sizeof(cfl_typed_node*) * node->number_of_children);

        if(!children)
            return 0;

        cfl_type** children_types =
            cfl_type_malloc(sizeof(cfl_type*) * node->number_of_children);

        if(!children_types)
        {
            free(children);

            return 0;
        }

        int i = 0;
        for( ; i < node->number_of_children; ++i)
        {
            children[i] = cfl_generate_typed_node(
                equations, hypothesis_head, definitions, node->children[i]);

            if(!children[i])
            {
                int j = 0;
                for( ; j < i; ++j)
                {
                    cfl_free_typed_node(children[j]);
                    cfl_free_type(children_types[j]);
                }

                for(++i; i < node->number_of_children; ++i)
                    cfl_free_node(node->children[i]);

                free(node->children);

                node->number_of_children = 0;

                return 0;
            }

            children_types[i] = cfl_copy_new_type(children[i]->resulting_type);

            if(!children_types[i])
            {
                int j = 0;
                for( ; j < i; ++j)
                {
                    cfl_free_typed_node(children[j]);
                    cfl_free_type(children_types[j]);
                }

                cfl_free_typed_node(children[i]);

                for(++i; i < node->number_of_children; ++i)
                    cfl_free_node(node->children[i]);

                free(node->children);

                node->number_of_children = 0;

                return 0;
            }

            free(node->children[i]);
        }

        free(node->children);

        cfl_type* tuple_type =
                cfl_create_new_type_tuple(node->number_of_children, children_types);

        if(!tuple_type)
        {
            int i = 0;
            for( ; i < node->number_of_children; ++i)
                cfl_free_typed_node(children[i]);

            free(children);
        }

        unsigned int number_of_children = node->number_of_children;

        node->number_of_children = 0;

        return cfl_create_typed_node(
            CFL_NODE_TUPLE, tuple_type, number_of_children, 0, children);
    }
    else if(node->type == CFL_NODE_FUNCTION)
    {
        unsigned int added_hypotheses;
        cfl_typed_node* typed_variable;

        if(!cfl_generate_typed_node_for_complex_variable(
                &added_hypotheses, &typed_variable, equations, hypothesis_head,
                definitions, node->children[0]))
            return 0;

        cfl_typed_node* typed_result = cfl_generate_typed_node(
            equations, hypothesis_head, definitions, node->children[1]);

        int i = 0;
        for( ; i < added_hypotheses; ++i)
            cfl_pop_hypothesis(hypothesis_head);

        if(!typed_result)
        {
            cfl_free_typed_node(typed_variable);

            return 0;
        }

        free(node->children[0]);
        free(node->children[1]);
        free(node->children);

        node->number_of_children = 0;

        cfl_type* variable_type =
            cfl_copy_new_type(typed_variable->resulting_type);

        if(!variable_type)
        {
            cfl_free_typed_node(typed_variable);
            cfl_free_typed_node(typed_result);

            return 0;
        }

        cfl_type* result_type = cfl_copy_new_type(typed_result->resulting_type);

        if(!result_type)
        {
            cfl_free_type(variable_type);
            cfl_free_typed_node(typed_variable);
            cfl_free_typed_node(typed_result);

            return 0;
        }

        cfl_type* type = cfl_create_new_type_arrow(variable_type, result_type);

        if(!type)
        {
            cfl_free_typed_node(typed_variable);
            cfl_free_typed_node(typed_result);

            return 0;
        }

        cfl_typed_node** children = cfl_type_malloc(sizeof(cfl_typed_node*) * 2);

        if(!children)
        {
            cfl_free_typed_node(typed_variable);
            cfl_free_typed_node(typed_result);

            return 0;
        }

        children[0] = typed_variable;
        children[1] = typed_result;

        return cfl_create_typed_node(CFL_NODE_FUNCTION, type, 2, 0, children);
    }
    else if(node->type == CFL_NODE_AND || node->type == CFL_NODE_OR)
        return cfl_generate_typed_node_for_binary_expression(
            equations, hypothesis_head, definitions, &cfl_create_new_type_bool,
            &cfl_create_new_type_bool, node);
    else if(node->type == CFL_NODE_NOT)
    {
        cfl_typed_node* typed_child = cfl_generate_typed_node(
            equations, hypothesis_head, definitions, node->children[0]);

        if(!typed_child)
            return 0;

        free(node->children[0]);
        free(node->children);

        node->number_of_children = 0;

        cfl_type* child_type = cfl_copy_new_type(typed_child->resulting_type);

        if(!child_type)
        {
            cfl_free_typed_node(typed_child);

            return 0;
        }

        cfl_type* boolean = cfl_create_new_type_bool();

        if(!boolean)
        {
            cfl_free_type(child_type);
            cfl_free_typed_node(typed_child);

            return 0;
        }

        if(!cfl_add_type_equations(equations, child_type, boolean))
        {
            cfl_free_typed_node(typed_child);

            return 0;
        }

        boolean = cfl_create_new_type_bool();

        if(!boolean)
        {
            cfl_free_typed_node(typed_child);

            return 0;
        }

        cfl_typed_node** children = cfl_type_malloc(sizeof(cfl_typed_node*));

        if(!children)
        {
            cfl_free_typed_node(typed_child);

            return 0;
        }

        children[0] = typed_child;

        return cfl_create_typed_node(CFL_NODE_NOT, boolean, 1, 0, children);
    }
    else if(node->type == CFL_NODE_ADD || node->type == CFL_NODE_MULTIPLY ||
            node->type == CFL_NODE_DIVIDE)
        return cfl_generate_typed_node_for_binary_expression(
            equations, hypothesis_head, definitions, &cfl_create_new_type_integer,
            &cfl_create_new_type_integer, node);
    else if(node->type == CFL_NODE_EQUAL || node->type == CFL_NODE_LESS)
        return cfl_generate_typed_node_for_binary_expression(
            equations, hypothesis_head, definitions, &cfl_create_new_type_integer,
            &cfl_create_new_type_bool, node);
    else if(node->type == CFL_NODE_APPLICATION)
    {
        cfl_typed_node* typed_argument = cfl_generate_typed_node(
            equations, hypothesis_head, definitions, node->children[1]);

        if(!typed_argument)
            return 0;

        cfl_type* argument_type = cfl_copy_new_type(typed_argument->resulting_type);

        if(!argument_type)
        {
            cfl_free_typed_node(typed_argument);

            return 0;
        }

        unsigned int id = cfl_type_get_next_id();

        cfl_type* result_type = cfl_create_new_type_variable(id);

        if(!result_type)
        {
            cfl_free_type(argument_type);
            cfl_free_typed_node(typed_argument);

            return 0;
        }

        cfl_type* arrow = cfl_create_new_type_arrow(argument_type, result_type);

        if(!arrow)
        {
            cfl_free_typed_node(typed_argument);

            return 0;
        }

        cfl_typed_node* typed_function = cfl_generate_typed_node(
            equations, hypothesis_head, definitions, node->children[0]);

        if(!typed_function)
        {
            cfl_free_type(arrow);
            cfl_free_typed_node(typed_argument);

            return 0;
        }

        free(node->children[0]);
        free(node->children[1]);
        free(node->children);

        node->number_of_children = 0;

        cfl_type* function_type = cfl_copy_new_type(typed_function->resulting_type);

        if(!function_type)
        {
            cfl_free_typed_node(typed_function);
            cfl_free_type(arrow);
            cfl_free_typed_node(typed_argument);

            return 0;
        }

        if(!cfl_add_type_equations(equations, function_type, arrow))
        {
            cfl_free_typed_node(typed_function);
            cfl_free_typed_node(typed_argument);

            return 0;
        }

        result_type = cfl_create_new_type_variable(id);

        if(!result_type)
        {
            cfl_free_type(argument_type);
            cfl_free_typed_node(typed_argument);

            return 0;
        }

        cfl_typed_node** children = cfl_type_malloc(sizeof(cfl_typed_node*) * 2);

        if(!children)
        {
            cfl_free_type(result_type);
            cfl_free_typed_node(typed_function);
            cfl_free_typed_node(typed_argument);

            return 0;
        }

        children[0] = typed_function;
        children[1] = typed_argument;

        return cfl_create_typed_node(CFL_NODE_APPLICATION, result_type, 2, 0, children);
    }
    else if(node->type == CFL_NODE_IF)
    {
        cfl_typed_node* typed_condition = cfl_generate_typed_node(
            equations, hypothesis_head, definitions, node->children[0]);

        if(!typed_condition)
            return 0;

        cfl_type* condition_type = cfl_copy_new_type(typed_condition->resulting_type);

        if(!condition_type)
        {
            cfl_free_typed_node(typed_condition);

            return 0;
        }

        cfl_type* boolean = cfl_create_new_type_bool();

        if(!boolean)
        {
            cfl_free_type(condition_type);
            cfl_free_typed_node(typed_condition);

            return 0;
        }

        if(!cfl_add_type_equations(equations, condition_type, boolean))
        {
            cfl_free_typed_node(typed_condition);

            return 0;
        }

        cfl_typed_node* typed_then = cfl_generate_typed_node(
            equations, hypothesis_head, definitions, node->children[1]);

        if(!typed_then)
        {
            cfl_free_typed_node(typed_condition);

            return 0;
        }

        cfl_type* then_type = cfl_copy_new_type(typed_then->resulting_type);

        if(!then_type)
        {
            cfl_free_typed_node(typed_then);
            cfl_free_typed_node(typed_condition);

            return 0;
        }

        unsigned int id = cfl_type_get_next_id();

        cfl_type* result_type = cfl_create_new_type_variable(id);

        if(!result_type)
        {
            cfl_free_type(then_type);
            cfl_free_typed_node(typed_then);
            cfl_free_typed_node(typed_condition);

            return 0;
        }

        if(!cfl_add_type_equations(equations, then_type, result_type))
        {
            cfl_free_typed_node(typed_then);
            cfl_free_typed_node(typed_condition);

            return 0;
        }

        cfl_typed_node* typed_else = cfl_generate_typed_node(
            equations, hypothesis_head, definitions, node->children[2]);

        if(!typed_else)
        {
            cfl_free_typed_node(typed_then);
            cfl_free_typed_node(typed_condition);

            return 0;
        }

        free(node->children[0]);
        free(node->children[1]);
        free(node->children[2]);
        free(node->children);

        node->number_of_children = 0;

        cfl_type* else_type = cfl_copy_new_type(typed_else->resulting_type);

        if(!else_type)
        {
            cfl_free_typed_node(typed_else);
            cfl_free_typed_node(typed_then);
            cfl_free_typed_node(typed_condition);

            return 0;
        }

        result_type = cfl_create_new_type_variable(id);

        if(!result_type)
        {
            cfl_free_type(else_type);
            cfl_free_typed_node(typed_else);
            cfl_free_typed_node(typed_then);
            cfl_free_typed_node(typed_condition);

            return 0;
        }

        if(!cfl_add_type_equations(equations, else_type, result_type))
        {
            cfl_free_typed_node(typed_else);
            cfl_free_typed_node(typed_then);
            cfl_free_typed_node(typed_condition);

            return 0;
        }

        result_type = cfl_create_new_type_variable(id);

        if(!result_type)
        {
            cfl_free_typed_node(typed_else);
            cfl_free_typed_node(typed_then);
            cfl_free_typed_node(typed_condition);

            return 0;
        }

        cfl_typed_node** children = cfl_type_malloc(sizeof(cfl_typed_node*) * 3);

        if(!children)
        {
            cfl_free_type(result_type);
            cfl_free_typed_node(typed_else);
            cfl_free_typed_node(typed_then);
            cfl_free_typed_node(typed_condition);

            return 0;
        }

        children[0] = typed_condition;
        children[1] = typed_then;
        children[2] = typed_else;

        return cfl_create_typed_node(CFL_NODE_IF, result_type, 3, 0, children);
    }
    else if(node->type == CFL_NODE_LET_REC)
    {
        unsigned int function_id = cfl_type_get_next_id();

        cfl_type* function_type = cfl_create_new_type_variable(function_id);

        if(!function_type)
            return 0;

        cfl_typed_node* typed_function = cfl_create_typed_node(
            CFL_NODE_VARIABLE, function_type, 0, node->children[0]->data, 0);

        node->children[0]->data = 0;

        if(!typed_function)
            return 0;

        if(!cfl_push_hypothesis(hypothesis_head, typed_function->data, function_id))
        {
            cfl_free_typed_node(typed_function);

            return 0;
        }

        unsigned int added_hypotheses;
        cfl_typed_node* typed_argument;

        if(!cfl_generate_typed_node_for_complex_variable(
                &added_hypotheses, &typed_argument, equations, hypothesis_head,
                definitions, node->children[1]))
        {
            cfl_pop_hypothesis(hypothesis_head);

            cfl_free_typed_node(typed_function);

            return 0;
        }

        cfl_typed_node* typed_def = cfl_generate_typed_node(
            equations, hypothesis_head, definitions, node->children[2]);

        int i = 0;
        for( ; i < added_hypotheses; ++i)
            cfl_pop_hypothesis(hypothesis_head);

        if(!typed_def)
        {
            cfl_pop_hypothesis(hypothesis_head);

            cfl_free_typed_node(typed_argument);
            cfl_free_typed_node(typed_function);

            return 0;
        }

        cfl_typed_node* typed_in = cfl_generate_typed_node(
            equations, hypothesis_head, definitions, node->children[3]);

        cfl_pop_hypothesis(hypothesis_head);

        if(!typed_in)
        {
            cfl_free_typed_node(typed_def);
            cfl_free_typed_node(typed_argument);
            cfl_free_typed_node(typed_function);

            return 0;
        }

        free(node->children[0]);
        free(node->children[1]);
        free(node->children[2]);
        free(node->children[3]);
        free(node->children);

        node->number_of_children = 0;

        cfl_type* argument_type = cfl_copy_new_type(typed_argument->resulting_type);

        if(!argument_type)
        {
            cfl_free_typed_node(typed_in);
            cfl_free_typed_node(typed_def);
            cfl_free_typed_node(typed_argument);
            cfl_free_typed_node(typed_function);

            return 0;
        }

        cfl_type* def_type = cfl_copy_new_type(typed_def->resulting_type);

        if(!def_type)
        {
            cfl_free_type(argument_type);
            cfl_free_typed_node(typed_in);
            cfl_free_typed_node(typed_def);
            cfl_free_typed_node(typed_argument);
            cfl_free_typed_node(typed_function);

            return 0;
        }

        cfl_type* arrow = cfl_create_new_type_arrow(argument_type, def_type);

        if(!arrow)
        {
            cfl_free_typed_node(typed_in);
            cfl_free_typed_node(typed_def);
            cfl_free_typed_node(typed_argument);
            cfl_free_typed_node(typed_function);

            return 0;
        }

        function_type = cfl_create_new_type_variable(function_id);

        if(!function_type)
        {
            cfl_free_type(arrow);
            cfl_free_typed_node(typed_in);
            cfl_free_typed_node(typed_def);
            cfl_free_typed_node(typed_argument);
            cfl_free_typed_node(typed_function);

            return 0;
        }

        if(!cfl_add_type_equations(equations, function_type, arrow))
        {
            cfl_free_typed_node(typed_in);
            cfl_free_typed_node(typed_def);
            cfl_free_typed_node(typed_argument);
            cfl_free_typed_node(typed_function);

            return 0;
        }

        cfl_type* in_type = cfl_copy_new_type(typed_in->resulting_type);

        if(!in_type)
        {
            cfl_free_typed_node(typed_in);
            cfl_free_typed_node(typed_def);
            cfl_free_typed_node(typed_argument);
            cfl_free_typed_node(typed_function);

            return 0;
        }

        cfl_typed_node** children = cfl_type_malloc(sizeof(cfl_typed_node*) * 4);

        if(!children)
        {
            cfl_free_type(in_type);
            cfl_free_typed_node(typed_in);
            cfl_free_typed_node(typed_def);
            cfl_free_typed_node(typed_argument);
            cfl_free_typed_node(typed_function);

            return 0;
        }

        children[0] = typed_function;
        children[1] = typed_argument;
        children[2] = typed_def;
        children[3] = typed_in;

        return cfl_create_typed_node(CFL_NODE_LET_REC, in_type, 4, 0, children);
    }
    else if(node->type == CFL_NODE_PUSH)
    {
        cfl_typed_node* typed_left = cfl_generate_typed_node(
            equations, hypothesis_head, definitions, node->children[0]);

        if(!typed_left)
            return 0;

        cfl_type* left_type = cfl_copy_new_type(typed_left->resulting_type);

        if(!left_type)
        {
            cfl_free_typed_node(typed_left);

            return 0;
        }

        unsigned int id = cfl_type_get_next_id();

        cfl_type* variable = cfl_create_new_type_variable(id);

        if(!variable)
        {
            cfl_free_type(left_type);
            cfl_free_typed_node(typed_left);

            return 0;
        }

        if(!cfl_add_type_equations(equations, left_type, variable))
        {
            cfl_free_typed_node(typed_left);

            return 0;
        }

        variable = cfl_create_new_type_variable(id);

        if(!variable)
        {
            cfl_free_typed_node(typed_left);

            return 0;
        }

        cfl_type* list = cfl_create_new_type_list(variable);

        if(!list)
        {
            cfl_free_typed_node(typed_left);

            return 0;
        }

        cfl_type* list_copy = cfl_copy_new_type(list);

        if(!list_copy)
        {
            cfl_free_type(list);
            cfl_free_typed_node(typed_left);

            return 0;
        }

        cfl_typed_node* typed_right = cfl_generate_typed_node(
            equations, hypothesis_head, definitions, node->children[1]);

        if(!typed_right)
        {
            cfl_free_type(list_copy);
            cfl_free_type(list);
            cfl_free_typed_node(typed_left);

            return 0;
        }

        free(node->children[0]);
        free(node->children[1]);
        free(node->children);

        node->number_of_children = 0;

        cfl_type* right_type = cfl_copy_new_type(typed_right->resulting_type);

        if(!right_type)
        {
            cfl_free_typed_node(typed_right);
            cfl_free_type(list_copy);
            cfl_free_type(list);
            cfl_free_typed_node(typed_left);

            return 0;
        }

        if(!cfl_add_type_equations(equations, right_type, list_copy))
        {
            cfl_free_typed_node(typed_right);
            cfl_free_type(list);
            cfl_free_typed_node(typed_left);

            return 0;
        }

        cfl_typed_node** children = cfl_type_malloc(sizeof(cfl_typed_node*) * 2);

        if(!children)
        {
            cfl_free_typed_node(typed_right);
            cfl_free_type(list);
            cfl_free_typed_node(typed_left);

            return 0;
        }

        children[0] = typed_left;
        children[1] = typed_right;

        return cfl_create_typed_node(CFL_NODE_PUSH, list, 2, 0, children);
    }
    else if(node->type == CFL_NODE_CONCATENATE)
    {
        cfl_typed_node* typed_left = cfl_generate_typed_node(
            equations, hypothesis_head, definitions, node->children[0]);

        if(!typed_left)
            return 0;

        unsigned int id = cfl_type_get_next_id();

        cfl_type* variable = cfl_create_new_type_variable(id);

        if(!variable)
        {
            cfl_free_typed_node(typed_left);

            return 0;
        }

        cfl_type* list = cfl_create_new_type_list(variable);

        if(!list)
        {
            cfl_free_typed_node(typed_left);

            return 0;
        }

        cfl_type* list_copy = cfl_copy_new_type(list);

        if(!list_copy)
        {
            cfl_free_type(list);
            cfl_free_typed_node(typed_left);

            return 0;
        }

        cfl_type* left_type = cfl_copy_new_type(typed_left->resulting_type);

        if(!left_type)
        {
            cfl_free_type(list_copy);
            cfl_free_type(list);
            cfl_free_typed_node(typed_left);

            return 0;
        }

        if(!cfl_add_type_equations(equations, left_type, list_copy))
        {
            cfl_free_type(list);
            cfl_free_typed_node(typed_left);

            return 0;
        }

        cfl_typed_node* typed_right = cfl_generate_typed_node(
            equations, hypothesis_head, definitions, node->children[1]);

        if(!typed_right)
        {
            cfl_free_type(list);
            cfl_free_typed_node(typed_left);

            return 0;
        }

        free(node->children[0]);
        free(node->children[1]);
        free(node->children);

        node->number_of_children = 0;

        list_copy = cfl_copy_new_type(list);

        if(!list_copy)
        {
            cfl_free_typed_node(typed_right);
            cfl_free_type(list);
            cfl_free_typed_node(typed_left);

            return 0;
        }

        cfl_type* right_type = cfl_copy_new_type(typed_right->resulting_type);

        if(!right_type)
        {
            cfl_free_type(list_copy);
            cfl_free_typed_node(typed_right);
            cfl_free_type(list);
            cfl_free_typed_node(typed_left);

            return 0;
        }

        if(!cfl_add_type_equations(equations, right_type, list_copy))
        {
            cfl_free_typed_node(typed_right);
            cfl_free_type(list);
            cfl_free_typed_node(typed_left);

            return 0;
        }

        cfl_typed_node** children = cfl_type_malloc(sizeof(cfl_typed_node*) * 2);

        if(!children)
        {
            cfl_free_typed_node(typed_right);
            cfl_free_type(list);
            cfl_free_typed_node(typed_left);

            return 0;
        }

        children[0] = typed_left;
        children[1] = typed_right;

        return cfl_create_typed_node(CFL_NODE_CONCATENATE, list, 2, 0, children);
    }
    else if(node->type == CFL_NODE_CASE)
    {
        cfl_typed_node* typed_list = cfl_generate_typed_node(
            equations, hypothesis_head, definitions, node->children[0]);

        if(!typed_list)
            return 0;

        unsigned int list_element_id = cfl_type_get_next_id();

        cfl_type* variable = cfl_create_new_type_variable(list_element_id);

        if(!variable)
        {
            cfl_free_typed_node(typed_list);

            return 0;
        }

        cfl_type* list_var = cfl_create_new_type_list(variable);

        if(!list_var)
        {
            cfl_free_typed_node(typed_list);

            return 0;
        }

        cfl_type* list_var_copy = cfl_copy_new_type(list_var);

        if(!list_var_copy)
        {
            cfl_free_type(list_var);
            cfl_free_typed_node(typed_list);

            return 0;
        }

        cfl_type* list_type = cfl_copy_new_type(typed_list->resulting_type);

        if(!list_type)
        {
            cfl_free_type(list_var_copy);
            cfl_free_type(list_var);
            cfl_free_typed_node(typed_list);

            return 0;
        }

        if(!cfl_add_type_equations(equations, list_type, list_var_copy))
        {
            cfl_free_type(list_var);
            cfl_free_typed_node(typed_list);

            return 0;
        }

        variable = cfl_create_new_type_variable(list_element_id);

        if(!variable)
        {
            cfl_free_type(list_var);
            cfl_free_typed_node(typed_list);

            return 0;
        }

        cfl_typed_node* typed_head = cfl_create_typed_node(
            CFL_NODE_VARIABLE, variable, 0, node->children[2]->data, 0);

        node->children[2]->data = 0;

        if(!typed_head)
        {
            cfl_free_type(list_var);
            cfl_free_typed_node(typed_list);

            return 0;
        }

        list_var_copy = cfl_copy_new_type(list_var);

        if(!list_var_copy)
        {
            cfl_free_typed_node(typed_head);
            cfl_free_type(list_var);
            cfl_free_typed_node(typed_list);

            return 0;
        }

        cfl_typed_node* typed_tail = cfl_create_typed_node(
            CFL_NODE_VARIABLE, list_var_copy, 0, node->children[3]->data, 0);

        node->children[3]->data = 0;

        if(!typed_tail)
        {
            cfl_free_typed_node(typed_head);
            cfl_free_type(list_var);
            cfl_free_typed_node(typed_list);

            return 0;
        }

        unsigned int list_id = cfl_type_get_next_id();

        variable = cfl_create_new_type_variable(list_id);

        if(!variable)
        {
            cfl_free_typed_node(typed_tail);
            cfl_free_typed_node(typed_head);
            cfl_free_type(list_var);
            cfl_free_typed_node(typed_list);

            return 0;
        }

        if(!cfl_add_type_equations(equations, variable, list_var))
        {
            cfl_free_typed_node(typed_tail);
            cfl_free_typed_node(typed_head);
            cfl_free_typed_node(typed_list);

            return 0;
        }

        if(!cfl_push_hypothesis(hypothesis_head, typed_head->data, list_element_id))
        {
            cfl_free_typed_node(typed_tail);
            cfl_free_typed_node(typed_head);
            cfl_free_typed_node(typed_list);

            return 0;
        }

        if(!cfl_push_hypothesis(hypothesis_head, typed_tail->data, list_id))
        {
            cfl_pop_hypothesis(hypothesis_head);
            cfl_free_typed_node(typed_tail);
            cfl_free_typed_node(typed_head);
            cfl_free_typed_node(typed_list);

            return 0;
        }

        cfl_typed_node* typed_nonempty = cfl_generate_typed_node(
            equations, hypothesis_head, definitions, node->children[4]);

        cfl_pop_hypothesis(hypothesis_head);
        cfl_pop_hypothesis(hypothesis_head);

        if(!typed_nonempty)
        {
            cfl_free_typed_node(typed_tail);
            cfl_free_typed_node(typed_head);
            cfl_free_typed_node(typed_list);

            return 0;
        }

        cfl_type* nonempty_type = cfl_copy_new_type(typed_nonempty->resulting_type);

        if(!nonempty_type)
        {
            cfl_free_typed_node(typed_nonempty);
            cfl_free_typed_node(typed_tail);
            cfl_free_typed_node(typed_head);
            cfl_free_typed_node(typed_list);

            return 0;
        }

        unsigned int result_id = cfl_type_get_next_id();

        cfl_type* result_type = cfl_create_new_type_variable(result_id);

        if(!result_type)
        {
            cfl_free_type(nonempty_type);
            cfl_free_typed_node(typed_nonempty);
            cfl_free_typed_node(typed_tail);
            cfl_free_typed_node(typed_head);
            cfl_free_typed_node(typed_list);

            return 0;
        }

        if(!cfl_add_type_equations(equations, nonempty_type, result_type))
        {
            cfl_free_typed_node(typed_nonempty);
            cfl_free_typed_node(typed_tail);
            cfl_free_typed_node(typed_head);
            cfl_free_typed_node(typed_list);

            return 0;
        }

        cfl_typed_node* typed_empty = cfl_generate_typed_node(
            equations, hypothesis_head, definitions, node->children[1]);

        if(!typed_empty)
        {
            cfl_free_typed_node(typed_nonempty);
            cfl_free_typed_node(typed_tail);
            cfl_free_typed_node(typed_head);
            cfl_free_typed_node(typed_list);

            return 0;
        }

        free(node->children[0]);
        free(node->children[1]);
        free(node->children[2]);
        free(node->children[3]);
        free(node->children[4]);
        free(node->children);

        node->number_of_children = 0;

        cfl_type* empty_type = cfl_copy_new_type(typed_empty->resulting_type);

        if(!empty_type)
        {
            cfl_free_typed_node(typed_empty);
            cfl_free_typed_node(typed_nonempty);
            cfl_free_typed_node(typed_tail);
            cfl_free_typed_node(typed_head);
            cfl_free_typed_node(typed_list);

            return 0;
        }

        result_type = cfl_create_new_type_variable(result_id);

        if(!result_type)
        {
            cfl_free_type(empty_type);
            cfl_free_typed_node(typed_empty);
            cfl_free_typed_node(typed_nonempty);
            cfl_free_typed_node(typed_tail);
            cfl_free_typed_node(typed_head);
            cfl_free_typed_node(typed_list);

            return 0;
        }

        if(!cfl_add_type_equations(equations, empty_type, result_type))
        {
            cfl_free_typed_node(typed_empty);
            cfl_free_typed_node(typed_nonempty);
            cfl_free_typed_node(typed_tail);
            cfl_free_typed_node(typed_head);
            cfl_free_typed_node(typed_list);

            return 0;
        }

        result_type = cfl_create_new_type_variable(result_id);

        if(!result_type)
        {
            cfl_free_typed_node(typed_empty);
            cfl_free_typed_node(typed_nonempty);
            cfl_free_typed_node(typed_tail);
            cfl_free_typed_node(typed_head);
            cfl_free_typed_node(typed_list);

            return 0;
        }

        cfl_typed_node** children = cfl_type_malloc(sizeof(cfl_typed_node*) * 5);

        if(!children)
        {
            cfl_free_typed_node(typed_empty);
            cfl_free_typed_node(typed_nonempty);
            cfl_free_typed_node(typed_tail);
            cfl_free_typed_node(typed_head);
            cfl_free_typed_node(typed_list);

            return 0;
        }

        children[0] = typed_list;
        children[1] = typed_empty;
        children[2] = typed_head;
        children[3] = typed_tail;
        children[4] = typed_nonempty;

        return cfl_create_typed_node(CFL_NODE_CASE, result_type, 5, 0, children);
    }

    return 0;
}

bool cfl_simplify_typed_node(cfl_type_equations* equations, cfl_typed_node* node)
{
    if(!cfl_simplify_type(equations, node->resulting_type))
        return false;

    if(node->node_type == CFL_NODE_LIST)
    {
        cfl_typed_node_list* pos = node->data;

        while(pos)
        {
            if(!cfl_simplify_typed_node(equations, pos->node))
                return false;

            pos = pos->next;
        }
    }
    else
    {
        int i = 0;
        for( ; i < node->number_of_children; ++i)
            if(!cfl_simplify_typed_node(equations, node->children[i]))
                return false;
    }

    return true;
}

static cfl_typed_definition_list* cfl_generate_definition_type(
        cfl_type_hypothesis_chain* hypothesis_head,
        cfl_hypothesis_load_list* load_list,
        cfl_typed_definition_list* definitions,
        unsigned int equation_hash_table_length,
        cfl_definition_list* definition)
{
    cfl_type_equations equations;
    cfl_initialize_type_equations(&equations, equation_hash_table_length);

    if(!cfl_load_hypotheses_equations(&equations, load_list))
        return 0;

    cfl_typed_definition_list* result =
        cfl_type_malloc(sizeof(cfl_typed_definition_list));

    if(!result)
        return 0;

    result->name = cfl_type_malloc(sizeof(char) * MAX_IDENTIFIER_LENGTH);

    if(!result->name)
    {
        free(result);

        return 0;
    }

    strcpy(result->name, definition->name->data);

    result->definition = cfl_generate_typed_node(&equations,
                                                 hypothesis_head,
                                                 definitions,
                                                 definition->definition);

    if(!result->definition)
    {
        cfl_delete_type_equations(&equations);
        free(result->name);
        free(result);

        return 0;
    }

    if(!cfl_close_type_equations(&equations))
    {
        cfl_delete_type_equations(&equations);
        free(result->name);
        cfl_free_typed_node(result->definition);
        free(result);

        return 0;
    }

    if(!cfl_are_type_equations_consistent(&equations))
    {
        cfl_type_error_bad_definition(result->name);

        cfl_delete_type_equations(&equations);
        free(result->name);
        cfl_free_typed_node(result->definition);
        free(result);

        return 0;
    }

    if(!cfl_simplify_typed_node(&equations, result->definition))
    {
        cfl_delete_type_equations(&equations);
        free(result->name);
        cfl_free_typed_node(result->definition);
        free(result);

        return 0;
    }

    cfl_delete_type_equations(&equations);

    return result;
}

cfl_typed_definition_list* cfl_generate_definition_types(
        cfl_type_hypothesis_chain* hypothesis_head,
        cfl_hypothesis_load_list* load_list,
        cfl_definition_list* definitions,
        unsigned int equation_hash_table_length)
{
    cfl_typed_definition_list definition_head;
    definition_head.next = 0;

    cfl_typed_definition_list* pos = &definition_head;

    while(definitions)
    {
        pos->next = cfl_generate_definition_type(hypothesis_head,
                                                 load_list,
                                                 definition_head.next,
                                                 equation_hash_table_length,
                                                 definitions);

        if(!pos->next)
        {
            cfl_free_typed_definition_list(definition_head.next);

            return 0;
        }

        pos = pos->next;

        pos->next = 0;

        definitions = definitions->next;
    }

    return definition_head.next;
}

cfl_typed_program* cfl_generate_typed_program(
        cfl_program* program,
        unsigned int equation_hash_table_length)
{
    cfl_reset_type_generator();

    cfl_type_equations equations;

    if(!cfl_initialize_type_equations(&equations, equation_hash_table_length))
        return false;

    cfl_type_hypothesis_chain hypothesis_head;
    hypothesis_head.next = 0;

    unsigned int global_hypotheses_added;
    cfl_hypothesis_load_list* load_list;

    if(!cfl_load_global_hypotheses(&global_hypotheses_added, &load_list,
                                   &hypothesis_head))
    {
        cfl_delete_type_equations(&equations);

        return 0;
    }

    cfl_typed_definition_list*  typed_definitions = 0;

    if(program->definitions)
    {
        typed_definitions =  cfl_generate_definition_types(
            &hypothesis_head, load_list, program->definitions,
            equation_hash_table_length);

        if(!typed_definitions)
        {
            cfl_free_hypothesis_load_list(load_list);
            cfl_free_program(program);
            cfl_delete_type_equations(&equations);

            return 0;
        }
    }

    if(!cfl_load_hypotheses_equations(&equations, load_list))
    {
        cfl_free_hypothesis_load_list(load_list);
        cfl_free_typed_definition_list(typed_definitions);
        cfl_free_program(program);
        cfl_delete_type_equations(&equations);

        return 0;
    }

    cfl_free_hypothesis_load_list(load_list);

    cfl_typed_node* typed_main = cfl_generate_typed_node(&equations,
                                                         &hypothesis_head,
                                                         typed_definitions,
                                                         program->main);

    int i = 0;
    for( ; i < global_hypotheses_added; ++i)
        cfl_pop_hypothesis(&hypothesis_head);

    if(!typed_main)
    {
        cfl_free_typed_definition_list(typed_definitions);
        cfl_free_program(program);
        cfl_delete_type_equations(&equations);

        return 0;
    }

    cfl_free_program(program);

    if(!cfl_close_type_equations(&equations))
    {
        cfl_free_typed_node(typed_main);
        cfl_free_typed_definition_list(typed_definitions);
        cfl_delete_type_equations(&equations);

        return 0;
    }

    if(!cfl_are_type_equations_consistent(&equations))
    {
        cfl_type_error_failure();

        cfl_free_typed_node(typed_main);
        cfl_free_typed_definition_list(typed_definitions);
        cfl_delete_type_equations(&equations);

        return 0;
    }

    if(!cfl_simplify_typed_node(&equations, typed_main))
    {
        cfl_free_typed_node(typed_main);
        cfl_free_typed_definition_list(typed_definitions);
        cfl_delete_type_equations(&equations);

        return 0;
    }

    cfl_typed_program* result = cfl_type_malloc(sizeof(cfl_typed_program));

    if(!result)
    {
        cfl_free_typed_node(typed_main);
        cfl_free_typed_definition_list(typed_definitions);
        cfl_delete_type_equations(&equations);

        return 0;
    }

    cfl_delete_type_equations(&equations);

    result->definitions = typed_definitions;
    result->main = typed_main;

    return result;
}
