#include "cfl_parser.h"

extern void* cfl_parser_malloc(size_t size);

cfl_node* cfl_parse_parentheses(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(cfl_token_string_compare(position, "(", 1))
        return 0;

    cfl_node* result = cfl_parse_expression(&position, position->next, block);

    if(!result)
    {
        cfl_parse_error_expected("expression", "\"(\"",
                                 position->start, position->end);

        return 0;
    }

    if(cfl_token_string_compare(position, ")", 1))
    {
        cfl_free_node(result);

        return 0;
    }

    *end = position->next;

    return result;
}

cfl_node* cfl_parse_variable(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(!cfl_is_letter(*position->start))
        return 0;

    int length = position->end - position->start;
    int i = 0;

    for( ; i < NUMBER_OF_RESERVED_WORDS; ++i)
        if(!cfl_token_string_compare(position, reserved_words[i], length))
            return 0;

    *end = position->next;

    return cfl_create_new_node_variable_n(length, position->start);
}

cfl_node* cfl_parse_bool(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    bool value;

    if(!cfl_token_string_compare(position, "true", 4))
        value = true;
    else if(!cfl_token_string_compare(position, "false", 5))
        value = false;
    else
        return 0;

    *end = position->next;

    return cfl_create_new_node_bool(value);
}

cfl_node* cfl_parse_integer(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
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

    *end = position->next;

    return cfl_create_new_node_integer(value);
}

cfl_node* cfl_parse_char(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(*position->start != '\'')
        return 0;

    *end = position->next;

    return cfl_create_new_node_char(position->start[1]);
}

cfl_node* cfl_parse_function(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
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

    return cfl_create_new_node_function(variable, expression);
}

cfl_list_node* cfl_parse_comma_separated(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    cfl_list_node head;
    cfl_list_node* list_pos = &head;

    bool comma = false;

    while(position != block)
    {
        cfl_token_list* pos;

        cfl_node* new_node = cfl_parse_expression(&pos, position, block);

        if(!new_node)
        {
            if(comma)
            {
                cfl_parse_error_expected("expression", "\",\"",
                                         position->start, position->end);

                list_pos->next = 0;

                cfl_delete_list_nodes(head.next);

                return 0;
            }

            break;
        }

        list_pos->next = cfl_parser_malloc(sizeof(cfl_list_node*));

        if(!list_pos->next)
        {
            cfl_free_node(new_node);

            cfl_delete_list_nodes(head.next);

            return 0;
        }

        list_pos->next->node = new_node;

        list_pos = list_pos->next;

        if(cfl_token_string_compare(pos, ",", 1))
        {
            position = pos;

            break;
        }

        comma = true;

        position = pos->next;
    }

    list_pos->next = 0;

    *end = position;

    return head.next;
}

cfl_node* cfl_parse_list(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(cfl_token_string_compare(position, "[", 1))
        return 0;

    cfl_list_node* list = cfl_parse_comma_separated(&position, position->next, block);

    if(cfl_token_string_compare(position, "]", 1))
    {
        cfl_delete_list_nodes(list);

        return 0;
    }

    *end = position->next;

    return cfl_create_new_node_list(list);
}

cfl_node* cfl_parse_tuple(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(cfl_token_string_compare(position, "(", 1))
        return 0;

    cfl_list_node* list = cfl_parse_comma_separated(&position, position->next, block);

    if(cfl_token_string_compare(position, ")", 1))
    {
        cfl_delete_list_nodes(list);

        return 0;
    }

    *end = position->next;

    cfl_list_node* list_pos = list;
    int number_of_children = 0;

    while(list_pos)
    {
        ++number_of_children;

        list_pos = list_pos->next;
    }

    cfl_node** children = 0;

    if(number_of_children)
    {
        children = cfl_parser_malloc(sizeof(cfl_node*) * number_of_children);

        if(!children)
        {
            cfl_delete_list_nodes(list);

            return 0;
        }

        list_pos = list;
        int i = 0;

        while(list_pos)
        {
            children[i] = list_pos->node;

            cfl_list_node* temp = list_pos;

            list_pos = list_pos->next;

            free(temp);

            ++i;
        }
    }

    return cfl_create_new_node_tuple(number_of_children, children);
}
