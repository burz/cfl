#include "cfl_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UPPER_CASE_A_CHAR 65
#define UPPER_CASE_Z_CHAR 90
#define LOWER_CASE_A_CHAR 97
#define LOWER_CASE_Z_CHAR 122
#define ZERO_CHAR 48
#define NINE_CHAR 57
#define MAX_INTEGER_STRING_LENGTH 11

char* cfl_parse_whitespace(char* start, char* end)
{
    char* result = start;

    while(start != end)
    {
        if(*start == ' ' || *start == '\n' || *start == '\t' || *start == '\r')
            result = ++start;
        else
            return result;
    }

    return result;
}

char* cfl_parse_parentheses(
        cfl_node* node,
        cfl_node_parser parser,
        char* start,
        char* end)
{
    if(start == end || *(start++) != '(')
        return 0;

    int depth = 1;
    char* end_pos = start;

    while(end_pos != end)
    {
        if(*end_pos == ')')
        {
            if(--depth == 0)
                break;
        }
        else if(*end_pos == '(')
            ++depth;

        ++end_pos;
    }

    if(end_pos == end)
        return 0;

    start = cfl_parse_whitespace(start, end_pos);
    start = (*parser)(node, start, end_pos);

    if(!start)
        return 0;

    start = cfl_parse_whitespace(start, end_pos);

    return start == end_pos ? end_pos + 1 : 0;
}

char* cfl_parse_binary_operation(
        cfl_node* left,
        cfl_node* right,
        cfl_node_parser left_parser,
        cfl_node_parser right_parser,
        int operand_length,
        char* operand,
        char* start,
        char* end)
{
    char* op_pos = start;

    while(end - op_pos > operand_length)
    {
        int found = 1;
        int i = 0;

        for( ; i < operand_length; ++i)
            if(op_pos[i] != operand[i])
            {
                found = 0;

                break;
            }

        if(found)
            break;

        ++op_pos;
    }

    if(end - op_pos <= operand_length)
        return 0;

    start = (*left_parser)(left, start, op_pos);

    if(!start)
        return 0;

    start = cfl_parse_whitespace(start, op_pos);

    if(start != op_pos)
    {
        cfl_delete_node(left);

        return 0;
    }

    start = cfl_parse_whitespace(op_pos + operand_length, end);

    start = (*right_parser)(right, start, end);

    if(!start)
    {
        cfl_delete_node(left);

        return 0;
    }

    return start;
}

char* cfl_parse_variable(cfl_node* node, char* start, char* end)
{
    if(start == end)
        return 0;

    char buffer[MAX_IDENTIFIER_LENGTH];
    int length = 0;

    if((*start < UPPER_CASE_A_CHAR || *start > UPPER_CASE_Z_CHAR) &&
       (*start < LOWER_CASE_A_CHAR || *start > LOWER_CASE_Z_CHAR))
        return 0;

    while(start != end && length < MAX_IDENTIFIER_LENGTH &&
          ((*start >= UPPER_CASE_A_CHAR && *start <= UPPER_CASE_Z_CHAR) ||
           (*start >= LOWER_CASE_A_CHAR && *start <= LOWER_CASE_Z_CHAR) ||
           (*start >= ZERO_CHAR && *start <= NINE_CHAR) || *start == '_'))
    {
        buffer[length] = *start;

        ++length;
        ++start;
    }

    while(start != end && length < MAX_IDENTIFIER_LENGTH && *start == '\'')
    {
        buffer[length] = '\'';

        ++length;
        ++start;
    }

    buffer[length] = 0;

    int i = 0;

    for( ; i < NUMBER_OF_RESERVED_WORDS; ++i)
    {
        if(!strcmp(buffer, reserved_words[i]))
            return 0;
    }

    if(!cfl_create_node_variable(node, buffer))
        return 0;

    return start;
}

char* cfl_parse_bool(cfl_node* node, char* start, char* end)
{
    if(end - start > 3 && start[0] == 't' && start[1] == 'r' &&
       start[2] == 'u' && start[3] == 'e')
    {
        if(!cfl_create_node_bool(node, true))
            return 0;

        return start + 4;
    }
    else if(end - start > 4 && start[0] == 'f' &&
            start[1] == 'a' && start[2] == 'l' &&
            start[3] == 's' && start[4] == 'e')
    {
        if(!cfl_create_node_bool(node, false))
            return 0;

        return start + 5;
    }

    return 0;
}

char* cfl_parse_integer(cfl_node* node, char* start, char* end)
{
    if(start == end)
        return 0;

    int negate = 0;

    if(*start == '-')
    {
        negate = 1;

        start = cfl_parse_whitespace(start + 1, end);
    }

    int length = 0;
    char buffer[MAX_INTEGER_STRING_LENGTH];

    while(start != end && length < MAX_INTEGER_STRING_LENGTH &&
          *start >= ZERO_CHAR && *start <= NINE_CHAR)
    {
        buffer[length] = *start;

        ++start;
        ++length;
    }

    if(!length)
        return 0;

    buffer[length] = 0;

    int value = atoi(buffer);

    if(negate)
        value = -value;

    if(!cfl_create_node_integer(node, value))
        return 0;

    return start;
}

char* cfl_parse_function(cfl_node* node, char* start, char* end)
{
    if(end - start < 8 || start[0] != 'f' || start[1] != 'u' ||
       start[2] != 'n' || start[3] != 'c' || start[4] != 't' ||
       start[5] != 'i' || start[6] != 'o' || start[7] != 'n')
        return 0;

    start = cfl_parse_whitespace(start + 8, end);

    cfl_node* argument = malloc(sizeof(cfl_node));

    if(!argument)
        return 0;

    start = cfl_parse_variable(argument, start, end);

    if(!start)
    {
        free(argument);

        return 0;
    }

    start = cfl_parse_whitespace(start, end);

    if(end - start < 2 || start[0] != '-' || start[1] != '>')
    {
        cfl_delete_node(argument);
        free(argument);

        return 0;
    }

    start = cfl_parse_whitespace(start + 2, end);

    cfl_node* body = malloc(sizeof(cfl_node));

    if(!body)
    {
        cfl_delete_node(argument);
        free(argument);

        return 0;
    }

    start = cfl_parse_expression(body, start, end);

    if(!start)
    {
        cfl_delete_node(argument);
        free(argument);
        free(body);

        return 0;
    }

    if(!cfl_create_node_function(node, argument, body))
    {
        cfl_delete_node(argument);
        free(argument);
        cfl_delete_node(body);
        free(body);

        return 0;
    }

    return start;
}

char* cfl_parse_and(cfl_node* node, char* start, char* end)
{
    cfl_node* left = malloc(sizeof(cfl_node));

    if(!left)
        return 0;

    cfl_node* right = malloc(sizeof(cfl_node));

    if(!right)
    {
        free(left);

        return 0;
    }

    start = cfl_parse_binary_operation(left,
                                       right,
                                       &cfl_parse_molecule,
                                       &cfl_parse_factor,
                                       2,
                                       "&&",
                                       start,
                                       end);

    if(!start)
    {
        free(left);
        free(right);

        return 0;
    }

    if(!cfl_create_node_and(node, left, right))
    {
        cfl_delete_node(left);
        free(left);
        cfl_delete_node(right);
        free(right);

        return 0;
    }

    return start;
}

char* cfl_parse_or(cfl_node* node, char* start, char* end)
{
    cfl_node* left = malloc(sizeof(cfl_node));

    if(!left)
        return 0;

    cfl_node* right = malloc(sizeof(cfl_node));

    if(!right)
    {
        free(left);

        return 0;
    }

    start = cfl_parse_binary_operation(left,
                                       right,
                                       &cfl_parse_factor,
                                       &cfl_parse_term,
                                       2,
                                       "||",
                                       start,
                                       end);

    if(!start)
    {
        free(left);
        free(right);

        return 0;
    }

    if(!cfl_create_node_or(node, left, right))
    {
        cfl_delete_node(left);
        free(left);
        cfl_delete_node(right);
        free(right);

        return 0;
    }

    return start;
}

char* cfl_parse_not(cfl_node* node, char* start, char* end)
{
    if(*(start++) != '!')
        return 0;

    cfl_node* child_node = malloc(sizeof(cfl_node));

    if(!child_node)
    {
        fprintf(stderr, "ERROR: Could not allocate enough "
                        "space for a child node\n");

        return 0;
    }

    start = cfl_parse_atom(child_node, start, end);

    if(!start)
    {
        free(child_node);

        return 0;
    }

    if(!cfl_create_node_not(node, child_node))
    {
        cfl_delete_node(child_node);

        free(child_node);

        return 0;
    }

    return start;
}

char* cfl_parse_add(cfl_node* node, char* start, char* end)
{
    cfl_node* left = malloc(sizeof(cfl_node));

    if(!left)
        return 0;

    cfl_node* right = malloc(sizeof(cfl_node));

    if(!right)
    {
        free(left);

        return 0;
    }

    start = cfl_parse_binary_operation(left,
                                       right,
                                       &cfl_parse_molecule,
                                       &cfl_parse_factor,
                                       1,
                                       "+",
                                       start,
                                       end);

    if(!start)
    {
        free(left);
        free(right);

        return 0;
    }

    if(!cfl_create_node_add(node, left, right))
    {
        cfl_delete_node(left);
        free(left);
        cfl_delete_node(right);
        free(right);

        return 0;
    }

    return start;
}

char* cfl_parse_multiply(cfl_node* node, char* start, char* end)
{
    cfl_node* left = malloc(sizeof(cfl_node));

    if(!left)
        return 0;

    cfl_node* right = malloc(sizeof(cfl_node));

    if(!right)
    {
        free(left);

        return 0;
    }

    start = cfl_parse_binary_operation(left,
                                       right,
                                       &cfl_parse_factor,
                                       &cfl_parse_term,
                                       1,
                                       "*",
                                       start,
                                       end);

    if(!start)
    {
        free(left);
        free(right);

        return 0;
    }

    if(!cfl_create_node_multiply(node, left, right))
    {
        cfl_delete_node(left);
        free(left);
        cfl_delete_node(right);
        free(right);

        return 0;
    }

    return start;
}

char* cfl_parse_divide(cfl_node* node, char* start, char* end)
{
    cfl_node* left = malloc(sizeof(cfl_node));

    if(!left)
        return 0;

    cfl_node* right = malloc(sizeof(cfl_node));

    if(!right)
    {
        free(left);

        return 0;
    }

    start = cfl_parse_binary_operation(left,
                                       right,
                                       &cfl_parse_factor,
                                       &cfl_parse_term,
                                       1,
                                       "/",
                                       start,
                                       end);

    if(!start)
    {
        free(left);
        free(right);

        return 0;
    }

    if(!cfl_create_node_divide(node, left, right))
    {
        cfl_delete_node(left);
        free(left);
        cfl_delete_node(right);
        free(right);

        return 0;
    }

    return start;
}

char* cfl_parse_equal(cfl_node* node, char* start, char* end)
{
    cfl_node* left = malloc(sizeof(cfl_node));

    if(!left)
        return 0;

    cfl_node* right = malloc(sizeof(cfl_node));

    if(!right)
    {
        free(left);

        return 0;
    }

    start = cfl_parse_binary_operation(left,
                                       right,
                                       &cfl_parse_molecule,
                                       &cfl_parse_molecule,
                                       2,
                                       "==",
                                       start,
                                       end);

    if(!start)
    {
        free(left);
        free(right);

        return 0;
    }

    if(!cfl_create_node_equal(node, left, right))
    {
        cfl_delete_node(left);
        free(left);
        cfl_delete_node(right);
        free(right);

        return 0;
    }

    return start;
}

char* cfl_parse_less(cfl_node* node, char* start, char* end)
{
    cfl_node* left = malloc(sizeof(cfl_node));

    if(!left)
        return 0;

    cfl_node* right = malloc(sizeof(cfl_node));

    if(!right)
    {
        free(left);

        return 0;
    }

    start = cfl_parse_binary_operation(left,
                                       right,
                                       &cfl_parse_molecule,
                                       &cfl_parse_molecule,
                                       1,
                                       "<",
                                       start,
                                       end);

    if(!start)
    {
        free(left);
        free(right);

        return 0;
    }

    if(!cfl_create_node_less(node, left, right))
    {
        cfl_delete_node(left);
        free(left);
        cfl_delete_node(right);
        free(right);

        return 0;
    }

    return start;
}

char* cfl_parse_application(cfl_node* node, char* start, char* end)
{
    cfl_node* function = malloc(sizeof(cfl_node));

    if(!function)
        return 0;

    start = cfl_parse_atom(function, start, end);

    if(!start)
    {
        free(function);

        return 0;
    }

    int argument_count = 0;

    while(start != end)
    {
        char* pos = cfl_parse_whitespace(start, end);

        cfl_node* argument = malloc(sizeof(cfl_node));

        if(!argument)
        {
            cfl_delete_node(function);
            free(function);

            return 0;
        }

        pos = cfl_parse_atom(argument, pos, end);

        if(!pos)
        {
            free(argument);

            break;
        }

        start = pos;

        cfl_node* new_function = malloc(sizeof(cfl_node));

        if(!new_function)
        {
            cfl_delete_node(function);
            free(function);
            cfl_delete_node(argument);
            free(argument);

            return 0;
        }

        if(!cfl_create_node_application(new_function, function, argument))
        {
            cfl_delete_node(function);
            free(function);
            cfl_delete_node(argument);
            free(argument);
            free(new_function);

            return 0;
        }

        function = new_function;

        ++argument_count;
    }

    if(!argument_count)
    {
        cfl_delete_node(function);
        free(function);

        return 0;
    }

    *node = *function;

    free(function);

    return start;
}

char* cfl_parse_if(cfl_node* node, char* start, char* end)
{
    if(end - start < 2 || start[0] != 'i' || start[1] != 'f')
        return 0;

    start += 2;

    if(start == end)
        return 0;

    start = cfl_parse_whitespace(start, end);

    char* pos = start;
    int depth = 1;

    while(pos != end)
    {
        if(end - pos > 1 && pos[0] == 'i' && pos[1] == 'f')
            ++depth;
        else if(end - pos > 3 && pos[0] == 'e' && pos[1] == 'l' &&
                pos[2] == 's' && pos[3] == 'e')
            --depth;
        else if(end - pos > 3 && depth == 1 &&
                pos[0] == 't' && pos[1] == 'h' &&
                pos[2] == 'e' && pos[3] == 'n')
            break;

        ++pos;
    }

    if(pos == end || depth != 1)
        return 0;

    cfl_node* condition = malloc(sizeof(cfl_node));

    if(!condition)
    {
        fprintf(stderr, "ERROR: Could not allocate enough "
                        "space for a child node\n");

        return 0;
    }

    start = cfl_parse_expression(condition, start, pos);

    if(!start)
    {
        free(condition);

        return 0;
    }

    start = cfl_parse_whitespace(start, pos);

    if(start != pos)
    {
        cfl_delete_node(condition);
        free(condition);

        return 0;
    }

    pos += 4;
    start = pos;

    while(pos != end)
    {
        if(end - pos > 1 && pos[0] == 'i' && pos[1] == 'f')
            ++depth;
        else if(end - pos > 3 && pos[0] == 'e' && pos[1] == 'l' &&
                pos[2] == 's' && pos[3] == 'e')
        {
            --depth;

            if(depth == 0)
                break;
        }

        ++pos;
    }

    if(pos == end || depth != 0)
    {
        cfl_delete_node(condition);
        free(condition);

        return 0;
    }

    start = cfl_parse_whitespace(start, pos);

    cfl_node* then_node = malloc(sizeof(cfl_node));

    if(!then_node)
    {
        cfl_delete_node(condition);
        free(condition);

        return 0;
    }

    start = cfl_parse_expression(then_node, start, pos);

    if(!start)
    {
        cfl_delete_node(condition);
        free(condition);
        free(then_node);

        return 0;
    }

    start = cfl_parse_whitespace(start, pos);

    if(start != pos)
    {
        cfl_delete_node(condition);
        free(condition);
        cfl_delete_node(then_node);
        free(then_node);

        return 0;
    }

    start += 4;

    start = cfl_parse_whitespace(start, end);

    cfl_node* else_node = malloc(sizeof(cfl_node));

    if(!else_node)
    {
        cfl_delete_node(condition);
        free(condition);
        cfl_delete_node(then_node);
        free(then_node);

        return 0;
    }

    start = cfl_parse_expression(else_node, start, end);

    if(!start)
    {
        cfl_delete_node(condition);
        free(condition);
        cfl_delete_node(then_node);
        free(then_node);
        free(else_node);

        return 0;
    }

    if(!cfl_create_node_if(node, condition, then_node, else_node))
    {
        cfl_delete_node(condition);
        free(condition);
        cfl_delete_node(then_node);
        free(then_node);
        cfl_delete_node(else_node);
        free(else_node);

        return 0;
    }

    return start;
}

char* cfl_parse_atom(cfl_node* node, char* start, char* end)
{
    char* result = cfl_parse_parentheses(node, &cfl_parse_expression, start, end);

    if(!result)
        result = cfl_parse_not(node, start, end);

    if(!result)
        result = cfl_parse_bool(node, start, end);

    if(!result)
        result = cfl_parse_integer(node, start, end);

    if(!result)
        result = cfl_parse_variable(node, start, end);

    return result;
}

char* cfl_parse_molecule(cfl_node* node, char* start, char* end)
{
    char* result = cfl_parse_application(node, start, end);

    if(!result)
        result = cfl_parse_atom(node, start, end);

    return result;
}

char* cfl_parse_factor(cfl_node* node, char* start, char* end)
{
    char* result = cfl_parse_and(node, start, end);

    if(!result)
        result = cfl_parse_multiply(node, start, end);

    if(!result)
        result = cfl_parse_divide(node, start, end);

    if(!result)
        result = cfl_parse_mod(node, start, end);

    if(!result)
        result = cfl_parse_equal(node, start, end);

    if(!result)
        result = cfl_parse_less(node, start, end);

    if(!result)
        result = cfl_parse_molecule(node, start, end);

    return result;
}

char* cfl_parse_term(cfl_node* node, char* start, char* end)
{
    char* result = cfl_parse_or(node, start, end);

    if(!result)
        result = cfl_parse_add(node, start, end);

    if(!result)
        result = cfl_parse_subtract(node, start, end);

    if(!result)
        result = cfl_parse_factor(node, start, end);

    return result;
}

char* cfl_parse_expression(cfl_node* node, char* start, char* end)
{
    char* result = cfl_parse_if(node, start, end);

    if(!result)
        result = cfl_parse_let(node, start, end);

    if(!result)
        result = cfl_parse_function(node, start, end);

    if(!result)
        result = cfl_parse_term(node, start, end);

    return result;
}

int cfl_parse_file(cfl_node* node, char* filename)
{
    FILE* f = fopen(filename, "rb");

    if(!f)
    {
        fprintf(stderr, "ERROR: Could not open %s\n", filename);

        return 0;
    }

    fseek(f, 0, SEEK_END);

    int file_size = ftell(f);

    fseek(f, 0, SEEK_SET);

    char* program = malloc(file_size + 1);

    if(!program)
    {
        fprintf(stderr, "ERROR: Could not allocate enough space "
                        "to read in %s\n", filename);

        fclose(f);

        return 0;
    }

    size_t size = fread(program, 1, file_size, f);

    fclose(f);

    if(size < 1)
    {
        fprintf(stderr, "ERROR: The file %s was empty\n", filename);

        free(program);

        return 0;
    }

    program[size] = 0;

    char* end = program + size;
    char* pos = cfl_parse_whitespace(program, end);

    pos = cfl_parse_expression(node, pos, end);

    if(!pos)
    {
        fprintf(stderr, "ERROR: Could not parse a term in file %s\n", filename);

        free(program);

        return 0;
    }

    pos = cfl_parse_whitespace(pos, end);

    free(program);

    if(pos != end)
    {
        fprintf(stderr, "ERROR: Could not parse the entire file %s\n", filename);

        cfl_delete_node(node);

        return 0;
    }

    return 1;
}
