#include "cfl_type.h"

#include <stdio.h>

extern void* cfl_type_malloc(size_t size);
extern void cfl_reset_type_generator(void);

void cfl_create_type_variable(cfl_type* node, unsigned int id)
{
    node->type = CFL_TYPE_VARIABLE;
    node->id = id;
    node->input = 0;
    node->output = 0;
}

void cfl_create_type_bool(cfl_type* node)
{
    node->type = CFL_TYPE_BOOL;
    node->id = 0;
    node->input = 0;
    node->output = 0;
}

void cfl_create_type_integer(cfl_type* node)
{
    node->type = CFL_TYPE_INTEGER;
    node->id = 0;
    node->input = 0;
    node->output = 0;
}

void cfl_create_type_char(cfl_type* node)
{
    node->type = CFL_TYPE_CHAR;
    node->id = 0;
    node->input = 0;
    node->output = 0;
}

void cfl_create_type_list(cfl_type* node, cfl_type* content)
{
    node->type = CFL_TYPE_LIST;
    node->id = 0;
    node->input = content;
    node->output = 0;
}

void cfl_create_type_tuple(
        cfl_type* node,
        unsigned int number_of_children,
        cfl_type** children)
{
    node->type = CFL_TYPE_TUPLE;
    node->id = number_of_children;
    node->input = children;
    node->output = 0;
}

void cfl_create_type_arrow(cfl_type* node, cfl_type* input, cfl_type* output)
{
    node->type = CFL_TYPE_ARROW;
    node->id = 0;
    node->input = input;
    node->output = output;
}

int cfl_compare_type(cfl_type* left, cfl_type* right)
{
    if(left->type != right->type)
    {
        if(left->type > right->type)
            return -1;
        else
            return 1;
    }

    if(left->type == CFL_TYPE_VARIABLE)
    {
        if(left->id > right->id)
            return -1;
        else if(left->id == right->id)
            return 0;
        else
            return 1;
    }
    else if(left->type == CFL_TYPE_LIST)
        return cfl_compare_type(left->input, right->input);
    else if(left->type == CFL_TYPE_ARROW)
    {
        int result = cfl_compare_type(left->input, right->input);

        if(result)
            return result;

        return cfl_compare_type(left->output, right->output);
    }
    else if(left->type == CFL_TYPE_TUPLE)
    {
        if(left->id != right->id)
            return 1;

        int i = 0;
        for( ; i < left->id; ++i)
        {
            int result = cfl_compare_type(((cfl_type**) left->input)[i],
                                          ((cfl_type**) right->input)[i]);

            if(result)
                return result;
        }
    }

    return 0;
}

int cfl_copy_type(cfl_type* target, cfl_type* node)
{
    void* input;
    void* output;

    switch(node->type)
    {
        case CFL_TYPE_VARIABLE:
            cfl_create_type_variable(target, node->id);
            break;
        case CFL_TYPE_BOOL:
            cfl_create_type_bool(target);
            break;
        case CFL_TYPE_INTEGER:
            cfl_create_type_integer(target);
            break;
        case CFL_TYPE_CHAR:
            cfl_create_type_char(target);
            break;
        case CFL_TYPE_LIST:
            input = cfl_type_malloc(sizeof(cfl_type));

            if(!input)
                return 0;

            if(!cfl_copy_type(input, node->input))
            {
                free(input);

                return 0;
            }

            cfl_create_type_list(target, input);

            break;
        case CFL_TYPE_TUPLE:
            if(node->id)
            {
                input = cfl_type_malloc(sizeof(cfl_type*) * node->id);

                int i = 0;
                for( ; i < node->id; ++i)
                {
                    ((cfl_type**) input)[i] = cfl_type_malloc(sizeof(cfl_type));

                    if(!((cfl_type**) input)[i])
                    {
                        int j = 0;
                        for( ; j < i; ++j)
                            cfl_free_type(((cfl_type**) input)[j]);

                        free(input);
                    }

                    if(!cfl_copy_type(((cfl_type**) input)[i],
                                      ((cfl_type**) node->input)[i]))
                    {
                        int j = 0;
                        for( ; j < i; ++j)
                            cfl_free_type(((cfl_type**) input)[j]);

                        free(((cfl_type**) input)[i]);
                        free(input);
                    }
                }
            }

            cfl_create_type_tuple(target, node->id, input);

            break;
        case CFL_TYPE_ARROW:
            input = cfl_type_malloc(sizeof(cfl_type));

            if(!input)
                return 0;

            output = cfl_type_malloc(sizeof(cfl_type));

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
                cfl_free_type(input);
                free(output);

                return 0;
            }

            cfl_create_type_arrow(target, input, output);

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
        if(node->type == CFL_TYPE_TUPLE)
        {
            if(node->id)
            {
                int i = 0;
                for( ; i < node->id; ++i)
                    cfl_free_type(((cfl_type**) node->input)[i]);

                free(node->input);
            }
        }
        else
            cfl_free_type(node->input);
    }

    if(node->output)
        cfl_free_type(node->output);
}

void cfl_free_type(cfl_type* node)
{
    cfl_delete_type(node);

    free(node);
}

static void cfl_print_type_inner(cfl_type* node)
{
    int i;

    switch(node->type)
    {
        case CFL_TYPE_VARIABLE:
            printf("a%u", node->id);
            break;
        case CFL_TYPE_BOOL:
            printf("Bool");
            break;
        case CFL_TYPE_INTEGER:
            printf("Integer");
            break;
        case CFL_TYPE_CHAR:
            printf("Char");
            break;
        case CFL_TYPE_LIST:
            printf("[");
            cfl_print_type_inner(node->input);
            printf("]");
            break;
        case CFL_TYPE_ARROW:
            printf("(");
            cfl_print_type_inner(node->input);
            printf(" -> ");
            cfl_print_type_inner(node->output);
            printf(")");
            break;
        case CFL_TYPE_TUPLE:
            printf("(");
            for(i = 0; i < node->id; ++i)
            {
                cfl_print_type_inner(((cfl_type**) node->input)[i]);

                if(i < node->id - 1)
                    printf(", ");
            }
            printf(")");
        default:
            break;
    }
}

void cfl_print_type(cfl_type* node)
{
    cfl_print_type_inner(node);

    printf("\n");
}

bool cfl_typecheck(
        cfl_program* program,
        unsigned int equation_hash_table_length)
{
    cfl_reset_type_error_flag();
    cfl_reset_type_generator();

    cfl_type_equations equations;

    if(!cfl_initialize_type_equations(&equations, equation_hash_table_length))
        return false;

    cfl_type_hypothesis_chain hypothesis_head;
    hypothesis_head.next = 0;

    unsigned int definition_hypothesis_count = 0;

    if(program->definitions)
    {
        definition_hypothesis_count = cfl_setup_definitions(&equations,
                                                            &hypothesis_head,
                                                            program->definitions);

        if(!definition_hypothesis_count)
        {
            cfl_delete_type_equations(&equations);

            return false;
        }
    }

    cfl_type* result = cfl_generate_type_equation_chain(&equations,
                                                        &hypothesis_head,
                                                        program->main);

    cfl_remove_n_hypotheses(&hypothesis_head, definition_hypothesis_count);

    if(!result)
    {
        cfl_delete_type_equations(&equations);

        return false;
    }

    if(!cfl_close_type_equations(&equations))
    {
        cfl_delete_type_equations(&equations);
        cfl_free_type(result);

        return false;
    }

    if(!cfl_are_type_equations_consistent(&equations))
    {
        cfl_type_error_failure();

        cfl_delete_type_equations(&equations);
        cfl_free_type(result);

        return false;
    }

    program->type = result;

    cfl_delete_type_equations(&equations);

    return true;
}
