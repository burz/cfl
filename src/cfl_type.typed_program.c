#include "cfl_type.typed_program.h"
#include "cfl_typed_program.h"
#include "cfl_type.h"

#include <string.h>

extern void* cfl_type_malloc(size_t size);
extern unsigned int cfl_type_get_next_id(void);
extern void cfl_reset_type_generator(void);

static unsigned int cfl_lookup_variable(
        cfl_type_equations* equations,
        cfl_type_hypothesis_chain* hypotheses,
        cfl_typed_definition_list* definitions,
        char* name)
{
    while(hypotheses)
    {
        if(!strcmp(name, hypotheses->name))
            return hypotheses->id;

        hypotheses = hypotheses->next;
    }

    return 0;
}

static bool cfl_push_hypothesis(
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

static void cfl_pop_hypothesis(cfl_type_hypothesis_chain* hypothesis_head)
{
    cfl_type_hypothesis_chain* temp = hypothesis_head->next;

    hypothesis_head->next = temp->next;

    free(temp);
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
        unsigned int id = cfl_lookup_variable(equations,
                                              hypothesis_head->next,
                                              definitions,
                                              node->data);

        if(!id)
        {
            cfl_type_error_undefined_variable(node->data);

            return 0;
        }

        cfl_type* type = cfl_create_new_type_variable(id);

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
        unsigned int id = cfl_type_get_next_id();

        if(!cfl_push_hypothesis(hypothesis_head, node->children[0]->data, id))
            return 0;

        cfl_typed_node* typed_result = cfl_generate_typed_node(
            equations, hypothesis_head, definitions, node->children[1]);

        cfl_pop_hypothesis(hypothesis_head);

        if(!typed_result)
            return 0;

        cfl_type* variable_type = cfl_create_new_type_variable(id);

        if(!variable_type)
        {
            cfl_free_typed_node(typed_result);

            return 0;
        }

        cfl_typed_node* typed_variable = cfl_create_typed_node(
            CFL_NODE_VARIABLE, variable_type, 0, node->children[0]->data, 0);

        free(node->children[0]);
        free(node->children[1]);
        free(node->children);

        node->number_of_children = 0;

        if(!typed_variable)
        {
            cfl_free_typed_node(typed_result);

            return 0;
        }

        variable_type = cfl_create_new_type_variable(id);

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
        cfl_typed_definition_list* definitions,
        unsigned int equation_hash_table_length,
        cfl_definition_list* definition)
{
    cfl_typed_definition_list* result = cfl_type_malloc(sizeof(cfl_typed_definition_list));

    if(!result)
        return 0;

    result->name = cfl_type_malloc(sizeof(char) * MAX_IDENTIFIER_LENGTH);

    if(!result->name)
    {
        free(result);

        return 0;
    }

    strcpy(result->name, definition->name->data);

    cfl_type_equations equations;
    cfl_initialize_type_equations(&equations, equation_hash_table_length);

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
        cfl_definition_list* definitions,
        unsigned int equation_hash_table_length)
{
    cfl_typed_definition_list definition_head;
    cfl_typed_definition_list* pos = &definition_head;

    while(definitions)
    {
        pos->next = cfl_generate_definition_type(hypothesis_head,
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

    cfl_typed_definition_list*  typed_definitions = 0;

    if(program->definitions)
    {
        cfl_typed_definition_list* typed_definitions =  cfl_generate_definition_types(
            &hypothesis_head, program->definitions, equation_hash_table_length);

        if(!typed_definitions)
        {
            cfl_free_program(program);
            cfl_delete_type_equations(&equations);

            return 0;
        }
    }

    cfl_typed_node* typed_main = cfl_generate_typed_node(&equations,
                                                         &hypothesis_head,
                                                         typed_definitions,
                                                         program->main);

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
