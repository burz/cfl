#include "cfl_parser.h"

#include <stdio.h>
#include <string.h>

static int cfl_parse_error;

void* cfl_parser_malloc(size_t size)
{
    void* result = malloc(size);

    if(!result)
    {
        fprintf(stderr, "MEMORY ERROR: Ran out of memory while parsing\n");

        cfl_parse_error = 1;
    }

    return result;
}

void cfl_parse_error_unexpected_char(char x)
{
    fprintf(stderr, "PARSING ERROR: Unexpected char %c\n", x);

    cfl_parse_error = 1;
}

void cfl_parse_error_integer_overflow(char* start, int length)
{
    fprintf(stderr, "PARSING ERROR: The integer \"");

    char* pos = start;

    while(pos - start < length)
        fprintf(stderr, "%c", *pos);

    fprintf(stderr, "\" is too big. The integer string length maximum is "
            "%d characters\n", MAX_INTEGER_STRING_LENGTH);

    cfl_parse_error = 1;
}

void cfl_parse_error_expected(char* expected, char* after, char* start, char* end)
{
    if(cfl_parse_error)
        return;

    char buffer[100];
    int length = 0;

    while(start != end && length < 100 && !cfl_is_whitespace(*start))
    {
        buffer[length] = *start;

        ++start;
        ++length;
    }

    if(!length)
        fprintf(stderr, "PARSING ERROR: Expected %s after %s "
                        "but reached end of input context\n", expected, after);
    else
    {
        buffer[length] = 0;

        fprintf(stderr, "PARSING ERROR: Expected %s after %s "
                        "but encountered \"%s\"\n", expected, after, buffer);
    }

    cfl_parse_error = 1;
}

void cfl_parse_error_bad_division(void)
{
    if(cfl_parse_error)
        return;

    fprintf(stderr, "EVALUATION ERROR: Division by 0\n");

    cfl_parse_error = 1;
}

void cfl_parse_error_partial_program(void)
{
    if(cfl_parse_error)
        return;

    fprintf(stderr, "PARSING ERROR: Could not parse a full program. "
                    "Perhaps a \";\" is missing\n");

    cfl_parse_error = 1;
}

void cfl_parse_error_missing_main(void)
{
    if(cfl_parse_error)
        return;

    fprintf(stderr, "PARSING ERROR: The final definition in the "
                    "program is not the definition of \"main\"\n");

    cfl_parse_error = 1;
}

void cfl_parse_error_main_has_arguments(void)
{
    if(cfl_parse_error)
        return;

    fprintf(stderr, "PARSING ERROR: \"main\" cannot have any arguments\n");

    cfl_parse_error = 1;
}

void cfl_parse_error_unparseable_file(char* filename)
{
    if(cfl_parse_error)
        return;

    fprintf(stderr, "PARSING ERROR: Could not parse anything "
                    "in file %s\n", filename);

    cfl_parse_error = 1;
}

static int cfl_error_occured_while_parsing(void)
{
    return cfl_parse_error || cfl_get_ast_error_flag();
}

cfl_node* cfl_parse_atom(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(position == block)
        return 0;

    cfl_node* result = cfl_parse_parentheses(end, position, block);

    if(result || cfl_error_occured_while_parsing())
        return result;

    result = cfl_parse_not(end, position, block);

    if(result || cfl_error_occured_while_parsing())
        return result;

    result = cfl_parse_char(end, position, block);

    if(result || cfl_error_occured_while_parsing())
        return result;

    result = cfl_parse_variable(end, position, block);

    if(result || cfl_error_occured_while_parsing())
        return result;

    result = cfl_parse_bool(end, position, block);

    if(result || cfl_error_occured_while_parsing())
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

    cfl_node* result = cfl_parse_atom(end, position, block);

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

    if(result || cfl_error_occured_while_parsing())
        return result;

    result = cfl_parse_divide(end, position, block);

    if(result || cfl_error_occured_while_parsing())
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

    if(result || cfl_error_occured_while_parsing())
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

    if(result || cfl_error_occured_while_parsing())
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

    if(result || cfl_error_occured_while_parsing())
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

    if(result || cfl_error_occured_while_parsing())
        return result;

    result = cfl_parse_less(end, position, block);

    if(result || cfl_error_occured_while_parsing())
        return result;

    result = cfl_parse_term(end, position, block);

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

    if(result || cfl_error_occured_while_parsing())
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

    if(result || cfl_error_occured_while_parsing())
        return result;

    result = cfl_parse_boolean_factor(end, position, block);

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

    if(result || cfl_error_occured_while_parsing())
        return result;

    result = cfl_parse_boolean_term(end, position, block);

    return result;
}

cfl_node* cfl_parse_program(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(position == block)
        return 0;

    cfl_node* result = cfl_parse_expression(end, position, block);

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

    char* program = malloc(file_size + 1);

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

    cfl_parse_error = 0;
    cfl_reset_ast_error_flag();

    char* program_end = program + size;

    cfl_token_list head;
    head.next = 0;

    if(!cfl_generate_token_list(&head, program, program_end))
        return 0;

    cfl_print_token_list(head.next);

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
