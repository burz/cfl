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

int cfl_is_whitespace(char c)
{
    if(c == ' ' || c == '\n' || c == '\t' ||
       c == '\r' || c == '\v' || c == '\f')
        return 1;

    return 0;
}

char* cfl_parse_whitespace(char* start, char* end)
{
    char* result = start;

    while(start != end)
    {
        if(cfl_is_whitespace(*start))
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

    char* consumed = cfl_parse_whitespace(start, op_pos);

    if(consumed != op_pos || consumed - start < 1)
    {
        cfl_delete_node(left);

        return 0;
    }

    start = op_pos + operand_length;

    consumed = cfl_parse_whitespace(start, end);

    if(consumed - start < 1)
    {
        cfl_delete_node(left);

        return 0;
    }

    start = (*right_parser)(right, consumed, end);

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

    char* pos = start;
    int length = 0;
    char buffer[MAX_INTEGER_STRING_LENGTH];

    while(pos != end && length < MAX_INTEGER_STRING_LENGTH &&
          *pos >= ZERO_CHAR && *pos <= NINE_CHAR)
    {
        buffer[length] = *pos;

        ++pos;
        ++length;
    }

    if(!length)
    {
        if(negate)
            cfl_print_expected_error("integer", "\"-\"", start, end);

        return 0;
    }

    buffer[length] = 0;

    int value = atoi(buffer);

    if(negate)
        value = -value;

    if(!cfl_create_node_integer(node, value))
        return 0;

    return pos;
}

char* cfl_parse_list(cfl_node* node, char* start, char* end)
{
    if(start == end || *(start++) != '[')
        return 0;

    cfl_list_node* list_start = 0;
    cfl_list_node* list_pos = 0;

    int encountered_comma = 0;

    while(start != end)
    {
        start = cfl_parse_whitespace(start, end);

        cfl_node* item = malloc(sizeof(cfl_node));

        if(!item)
        {
            if(!list_pos)
                return 0;

            list_pos->next = 0;

            while(list_start)
            {
                cfl_list_node* temp = list_start;

                list_start = list_start->next;

                cfl_free_node(temp->node);
                free(temp);
            }

            return 0;
        }

        char* pos = cfl_parse_expression(item, start, end);

        if(!pos)
        {
            free(item);

            if(encountered_comma)
            {
                cfl_print_expected_error("expression", "\",\"", start, end);

                if(!list_pos)
                    return 0;

                list_pos->next = 0;

                while(list_start)
                {
                    cfl_list_node* temp = list_start;

                    list_start = list_start->next;

                    cfl_free_node(temp->node);
                    free(temp);
                }

                return 0;
            }

            break;
        }

        start = pos;

        if(!list_pos)
        {
            list_start = malloc(sizeof(cfl_list_node));

            if(!list_start)
                return 0;

            list_start->node = item;

            list_pos = list_start;
        }
        else
        {
            list_pos->next = malloc(sizeof(cfl_list_node));

            if(!list_pos->next)
            {
                while(list_start)
                {
                    cfl_list_node* temp = list_start;

                    list_start = list_start->next;

                    cfl_free_node(temp->node);
                    free(temp);
                }

                return 0;
            }

            list_pos = list_pos->next;

            list_pos->node = item;
        }

        pos = cfl_parse_whitespace(start, end);

        if(pos != end && *(pos++) == ',')
        {
            start = pos;

            encountered_comma = 1;
        }
        else
            break;
    }

    if(list_pos)
        list_pos->next = 0;

    start = cfl_parse_whitespace(start, end);

    if(start == end || *(start++) != ']')
    {
        cfl_print_expected_error("\"]\"", "\"[\"", start, end);

        if(!list_pos)
            return 0;

        while(list_start)
        {
            cfl_list_node* temp = list_start;

            list_start = list_start->next;

            cfl_free_node(temp->node);
            free(temp);
        }

        return 0;
    }

    cfl_create_node_list(node, list_start);

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

    char* pos = cfl_parse_variable(argument, start, end);

    if(!pos)
    {
        cfl_print_expected_error("variable", "\"function\"", start, end);

        free(argument);

        return 0;
    }

    start = cfl_parse_whitespace(pos, end);

    if(end - start < 2 || start[0] != '-' || start[1] != '>')
    {
        cfl_print_expected_error("\"->\"", "\"function\"", start, end);

        cfl_free_node(argument);

        return 0;
    }

    start = cfl_parse_whitespace(start + 2, end);

    cfl_node* body = malloc(sizeof(cfl_node));

    if(!body)
    {
        cfl_free_node(argument);

        return 0;
    }

    pos = cfl_parse_expression(body, start, end);

    if(!pos)
    {
        cfl_print_expected_error("expression", "\"->\"", start, end);

        cfl_free_node(argument);
        free(body);

        return 0;
    }

    if(!cfl_create_node_function(node, argument, body))
    {
        cfl_free_node(argument);
        cfl_free_node(body);

        return 0;
    }

    return pos;
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
        cfl_free_node(left);
        cfl_free_node(right);

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
        cfl_free_node(left);
        cfl_free_node(right);

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

    char* pos = cfl_parse_atom(child_node, start, end);

    if(!pos)
    {
        cfl_print_expected_error("atom", "\"!\"", start, end);

        free(child_node);

        return 0;
    }

    if(!cfl_create_node_not(node, child_node))
    {
        cfl_free_node(child_node);

        return 0;
    }

    return pos;
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
        cfl_free_node(left);
        cfl_free_node(right);

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
        cfl_free_node(left);
        cfl_free_node(right);

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
        cfl_free_node(left);
        cfl_free_node(right);

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
        cfl_free_node(left);
        cfl_free_node(right);

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
        cfl_free_node(left);
        cfl_free_node(right);

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
        char* consumed = cfl_parse_whitespace(start, end);

        cfl_node* argument = malloc(sizeof(cfl_node));

        if(!argument)
        {
            cfl_free_node(function);

            return 0;
        }

        char* pos = cfl_parse_atom(argument, consumed, end);

        if(!pos)
        {
            free(argument);

            break;
        }

        if(consumed - start < 1)
        {
            cfl_free_node(argument);

            break;
        }

        start = pos;

        cfl_node* new_function = malloc(sizeof(cfl_node));

        if(!new_function)
        {
            cfl_free_node(function);
            cfl_free_node(argument);

            return 0;
        }

        if(!cfl_create_node_application(new_function, function, argument))
        {
            cfl_free_node(function);
            cfl_free_node(argument);
            free(new_function);

            return 0;
        }

        function = new_function;

        ++argument_count;
    }

    if(!argument_count)
    {
        cfl_free_node(function);

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

    char* then_pos = start;
    int depth = 1;

    while(then_pos != end)
    {
        if(end - then_pos > 1 && then_pos[0] == 'i' && then_pos[1] == 'f')
            ++depth;
        else if(end - then_pos > 3 && then_pos[0] == 'e' && then_pos[1] == 'l' &&
                then_pos[2] == 's' && then_pos[3] == 'e')
            --depth;
        else if(end - then_pos > 3 && depth == 1 &&
                then_pos[0] == 't' && then_pos[1] == 'h' &&
                then_pos[2] == 'e' && then_pos[3] == 'n')
            break;

        ++then_pos;
    }

    if(then_pos == end || depth != 1)
    {
        cfl_print_expected_error("\"then\"", "\"if\"", start, end);

        return 0;
    }

    cfl_node* condition = malloc(sizeof(cfl_node));

    if(!condition)
    {
        fprintf(stderr, "ERROR: Could not allocate enough "
                        "space for a child node\n");

        return 0;
    }

    char* pos = cfl_parse_expression(condition, start, then_pos);

    if(!pos)
    {
        cfl_print_expected_error("expression", "\"if\"", start, end);

        free(condition);

        return 0;
    }

    start = cfl_parse_whitespace(pos, then_pos);

    if(start != then_pos)
    {
        cfl_free_node(condition);

        return 0;
    }

    start = then_pos + 4;

    char* else_pos = start;

    while(else_pos != end)
    {
        if(end - else_pos > 1 && else_pos[0] == 'i' && else_pos[1] == 'f')
            ++depth;
        else if(end - else_pos > 3 && else_pos[0] == 'e' && else_pos[1] == 'l' &&
                else_pos[2] == 's' && else_pos[3] == 'e')
        {
            --depth;

            if(depth == 0)
                break;
        }

        ++else_pos;
    }

    if(else_pos == end || depth != 0)
    {
        cfl_print_expected_error("\"else\"", "\"then\"", start, end);

        cfl_free_node(condition);

        return 0;
    }

    start = cfl_parse_whitespace(start, else_pos);

    cfl_node* then_node = malloc(sizeof(cfl_node));

    if(!then_node)
    {
        cfl_free_node(condition);

        return 0;
    }

    pos = cfl_parse_expression(then_node, start, else_pos);

    if(!pos)
    {
        cfl_print_expected_error("expression", "\"then\"", start, else_pos);

        cfl_free_node(condition);
        free(then_node);

        return 0;
    }

    start = cfl_parse_whitespace(pos, else_pos);

    if(start != else_pos)
    {
        cfl_free_node(condition);
        cfl_free_node(then_node);

        return 0;
    }

    start += 4;

    start = cfl_parse_whitespace(start, end);

    cfl_node* else_node = malloc(sizeof(cfl_node));

    if(!else_node)
    {
        cfl_free_node(condition);
        cfl_free_node(then_node);

        return 0;
    }

    pos = cfl_parse_expression(else_node, start, end);

    if(!pos)
    {
        cfl_print_expected_error("expression", "\"else\"", start, end);

        cfl_free_node(condition);
        cfl_free_node(then_node);
        free(else_node);

        return 0;
    }

    if(!cfl_create_node_if(node, condition, then_node, else_node))
    {
        cfl_free_node(condition);
        cfl_free_node(then_node);
        cfl_free_node(else_node);

        return 0;
    }

    return pos;
}

char* cfl_parse_push(cfl_node* node, char* start, char* end)
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
                                       &cfl_parse_term,
                                       &cfl_parse_term,
                                       1,
                                       ":",
                                       start,
                                       end);

    if(!start)
    {
        free(left);
        free(right);

        return 0;
    }

    if(!cfl_create_node_push(node, left, right))
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    return start;
}

char* cfl_parse_concatenate(cfl_node* node, char* start, char* end)
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
                                       &cfl_parse_term,
                                       &cfl_parse_term,
                                       2,
                                       "++",
                                       start,
                                       end);

    if(!start)
    {
        free(left);
        free(right);

        return 0;
    }

    if(!cfl_create_node_concatenate(node, left, right))
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    return start;
}

char* cfl_parse_case(cfl_node* node, char* start, char* end)
{
    if(end - start < 4 || start[0] != 'c' || start[1] != 'a' ||
       start[2] != 's' || start[3] != 'e')
        return 0;

    start = cfl_parse_whitespace(start + 4, end);

    char* of_pos = start;
    int depth = 1;

    while(end - of_pos > 1 &&
          !(depth == 1 && of_pos[0] == 'o' && of_pos[1] == 'f'))
    {
        if(end - of_pos > 3 && of_pos[0] == 'c' && of_pos[1] == 'a' &&
           of_pos[2] == 's' && of_pos[3] == 'e')
        {
            depth += 2;

            of_pos += 4;
        }
        else if(end - of_pos > 7 && of_pos[0] == 'f' && of_pos[1] == 'u' &&
                of_pos[2] == 'n' && of_pos[3] == 'c' && of_pos[4] == 't' &&
                of_pos[5] == 'i' && of_pos[6] == 'o' && of_pos[7] == 'n')
        {
            ++depth;

            of_pos += 8;
        }
        else if(end - of_pos > 2 && of_pos[0] == '-' && of_pos[1] == '>')
        {
            --depth;

            of_pos += 2;
        }
        else
            ++of_pos;
    }

    if(end - of_pos < 2 || depth != 1)
    {
        cfl_print_expected_error("\"of\"", "\"case\"", start, end);

        return 0;
    }

    start = cfl_parse_whitespace(start, of_pos);

    cfl_node* list = malloc(sizeof(cfl_node));

    if(!list)
        return 0;

    char* pos = cfl_parse_expression(list, start, of_pos);

    if(!pos)
    {
        cfl_print_expected_error("expression", "\"case\"", start, end);

        free(list);

        return 0;
    }

    start = cfl_parse_whitespace(pos, of_pos);

    if(start != of_pos)
    {
        cfl_free_node(list);

        return 0;
    }

    start = cfl_parse_whitespace(of_pos + 2, end);

    if(end - start < 2 || start[0] != '[' || start[1] != ']')
    {
        cfl_print_expected_error("\"[]\"", "\"of\"", start, end);

        cfl_free_node(list);

        return 0;
    }

    start = cfl_parse_whitespace(start + 2, end);

    if(end - start < 2 || start[0] != '-' || start[1] != '>')
    {
        cfl_print_expected_error("\"->\"", "\"[]\"", start, end);

        cfl_free_node(list);

        return 0;
    }

    start = cfl_parse_whitespace(start + 2, end);

    char* line_pos = start;
    depth = 1;

    while(end - line_pos > 2 &&
          !(depth == 1 && line_pos[0] == '|' && line_pos[1] != '|'))
    {
        if(end - line_pos > 3 && line_pos[0] == 'c' && line_pos[1] == 'a' &&
           line_pos[2] == 's' && line_pos[3] == 'e')
        {
            depth += 2;

            line_pos += 4;
        }
        else if(end - line_pos > 7 && line_pos[0] == 'f' && line_pos[1] == 'u' &&
                line_pos[2] == 'n' && line_pos[3] == 'c' && line_pos[4] == 't' &&
                line_pos[5] == 'i' && line_pos[6] == 'o' && line_pos[7] == 'n')
        {
            ++depth;

            line_pos += 8;
        }
        else if(end - line_pos > 2 && line_pos[0] == '-' && line_pos[1] == '>')
        {
            --depth;

            line_pos += 2;
        }
        else
            ++line_pos;
    }

    if(end - line_pos < 3 || depth != 1)
    {
        cfl_print_expected_error("\"|\"", "\"case\"", start, end);

        cfl_free_node(list);

        return 0;
    }

    cfl_node* empty = malloc(sizeof(cfl_node));

    if(!empty)
    {
        cfl_free_node(list);

        return 0;
    }

    start = cfl_parse_expression(empty, start, line_pos);

    if(!start)
    {
        cfl_free_node(list);
        free(empty);

        return 0;
    }

    pos = cfl_parse_whitespace(line_pos + 1, end);

    if(end - pos < 1 || *pos != '(')
    {
        cfl_print_expected_error("\"(\"", "\"|\"", pos, end);

        cfl_free_node(list);
        cfl_free_node(empty);

        return 0;
    }

    start = cfl_parse_whitespace(pos + 1, end);

    char* semi_pos = start;

    while(end - semi_pos > 1 && *semi_pos != ':')
        ++semi_pos;

    if(end - semi_pos < 2)
    {
        cfl_print_expected_error("\":\"", "\"(\"", start, end);

        cfl_free_node(list);
        cfl_free_node(empty);

        return 0;
    }

    cfl_node* head = malloc(sizeof(cfl_node));

    if(!head)
    {
        cfl_free_node(list);
        cfl_free_node(empty);

        return 0;
    }

    pos = cfl_parse_variable(head, start, semi_pos);

    if(!pos)
    {
        cfl_print_expected_error("variable", "\"(\"", start, end);

        cfl_free_node(list);
        cfl_free_node(empty);
        free(head);

        return 0;
    }

    start = cfl_parse_whitespace(pos, semi_pos);

    if(start != semi_pos)
    {
        cfl_print_expected_error("variable", "\"(\"", start, end);

        cfl_free_node(list);
        cfl_free_node(empty);
        cfl_free_node(head);

        return 0;
    }

    start = cfl_parse_whitespace(semi_pos + 1, end);

    char* par_pos = start;

    while(end - par_pos > 1 && *par_pos != ')')
        ++par_pos;

    if(end - par_pos < 2)
    {
        cfl_print_expected_error("\")\"", "\"(\"", start, end);

        cfl_free_node(list);
        cfl_free_node(empty);
        cfl_free_node(head);

        return 0;
    }

    cfl_node* tail = malloc(sizeof(cfl_node));

    if(!tail)
    {
        cfl_free_node(list);
        cfl_free_node(empty);
        cfl_free_node(head);

        return 0;
    }

    pos = cfl_parse_variable(tail, start, par_pos);

    if(!pos)
    {
        cfl_print_expected_error("variable", "\":\"", start, end);

        cfl_free_node(list);
        cfl_free_node(empty);
        cfl_free_node(head);
        free(tail);

        return 0;
    }

    pos = cfl_parse_whitespace(pos, par_pos);

    if(pos != par_pos)
    {
        cfl_print_expected_error("variable", "\":\"", start, end);

        cfl_free_node(list);
        cfl_free_node(empty);
        cfl_free_node(head);
        free(tail);

        return 0;
    }

    start = cfl_parse_whitespace(par_pos + 1, end);

    if(end - start < 2 || start[0] != '-' || start[1] != '>')
    {
        cfl_print_expected_error("\"->\"", "\")\"", start, end);

        cfl_free_node(list);
        cfl_free_node(empty);
        cfl_free_node(head);
        cfl_free_node(tail);

        return 0;
    }

    start = cfl_parse_whitespace(start + 2, end);

    cfl_node* nonempty = malloc(sizeof(cfl_node));

    if(!nonempty)
    {
        cfl_free_node(list);
        cfl_free_node(empty);
        cfl_free_node(head);
        cfl_free_node(tail);

        return 0;
    }

    pos = cfl_parse_expression(nonempty, start, end);

    if(!pos)
    {
        cfl_print_expected_error("expression", "\"->\"", start, end);

        cfl_free_node(list);
        cfl_free_node(empty);
        cfl_free_node(head);
        cfl_free_node(tail);
        free(nonempty);

        return 0;
    }

    if(!cfl_create_node_case(node, list, empty, head, tail, nonempty))
    {
        cfl_free_node(list);
        cfl_free_node(empty);
        cfl_free_node(head);
        cfl_free_node(tail);
        cfl_free_node(nonempty);

        return 0;
    }

    return pos;
}
