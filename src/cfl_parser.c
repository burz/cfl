#include "cfl_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UPPER_CASE_A_CHAR 65
#define UPPER_CASE_Z_CHAR 90
#define LOWER_CASE_A_CHAR 97
#define LOWER_CASE_Z_CHAR 122
#define APOSTROPHE_CHAR 39
#define ZERO_CHAR 48
#define NINE_CHAR 57

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
    if(*(start++) != '(')
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

char* cfl_parse_variable(cfl_node* node, char* start, char* end)
{
    char buffer[MAX_IDENTIFIER_LENGTH];
    int length = 0;

    if((*start < UPPER_CASE_A_CHAR || *start > UPPER_CASE_Z_CHAR) &&
       (*start < LOWER_CASE_A_CHAR || *start > LOWER_CASE_Z_CHAR))
        return 0;

    while(length < MAX_IDENTIFIER_LENGTH &&
          ((*start >= UPPER_CASE_A_CHAR && *start <= UPPER_CASE_Z_CHAR) ||
          (*start >= LOWER_CASE_A_CHAR && *start <= LOWER_CASE_Z_CHAR) ||
          (*start >= ZERO_CHAR && *start <= NINE_CHAR)))
    {
        buffer[length] = *start;

        ++length;
        ++start;
    }

    while(length < MAX_IDENTIFIER_LENGTH && *start == APOSTROPHE_CHAR)
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
    char* op_pos = start;

    while(op_pos != end)
    {
        if(*op_pos == '&' && op_pos[1] == '&')
            break;

        ++op_pos;
    }

    if(op_pos == end)
        return 0;

    cfl_node* left = malloc(sizeof(cfl_node));

    if(!left)
    {
        fprintf(stderr, "ERROR: Could not allocate enough "
                        "space for a child node\n");

        return 0;
    }

    start = cfl_parse_molecule(left, start, op_pos);

    if(!start)
    {
        free(left);

        return 0;
    }

    start = cfl_parse_whitespace(start, op_pos);

    if(start != op_pos)
    {
        cfl_delete_node(left);
        free(left);

        return 0;
    }

    cfl_node* right = malloc(sizeof(cfl_node));

    if(!right)
    {
        fprintf(stderr, "ERROR: Could not allocate enough "
                        "space for a child node\n");

        cfl_delete_node(left);
        free(left);

        return 0;
    }

    start = cfl_parse_whitespace(op_pos + 2, end);
    start = cfl_parse_factor(right, start, end);

    if(!start)
    {
        cfl_delete_node(left);
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
    char* op_pos = start;

    while(op_pos != end)
    {
        if(*op_pos == '|' && op_pos[1] == '|')
            break;

        ++op_pos;
    }

    if(op_pos == end)
        return 0;

    cfl_node* left = malloc(sizeof(cfl_node));

    if(!left)
    {
        fprintf(stderr, "ERROR: Could not allocate enough "
                        "space for a child node\n");

        return 0;
    }

    start = cfl_parse_factor(left, start, op_pos);

    if(!start)
    {
        free(left);

        return 0;
    }

    start = cfl_parse_whitespace(start, op_pos);

    if(start != op_pos)
    {
        cfl_delete_node(left);
        free(left);

        return 0;
    }

    cfl_node* right = malloc(sizeof(cfl_node));

    if(!right)
    {
        fprintf(stderr, "ERROR: Could not allocate enough "
                        "space for a child node\n");

        cfl_delete_node(left);
        free(left);

        return 0;
    }

    start = cfl_parse_whitespace(op_pos + 2, end);
    start = cfl_parse_term(right, start, end);

    if(!start)
    {
        cfl_delete_node(left);
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

char* cfl_parse_let(cfl_node* node, char* start, char* end)
{
    if(end - start < 3 || start[0] != 'l' ||
       start[1] != 'e' || start[2] != 't')
        return 0;

    start = cfl_parse_whitespace(start + 3, end);

    cfl_node* name = malloc(sizeof(cfl_node));

    if(!name)
        return 0;

    start = cfl_parse_variable(name, start, end);

    if(!start)
    {
        free(name);

        return 0;
    }

    start = cfl_parse_whitespace(start, end);

    if(end - start < 1 || *start != '=')
    {
        cfl_delete_node(name);
        free(name);

        return 0;
    }

    start = cfl_parse_whitespace(++start, end);

    char* in_pos = start;
    int depth = 1;

    while(in_pos != end)
    {
        if(end - in_pos > 2 && in_pos[0] == 'l' &&
           in_pos[1] == 'e' && in_pos[2] == 't')
            ++depth;
        else if(end - in_pos > 1 && in_pos[0] == 'i' && in_pos[1] == 'n')
            if(--depth == 0)
                break;

        ++in_pos;
    }

    if(in_pos == end)
    {
        cfl_delete_node(name);
        free(name);

        return 0;
    }

    cfl_node* value = malloc(sizeof(cfl_node));

    if(!value)
    {
        cfl_delete_node(name);
        free(name);

        return 0;
    }

    start = cfl_parse_expression(value, start, in_pos);

    if(!start)
    {
        cfl_delete_node(name);
        free(name);
        free(value);

        return 0;
    }

    start = cfl_parse_whitespace(start, in_pos);

    if(start != in_pos)
    {
        cfl_delete_node(name);
        free(name);
        cfl_delete_node(value);
        free(value);

        return 0;
    }

    start = cfl_parse_whitespace(in_pos + 2, end);

    cfl_node* body = malloc(sizeof(cfl_node));

    if(!body)
    {
        cfl_delete_node(name);
        free(name);
        cfl_delete_node(value);
        free(value);

        return 0;
    }

    start = cfl_parse_expression(body, start, end);

    if(!start)
    {
        cfl_delete_node(name);
        free(name);
        cfl_delete_node(value);
        free(value);
        free(body);

        return 0;
    }

    cfl_node* function = malloc(sizeof(cfl_node));

    if(!function)
    {
        cfl_delete_node(name);
        free(name);
        cfl_delete_node(value);
        free(value);
        cfl_delete_node(body);
        free(body);

        return 0;
    }

    if(!cfl_create_node_function(function, name, body))
    {
        cfl_delete_node(name);
        free(name);
        cfl_delete_node(value);
        free(value);
        cfl_delete_node(body);
        free(body);
        free(function);

        return 0;
    }

    if(!cfl_create_node_application(node, function, value))
    {
        cfl_delete_node(name);
        free(name);
        cfl_delete_node(value);
        free(value);
        cfl_delete_node(body);
        free(body);
        cfl_delete_node(function);
        free(function);

        return 0;
    }

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
        result = cfl_parse_bool(node, start, end);

    if(!result)
        result = cfl_parse_variable(node, start, end);

    return result;
}

char* cfl_parse_molecule(cfl_node* node, char* start, char* end)
{
    char* result = cfl_parse_not(node, start, end);

    if(!result)
        result = cfl_parse_application(node, start, end);

    if(!result)
        result = cfl_parse_atom(node, start, end);

    return result;
}

char* cfl_parse_factor(cfl_node* node, char* start, char* end)
{
    char* result = cfl_parse_and(node, start, end);

    if(!result)
        result = cfl_parse_molecule(node, start, end);

    return result;
}

char* cfl_parse_term(cfl_node* node, char* start, char* end)
{
    char* result = cfl_parse_or(node, start, end);

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

        return 0;
    }

    return 1;
}
