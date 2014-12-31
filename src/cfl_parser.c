#include "cfl_parser.h"

#include <stdio.h>
#include <stdlib.h>

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

char* cfl_parse_bool(cfl_node* node, char* start, char* end)
{
    if(end - start > 3 && start[0] == 't' && start[1] == 'r' &&
       start[2] == 'u' && start[3] == 'e')
    {
        if(!cfl_create_node_bool(node, true))
        {
            fprintf(stderr, "ERROR: Could not allocate enough "
                            "space for a boolean node\n");

            return 0;
        }

        return start + 4;
    }
    else if(end - start > 4 && start[0] == 'f' &&
            start[1] == 'a' && start[2] == 'l' &&
            start[3] == 's' && start[4] == 'e')
    {
        if(!cfl_create_node_bool(node, false))
        {
            fprintf(stderr, "ERROR: Could not allocate enough "
                            "space for a boolean node\n");

            return 0;
        }

        return start + 5;
    }

    return 0;
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

    start = cfl_parse_atom(left, start, op_pos);

    if(!start)
    {
        free(left);

        return 0;
    }

    start = cfl_parse_whitespace(start, op_pos);

    if(start != op_pos)
    {
        free(left);

        return 0;
    }

    cfl_node* right = malloc(sizeof(cfl_node));

    if(!right)
    {
        fprintf(stderr, "ERROR: Could not allocate enough "
                        "space for a child node\n");

        free(left);

        return 0;
    }

    start = cfl_parse_whitespace(op_pos + 2, end);
    start = cfl_parse_factor(right, start, end);

    if(!start || !cfl_create_node_and(node, left, right))
    {
        free(left);
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
        free(left);

        return 0;
    }

    cfl_node* right = malloc(sizeof(cfl_node));

    if(!right)
    {
        fprintf(stderr, "ERROR: Could not allocate enough "
                        "space for a child node\n");

        free(left);

        return 0;
    }

    start = cfl_parse_whitespace(op_pos + 2, end);
    start = cfl_parse_term(right, start, end);

    if(!start || !cfl_create_node_or(node, left, right))
    {
        free(left);
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
        free(condition);

        return 0;
    }

    start = cfl_parse_whitespace(start, pos);

    cfl_node* then_node = malloc(sizeof(cfl_node));

    if(!then_node)
    {
        free(condition);

        return 0;
    }

    start = cfl_parse_expression(then_node, start, pos);

    if(!start)
    {
        free(condition);
        free(then_node);

        return 0;
    }

    start = cfl_parse_whitespace(start, pos);

    if(start != pos)
    {
        free(condition);
        free(then_node);

        return 0;
    }

    start += 4;

    start = cfl_parse_whitespace(start, end);

    cfl_node* else_node = malloc(sizeof(cfl_node));

    if(!else_node)
    {
        free(condition);
        free(then_node);

        return 0;
    }

    start = cfl_parse_expression(else_node, start, end);

    if(!start)
    {
        free(condition);
        free(then_node);
        free(else_node);

        return 0;
    }

    if(!cfl_create_node_if(node, condition, then_node, else_node))
    {
        free(condition);
        free(then_node);
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

    return result;
}

char* cfl_parse_factor(cfl_node* node, char* start, char* end)
{
    char* result = cfl_parse_and(node, start, end);

    if(!result)
        result = cfl_parse_atom(node, start, end);

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
