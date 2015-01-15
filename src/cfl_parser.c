#include "cfl_parser.h"

#include <stdio.h>
#include <string.h>

extern void cfl_reset_parser_error_flag(void);
extern void* cfl_parser_malloc(size_t size);

cfl_node* cfl_parse_atom(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(position == block)
        return 0;

    cfl_node* result = cfl_parse_parentheses(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_string(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_list(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_tuple(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_not(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_char(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_variable(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_bool(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_integer(end, position, block);

    return result;
}

cfl_node* cfl_parse_molecule(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(position == block)
        return 0;

    cfl_node* result = cfl_parse_application(end, position, block);

    return result;
}

cfl_node* cfl_parse_factor(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(position == block)
        return 0;

    cfl_node* result = cfl_parse_multiply(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_divide(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_mod(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_molecule(end, position, block);

    return result;
}

cfl_node* cfl_parse_term(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(position == block)
        return 0;

    cfl_node* result = cfl_parse_add(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_subtract(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_factor(end, position, block);

    return result;
}

cfl_node* cfl_parse_list_molecule(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(position == block)
        return 0;

    cfl_node* result = cfl_parse_push(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_term(end, position, block);

    return result;
}

cfl_node* cfl_parse_list_factor(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(position == block)
        return 0;

    cfl_node* result = cfl_parse_concatenate(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_list_molecule(end, position, block);

    return result;
}

cfl_node* cfl_parse_boolean_molecule(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(position == block)
        return 0;

    cfl_node* result = cfl_parse_equal(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_less(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_less_equal(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_greater(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_greater_equal(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_list_factor(end, position, block);

    return result;
}

cfl_node* cfl_parse_boolean_factor(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(position == block)
        return 0;

    cfl_node* result = cfl_parse_and(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_boolean_molecule(end, position, block);

    return result;
}

cfl_node* cfl_parse_boolean_term(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(position == block)
        return 0;

    cfl_node* result = cfl_parse_or(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_boolean_factor(end, position, block);

    return result;
}

cfl_node* cfl_parse_function_factor(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(position == block)
        return 0;

    cfl_node* result = cfl_parse_composition(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_boolean_term(end, position, block);

    return result;
}


cfl_node* cfl_parse_expression(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(position == block)
        return 0;

    cfl_node* result = cfl_parse_function(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_if(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_let(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_case(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_applicative(end, position, block);

    if(result || cfl_get_parse_error_flag())
        return result;

    result = cfl_parse_function_factor(end, position, block);

    return result;
}

cfl_node* cfl_parse_file(char* filename)
{
    FILE* f = fopen(filename, "rb");

    if(!f)
    {
        fprintf(stderr, "FILE ERROR: Could not open %s\n", filename);

        return 0;
    }

    fseek(f, 0, SEEK_END);

    int file_size = ftell(f);

    fseek(f, 0, SEEK_SET);

    char* program = cfl_parser_malloc(file_size + 1);

    if(!program)
    {
        fprintf(stderr, "MEMORY ERROR: Could not allocate "
                        "enough space to read in %s\n", filename);

        fclose(f);

        return 0;
    }

    size_t size = fread(program, 1, file_size, f);

    fclose(f);

    if(size < 1)
    {
        fprintf(stderr, "PARSING ERROR: The file %s was empty\n", filename);

        free(program);

        return 0;
    }

    program[size] = 0;

    cfl_reset_parser_error_flag();
    cfl_reset_ast_error_flag();

    char* program_end = program + size;

    cfl_token_list head;
    head.next = 0;

    if(!cfl_generate_token_list(&head, program, program_end))
    {
        free(program);

        return 0;
    }

    cfl_token_list* ending_position;

    cfl_node* result = cfl_parse_program(&ending_position, head.next, 0);

    cfl_delete_token_list(head.next);

    free(program);

    if(!result)
    {
        cfl_parse_error_unparseable_file(filename);

        return 0;
    }

    return result;
}
