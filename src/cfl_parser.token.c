#include "cfl_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void* cfl_parser_malloc(size_t size);

cfl_token_list* cfl_create_token_list_node(char* start, unsigned int length)
{
    cfl_token_list* result = cfl_parser_malloc(sizeof(cfl_token_list));

    if(!result)
        return 0;

    result->start = start;
    result->end = start + length;

    return result;
}

bool cfl_is_whitespace(char x)
{
    return x == ' ' || x == '\n' || x == '\t' ||
           x == '\r' || x == '\v' || x == '\f';
}

bool cfl_is_number(char x)
{
    return '0' <= x && x <= '9';
}

bool cfl_is_letter(char x)
{
    return ('a' <= x && x <= 'z') ||
           ('A' <= x && x <= 'Z');
}

bool cfl_generate_token_list(cfl_token_list* head, char* start, char* end)
{
    cfl_token_list* back = head;

    while(start != end)
    {
        if(*start == '/' && start[1] == '/')
        {
            while(start + 1 != end && *start != '\n')
                ++start;

            ++start;

            continue;
        }
        else if(*start == '*' || *start == '/' || *start == '%' ||
           *start == ':' || *start == ';' || *start == '(' ||
           *start == ')' || *start == '[' || *start == ']' ||
           *start == ',' || *start == '_' || *start == '.' ||
           *start == '$')
        {
            back->next = cfl_create_token_list_node(start, 1);
            ++start;
        }
        else if(*start == '\'')
        {
            char* pos = start + 2;

            if(*pos != '\'')
            {
                cfl_parse_error_expected("\"'\"", "\"'\"", pos, end);

                back->next = 0;

                cfl_delete_token_list(head->next);

                return 0;
            }

            back->next = cfl_create_token_list_node(start, pos - start + 1);
            start = pos + 1;
        }
        else if(*start == '"')
        {
            char* pos = start + 1;

            while(pos != end && *pos != '"')
                ++pos;

            if(pos == end)
            {
                cfl_parse_error_expected("\"\"\"", "\"\"\"", end, end);

                back->next = 0;

                cfl_delete_token_list(head->next);

                return 0;
            }

            back->next = cfl_create_token_list_node(start, pos - start + 1);
            start = pos + 1;
        }
        else if(*start == '!')
        {
            if(start[1] == '=')
            {
                back->next = cfl_create_token_list_node(start, 2);
                start += 2;
            }
            else
            {
                back->next = cfl_create_token_list_node(start, 1);
                ++start;
            }
        }
        else if(*start == '+')
        {
            if(start[1] == '+')
            {
                back->next = cfl_create_token_list_node(start, 2);
                start += 2;
            }
            else
            {
                back->next = cfl_create_token_list_node(start, 1);
                ++start;
            }
        }
        else if(*start == '-')
        {
            if(start[1] == '>')
            {
                back->next = cfl_create_token_list_node(start, 2);
                start += 2;
            }
            else if(cfl_is_number(start[1]))
            {
                char* pos = start + 1;

                while(pos != end && cfl_is_number(*pos))
                    ++pos;

                back->next = cfl_create_token_list_node(start, pos - start);

                start = pos;
            }
            else
            {
                back->next = cfl_create_token_list_node(start, 1);
                ++start;
            }
        }
        else if(*start == '=')
        {
            if(start[1] == '=')
            {
                back->next = cfl_create_token_list_node(start, 2);
                start += 2;
            }
            else
            {
                back->next = cfl_create_token_list_node(start, 1);
                ++start;
            }
        }
        else if(*start == '|')
        {
            if(start[1] == '|')
            {
                back->next = cfl_create_token_list_node(start, 2);
                start += 2;
            }
            else
            {
                back->next = cfl_create_token_list_node(start, 1);
                ++start;
            }
        }
        else if(*start == '<')
        {
            if(start[1] == '=')
            {
                back->next = cfl_create_token_list_node(start, 2);
                start += 2;
            }
            else
            {
                back->next = cfl_create_token_list_node(start, 1);
                ++start;
            }
        }
        else if(*start == '>')
        {
            if(start[1] == '=')
            {
                back->next = cfl_create_token_list_node(start, 2);
                start += 2;
            }
            else
            {
                back->next = cfl_create_token_list_node(start, 1);
                ++start;
            }
        }
        else if(!strncmp(start, "&&", 2))
        {
            back->next = cfl_create_token_list_node(start, 2);
            start += 2;
        }
        else if(cfl_is_number(*start))
        {
            char* pos = start;

            while(pos != end && cfl_is_number(*pos))
                ++pos;

            back->next = cfl_create_token_list_node(start, pos - start);

            start = pos;
        }
        else if(cfl_is_letter(*start))
        {
            char* pos = start;

            while(pos != end && (cfl_is_letter(*pos) || cfl_is_number(*pos) ||
                                 *pos == '_'))
                ++pos;

            while(pos != end && *pos == '\'')
                ++pos;

            back->next = cfl_create_token_list_node(start, pos - start);

            start = pos;
        }
        else if(cfl_is_whitespace(*start))
        {
            ++start;

            continue;
        }
        else
        {
            cfl_parse_error_unexpected_char(*start);

            back->next = 0;

            cfl_delete_token_list(head->next);

            return 0;
        }

        if(!back->next)
        {
            cfl_delete_token_list(head->next);

            return 0;
        }

        back = back->next;
    }

    back->next = 0;

    return true;
}

void cfl_print_token_list(cfl_token_list* list)
{
    while(list)
    {
        char* pos = list->start;

        printf("\"");

        while(pos != list->end)
            printf("%c", *(pos++));

        printf("\"");

        if(list->next)
            printf(", ");
        else
            printf("\n");

        list = list->next;
    }
}

void cfl_delete_token_list(cfl_token_list* list)
{
    while(list)
    {
        cfl_token_list* temp = list;

        list = list->next;

        free(temp);
    }
}

bool cfl_token_string_compare(cfl_token_list* position, char* string, int length)
{
    if(position->end - position->start != length)
        return true;

    return strncmp(position->start, string, length);
}
