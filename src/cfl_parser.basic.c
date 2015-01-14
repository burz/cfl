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

cfl_list_node* cfl_parse_comma_separated(
        cfl_token_list** end,
        cfl_node_parser* element_parser,
        char* element_name,
        cfl_token_list* position,
        cfl_token_list* block)
{
    cfl_list_node head;
    cfl_list_node* list_pos = &head;

    bool comma = false;

    while(position != block)
    {
        cfl_token_list* pos;

        cfl_node* new_node = (*element_parser)(&pos, position, block);

        if(!new_node)
        {
            if(comma)
            {
                cfl_parse_error_expected(element_name, "\",\"",
                                         position->start, position->end);

                list_pos->next = 0;

                cfl_delete_list_nodes(head.next);

                return 0;
            }

            break;
        }

        list_pos->next = cfl_parser_malloc(sizeof(cfl_list_node));

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

bool cfl_parse_tuple_structure(
        cfl_token_list** end,
        unsigned int* number_of_children,
        cfl_node*** children,
        cfl_node_parser* element_parser,
        char* element_name,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(cfl_token_string_compare(position, "(", 1))
        return false;

    cfl_list_node* list = cfl_parse_comma_separated(&position,
                                                    element_parser,
                                                    element_name,
                                                    position->next,
                                                    block);

    if(cfl_token_string_compare(position, ")", 1))
    {
        cfl_delete_list_nodes(list);

        return false;
    }

    *end = position->next;

    cfl_list_node* list_pos = list;
    *number_of_children = 0;

    while(list_pos)
    {
        ++*number_of_children;

        list_pos = list_pos->next;
    }

    if(*number_of_children)
    {
        *children = cfl_parser_malloc(sizeof(cfl_node*) * (*number_of_children));

        if(!*children)
        {
            cfl_delete_list_nodes(list);

            return false;
        }

        list_pos = list;
        int i = 0;

        while(list_pos)
        {
            (*children)[i] = list_pos->node;

            cfl_list_node* temp = list_pos;

            list_pos = list_pos->next;

            free(temp);

            ++i;
        }
    }

    return true;
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
        if(!cfl_token_string_compare(position,
                                     reserved_words[i],
                                     reserved_word_size[i]))
            return 0;

    *end = position->next;

    return cfl_create_new_node_variable_n(length, position->start);
}

cfl_node* cfl_parse_complex_variable(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(!cfl_token_string_compare(position, "_", 1))
    {
        *end = position->next;

        return cfl_create_new_node_variable_n(1, "_");
    }

    cfl_node* result = cfl_parse_variable(end, position, block);

    if(result)
        return result;

    unsigned int number_of_children;
    cfl_node** children;

    if(!cfl_parse_tuple_structure(end,
                                  &number_of_children,
                                  &children,
                                  &cfl_parse_complex_variable,
                                  "variable",
                                  position, block))
        return 0;

    return cfl_create_new_node_tuple(number_of_children, children);
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

    cfl_node* variable = cfl_parse_complex_variable(&position, position->next, block);

    if(!variable)
        return 0;

    if(position == block || cfl_token_string_compare(position, "->", 2))
    {
        cfl_parse_error_expected("\"->\"", "\"function\"",
                                 position ? position->start : 0,
                                 position ? position->end : 0);

        cfl_free_node(variable);

        return 0;
    }

    cfl_node* expression = cfl_parse_expression(end, position->next, block);

    if(!expression)
    {
        cfl_free_node(variable);

        return 0;
    }

    return cfl_create_new_node_function(variable, expression);
}

cfl_node* cfl_parse_list(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(cfl_token_string_compare(position, "[", 1))
        return 0;

    cfl_list_node* list = cfl_parse_comma_separated(&position,
                                                    &cfl_parse_expression,
                                                    "expression",
                                                    position->next,
                                                    block);

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
    unsigned int number_of_children;
    cfl_node** children;

    if(!cfl_parse_tuple_structure(end,
                                  &number_of_children,
                                  &children,
                                  &cfl_parse_expression,
                                  "expression",
                                  position, block))
        return 0;

    return cfl_create_new_node_tuple(number_of_children, children);
}

cfl_node* cfl_parse_application(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    cfl_node* result = 0;

    while(position != block)
    {
        cfl_token_list* pos;
        cfl_node* atom = cfl_parse_atom(&pos, position, block);

        if(!atom)
            break;

        position = pos;

        if(!result)
        {
            result = atom;

            continue;
        }
        else
        {
            result = cfl_create_new_node_application(result, atom);

            if(!result)
                return 0;
        }
    }

    *end = position;

    return result;
}

static cfl_token_list* cfl_skip_inside_delimiter(
        cfl_token_list* position,
        cfl_token_list* block,
        unsigned int open_size,
        char* open,
        unsigned int close_size,
        char* close)
{
    int depth = 1;

    while(position != block)
    {
        if(!cfl_token_string_compare(position, close, close_size))
        {
            if(depth == 1)
                break;
            else
                --depth;
        }
        else if(!cfl_token_string_compare(position, open, open_size))
            ++depth;

        position = position->next;
    }

    if(position == block)
        return 0;

    return position->next;
}

cfl_token_list* cfl_lookahead_for(
        unsigned int symbol_length,
        char* symbol,
        cfl_token_list* position,
        cfl_token_list* block)
{
    while(position != block)
    {
        if(!cfl_token_string_compare(position, symbol, symbol_length))
            break;
        else if(!cfl_token_string_compare(position, "(", 1))
        {
            position = cfl_skip_inside_delimiter(position->next, block,
                                                 1, "(", 1, ")");

            if(!position)
                return 0;
        }
        else if(!cfl_token_string_compare(position, ")", 1))
            return 0;
        else if(!cfl_token_string_compare(position, "[", 1))
        {
            position = cfl_skip_inside_delimiter(position->next, block,
                                                 1, "[", 1, "]");

            if(!position)
                return 0;
        }
        else if(!cfl_token_string_compare(position, "]", 1))
            return 0;
        else if(!cfl_token_string_compare(position, "if", 2))
        {
            position = cfl_skip_inside_delimiter(position->next, block,
                                                 2, "if", 4, "else");

            if(!position)
                return 0;
        }
        else if(!cfl_token_string_compare(position, "else", 4))
            return 0;
        else if(!cfl_token_string_compare(position, "let", 3))
        {
            position = cfl_skip_inside_delimiter(position->next, block,
                                                 3, "let", 2, "in");

            if(!position)
                return 0;
        }
        else if(!cfl_token_string_compare(position, "in", 2))
            return 0;
        else if(!cfl_token_string_compare(position, "case", 4))
        {
            position = position->next;

            int depth = 2;

            while(position != block)
            {
                if(!cfl_token_string_compare(position, "case", 4))
                    depth += 2;
                else if(!cfl_token_string_compare(position, "function", 8))
                    ++depth;
                else if(!cfl_token_string_compare(position, "->", 2))
                {
                    if(depth == 1)
                        break;
                    else
                        --depth;
                }

                position = position->next;
            }

            if(position == block)
                return 0;

            position = position->next;
        }
        else
            position = position->next;
    }

    if(position == block)
        return 0;

    return position;
}

cfl_node* cfl_parse_if(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(cfl_token_string_compare(position, "if", 2))
        return 0;

    position = position->next;

    cfl_token_list* then_pos = cfl_lookahead_for(4, "then", position, block);

    if(!then_pos)
    {
        cfl_parse_error_expected("\"then\"", "\"if\"",
                                 position->start, position->end);

        return 0;
    }

    cfl_token_list* pos;
    cfl_node* condition = cfl_parse_expression(&pos, position, then_pos);

    if(!condition)
    {
        cfl_parse_error_expected("expression", "\"if\"",
                                 position->start, position->end);

        return 0;
    }

    if(pos != then_pos)
    {
        cfl_parse_error_expected("expression", "\"if\"",
                                 position->start, position->end);

        cfl_free_node(condition);

        return 0;
    }

    position = then_pos->next;

    cfl_token_list* else_pos = cfl_lookahead_for(4, "else", position, block);

    if(!else_pos)
    {
        cfl_parse_error_expected("\"else\"", "\"then\"",
                                 position->start, position->end);

        cfl_free_node(condition);

        return 0;
    }

    cfl_node* then_node = cfl_parse_expression(&pos, position, else_pos);

    if(!then_node)
    {
        cfl_parse_error_expected("expression", "\"then\"",
                                 position->start, position->end);

        cfl_free_node(condition);

        return 0;
    }

    if(pos != else_pos)
    {
        cfl_parse_error_expected("expression", "\"then\"",
                                 position->start, position->end);

        cfl_free_node(condition);
        cfl_free_node(then_node);

        return 0;
    }

    position = else_pos->next;

    cfl_node* else_node = cfl_parse_expression(end, position, block);

    if(!else_node)
    {
        cfl_parse_error_expected("expression", "\"else\"",
                                 position->start, position->end);

        cfl_free_node(condition);
        cfl_free_node(then_node);

        return 0;
    }

    return cfl_create_new_node_if(condition, then_node, else_node);
}

cfl_node* cfl_parse_case(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(cfl_token_string_compare(position, "case", 4))
        return 0;

    position = position->next;

    cfl_token_list* of_pos = cfl_lookahead_for(2, "of", position, block);

    if(!of_pos)
    {
        cfl_parse_error_expected("\"of\"", "\"case\"",
                                 position->start, position->end);

        return 0;
    }

    cfl_token_list* pos;

    cfl_node* list = cfl_parse_expression(&pos, position, of_pos);

    if(!list)
    {
        cfl_parse_error_expected("expression", "\"case\"",
                                 position->start, position->end);

        return 0;
    }

    if(pos != of_pos)
    {
        cfl_parse_error_expected("expression", "\"case\"",
                                 position->start, position->end);

        cfl_free_node(list);

        return 0;
    }

    position = of_pos->next;

    if(position == block || position->next == block ||
       cfl_token_string_compare(position, "[", 1) ||
       cfl_token_string_compare(position->next, "]", 1))
    {
        cfl_parse_error_expected("\"[]\"", "\"of\"",
                                 position->start, position->end);

        cfl_free_node(list);

        return 0;
    }

    position = position->next->next;

    if(position == block || cfl_token_string_compare(position, "->", 2))
    {
        cfl_parse_error_expected("\"->\"", "\"of\"",
                                 position->start, position->end);

        cfl_free_node(list);

        return 0;
    }

    position = position->next;

    cfl_token_list* line_pos = cfl_lookahead_for(1, "|", position, block);

    if(!line_pos)
    {
        cfl_parse_error_expected("\"|\"", "\"->\"",
                                 position->start, position->end);

        cfl_free_node(list);

        return 0;
    }

    cfl_node* empty = cfl_parse_expression(&pos, position, line_pos);

    if(!empty)
    {
        cfl_parse_error_expected("expression", "\"->\"",
                                 position->start, position->end);

        cfl_free_node(list);

        return 0;
    }

    if(pos != line_pos)
    {
        cfl_parse_error_expected("expression", "\"->\"",
                                 position->start, position->end);

        cfl_free_node(list);
        cfl_free_node(empty);

        return 0;
    }

    position = line_pos->next;

    if(position == block || cfl_token_string_compare(position, "(", 1))
    {
        cfl_parse_error_expected("\"(\"", "\"|\"",
                                 position->start, position->end);

        cfl_free_node(list);
        cfl_free_node(empty);

        return 0;
    }

    position = position->next;

    cfl_node* head = cfl_parse_complex_variable(&pos, position, block);

    if(!head)
    {
        cfl_parse_error_expected("variable", "\"(\"",
                                 position->start, position->end);

        cfl_free_node(list);
        cfl_free_node(empty);

        return 0;
    }

    position = pos;

    if(position == block || cfl_token_string_compare(position, ":", 1))
    {
        cfl_parse_error_expected("\":\"", "\"(\"",
                                 position->start, position->end);

        cfl_free_node(list);
        cfl_free_node(empty);
        cfl_free_node(head);

        return 0;
    }

    position = position->next;

    cfl_node* tail = cfl_parse_variable(&pos, position, block);

    if(!tail)
    {
        cfl_parse_error_expected("variable", "\":\"",
                                 position->start, position->end);

        cfl_free_node(list);
        cfl_free_node(empty);
        cfl_free_node(head);

        return 0;
    }

    position = pos;

    if(position == block || cfl_token_string_compare(position, ")", 1))
    {
        cfl_parse_error_expected("\")\"", "\"(\"",
                                 position->start, position->end);

        cfl_free_node(list);
        cfl_free_node(empty);
        cfl_free_node(head);
        cfl_free_node(tail);

        return 0;
    }

    position = position->next;

    if(position == block || cfl_token_string_compare(position, "->", 2))
    {
        cfl_parse_error_expected("\")\"", "\"(\"",
                                 position->start, position->end);

        cfl_free_node(list);
        cfl_free_node(empty);
        cfl_free_node(head);
        cfl_free_node(tail);

        return 0;
    }

    position = position->next;

    cfl_node* nonempty = cfl_parse_expression(end, position, block);

    if(!nonempty)
    {
        cfl_parse_error_expected("expression", "\"->\"",
                                 position->start, position->end);

        cfl_free_node(list);
        cfl_free_node(empty);
        cfl_free_node(head);
        cfl_free_node(tail);

        return 0;
    }

    return cfl_create_new_node_case(list, empty, head, tail, nonempty);
}
