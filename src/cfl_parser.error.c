#include "cfl_parser.h"

#include <stdio.h>
#include <stdlib.h>

static bool cfl_parse_error;

void cfl_reset_parser_error_flag(void)
{
    cfl_parse_error = false;
}

void* cfl_parser_malloc(size_t size)
{
    void* result = malloc(size);

    if(!result)
    {
        fprintf(stderr, "MEMORY ERROR: Ran out of memory while parsing\n");

        cfl_parse_error = true;
    }

    return result;
}

void cfl_parse_error_unexpected_char(char x)
{
    fprintf(stderr, "PARSING ERROR: Unexpected char %c\n", x);

    cfl_parse_error = true;
}

void cfl_parse_error_integer_overflow(char* start, int length)
{
    fprintf(stderr, "PARSING ERROR: The integer \"");

    char* pos = start;

    while(pos - start < length)
        fprintf(stderr, "%c", *pos);

    fprintf(stderr, "\" is too big. The integer string length maximum is "
            "%d characters\n", MAX_INTEGER_STRING_LENGTH);

    cfl_parse_error = true;
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

    cfl_parse_error = true;
}

void cfl_parse_error_no_equal_after_def(char* name)
{
    if(cfl_parse_error)
        return;

    fprintf(stderr, "PARSING ERROR: There is no \"=\" after the definition "
                    "of \"%s\"\n", name);

    cfl_parse_error = true;
}

void cfl_parse_error_bad_arguments_after_def(char* name)
{
    if(cfl_parse_error)
        return;

    fprintf(stderr, "PARSING ERROR: Could not parse the arguments following "
                    "the definition of \"%s\"", name);

    cfl_parse_error = true;
}

void cfl_parse_error_bad_division(void)
{
    if(cfl_parse_error)
        return;

    fprintf(stderr, "EVALUATION ERROR: Division by 0\n");

    cfl_parse_error = true;
}

void cfl_parse_error_partial_program(void)
{
    if(cfl_parse_error)
        return;

    fprintf(stderr, "PARSING ERROR: Could not parse a full program. "
                    "Perhaps a \";\" is missing\n");

    cfl_parse_error = true;
}

void cfl_parse_error_missing_main(void)
{
    if(cfl_parse_error)
        return;

    fprintf(stderr, "PARSING ERROR: The final definition in the "
                    "program is not the definition of \"main\"\n");

    cfl_parse_error = true;
}

void cfl_parse_error_main_has_arguments(void)
{
    if(cfl_parse_error)
        return;

    fprintf(stderr, "PARSING ERROR: \"main\" cannot have any arguments\n");

    cfl_parse_error = true;
}

void cfl_parse_error_unparseable_file(char* filename)
{
    if(cfl_parse_error)
        return;

    fprintf(stderr, "PARSING ERROR: Could not parse anything "
                    "in file %s\n", filename);

    cfl_parse_error = true;
}

bool cfl_error_occured_while_parsing(void)
{
    return cfl_parse_error || cfl_get_ast_error_flag();
}
