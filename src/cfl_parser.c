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

void cfl_parse_error_expected(char* expected, char* after, char* start, char* end)
{
    if(cfl_parse_error)
        return;

    char buffer[100];
    int length = 0;

    while(start != end && !cfl_is_whitespace(*start))
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

char* cfl_parse_atom(cfl_node* node, char* start, char* end)
{
    char* result = cfl_parse_parentheses(node, &cfl_parse_expression, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_not(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_bool(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_integer(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_variable(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_list(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_tuple(node, start, end);

    return result;
}

char* cfl_parse_molecule(cfl_node* node, char* start, char* end)
{
    char* result = cfl_parse_application(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_atom(node, start, end);

    return result;
}

char* cfl_parse_factor(cfl_node* node, char* start, char* end)
{
    char* result = cfl_parse_and(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_multiply(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_divide(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_mod(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_equal(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_less(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_less_equal(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_greater(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_greater_equal(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_molecule(node, start, end);

    return result;
}

char* cfl_parse_term(cfl_node* node, char* start, char* end)
{
    char* result = cfl_parse_or(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_add(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_subtract(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_factor(node, start, end);

    return result;
}

char* cfl_parse_list_expression(cfl_node* node, char* start, char* end)
{
    char* result = cfl_parse_push(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_term(node, start, end);

    return result;
}

char* cfl_parse_expression(cfl_node* node, char* start, char* end)
{
    char* result = cfl_parse_if(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_let(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_function(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_case(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_concatenate(node, start, end);

    if(!result && !cfl_error_occured_while_parsing())
        result = cfl_parse_list_expression(node, start, end);

    return result;
}

int cfl_parse_file(cfl_node* node, char* filename)
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

    char* end = program + size;
    char* pos = cfl_parse_whitespace(program, end);

    pos = cfl_parse_program(node, pos, end);

    if(!pos)
    {
        cfl_parse_error_unparseable_file(filename);

        free(program);

        return 0;
    }

    free(program);

    return 1;
}
