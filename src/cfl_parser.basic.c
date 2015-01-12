#include "cfl_parser.h"

extern void* cfl_parser_malloc(size_t size);

cfl_node* cfl_parse_variable(
        cfl_token_chain** end,
        cfl_token_chain* position,
        cfl_token_chain* block)
{
    if(!cfl_is_letter(*position->start))
        return 0;

    int length = position->end - position->start;
    int i = 0;

    for( ; i < NUMBER_OF_RESERVED_WORDS; ++i)
        if(!cfl_token_string_compare(position, reserved_words[i], length))
            return 0;

    cfl_node* result = cfl_parser_malloc(sizeof(cfl_node));

    if(!result)
        return 0;

    if(!cfl_create_node_variable_n(result, length, position->start))
    {
        free(result);

        return 0;
    }

    *end = position->next;

    return result;
}

cfl_node* cfl_parse_bool(
        cfl_token_chain** end,
        cfl_token_chain* position,
        cfl_token_chain* block)
{
    bool value;

    if(!cfl_token_string_compare(position, "true", 4))
        value = true;
    else if(!cfl_token_string_compare(position, "false", 5))
        value = false;
    else
        return 0;

    cfl_node* result = cfl_parser_malloc(sizeof(cfl_node));

    if(!result)
        return 0;

    if(!cfl_create_node_bool(result, value))
    {
        free(result);

        return 0;
    }

    *end = position->next;

    return result;
}

cfl_node* cfl_parse_integer(
        cfl_token_chain** end,
        cfl_token_chain* position,
        cfl_token_chain* block)
{
    char* pos = position->start;

    bool negate = false;

    if(*position->start == '-')
    {
        negate = true;

        ++pos;
    }

    if(position->end == pos || !cfl_is_number(*pos))
        return 0;

    if(position->end - pos > MAX_INTEGER_STRING_LENGTH)
    {
        cfl_parse_error_integer_overflow(pos,
                                         position->end - position->start);

        return 0;
    }

    char buffer[position->end - pos + 1];
    int i = 0;

    for( ; i < position->end - pos; ++i)
        buffer[i] = pos[i];

    buffer[position->end - pos] = 0;

    int value = atoi(buffer);

    if(negate)
        value = -value;

    cfl_node* result = cfl_parser_malloc(sizeof(cfl_node));

    if(!result)
        return 0;

    if(!cfl_create_node_integer(result, value))
    {
        free(result);

        return 0;
    }

    *end = position->next;

    return result;
}

cfl_node* cfl_parse_char(
        cfl_token_chain** end,
        cfl_token_chain* position,
        cfl_token_chain* block)
{
    if(*position->start != '\'')
        return 0;

    cfl_node* result = cfl_parser_malloc(sizeof(cfl_node));

    if(!result)
        return 0;

    if(!cfl_create_node_char(result, position->start[1]))
    {
        free(result);

        return 0;
    }

    *end = position->next;

    return result;
}

cfl_node* cfl_parse_function(
        cfl_token_chain** end,
        cfl_token_chain* position,
        cfl_token_chain* block)
{
    if(cfl_token_string_compare(position, "function", 8))
        return 0;

    cfl_node* variable = cfl_parse_variable(&position, position->next, block);

    if(!variable)
        return 0;

    if(position == block || cfl_token_string_compare(position, "->", 2))
    {
        cfl_free_node(variable);

        return 0;
    }

    cfl_node* expression = cfl_parse_expression(&position, position->next, block);

    if(!expression)
    {
        cfl_free_node(variable);

        return 0;
    }

    cfl_node* result = cfl_parser_malloc(sizeof(cfl_node));

    if(!result)
    {
        cfl_free_node(variable);
        cfl_free_node(expression);

        return 0;
    }

    if(!cfl_create_node_function(result, variable, expression))
    {
        cfl_free_node(variable);
        cfl_free_node(expression);
        free(result);

        return 0;
    }

    return result;
}
