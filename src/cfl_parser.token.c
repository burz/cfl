#include "cfl_parser.h"

#include <stdio.h>
#include <string.h>

extern void* cfl_parser_malloc(size_t size);

cfl_token_chain* cfl_create_token_chain_node(char* start, unsigned int length)
{
    cfl_token_chain* result = cfl_parser_malloc(sizeof(cfl_token_chain));

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

bool cfl_generate_token_chain(cfl_token_chain* head, char* start, char* end)
{
    cfl_token_chain* back = head;

    while(start != end)
    {
        if(*start == '*' || *start == '/' || *start == '%' ||
           *start == '!' || *start == ':' || *start == ';' ||
           *start == '(' || *start == ')' || *start == '[' ||
           *start == ']' || *start == ',')
        {
            back->next = cfl_create_token_chain_node(start, 1);
            ++start;
        }
        else if(*start == '\'')
        {
            char* pos = start + 2;

            if(*pos != '\'')
            {
                cfl_parse_error_expected("\"'\"", "\"'\"", pos, end);

                return 0;
            }

            back->next = cfl_create_token_chain_node(start, pos - start + 1);
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

                return 0;
            }

            back->next = cfl_create_token_chain_node(start, pos - start + 1);
            start = pos + 1;
        }
        else if(*start == '+')
        {
            if(start[1] == '+')
            {
                back->next = cfl_create_token_chain_node(start, 2);
                start += 2;
            }
            else
            {
                back->next = cfl_create_token_chain_node(start, 1);
                start += 2;
            }
        }
        else if(*start == '-')
        {
            if(start[1] == '>')
            {
                back->next = cfl_create_token_chain_node(start, 2);
                start += 2;
            }
            else if(cfl_is_number(start[1]))
            {
                char* pos = start + 1;

                while(pos != end && cfl_is_number(*pos))
                    ++pos;

                back->next = cfl_create_token_chain_node(start, pos - start);

                start = pos;
            }
            else
            {
                back->next = cfl_create_token_chain_node(start, 1);
                ++start;
            }
        }
        else if(*start == '=')
        {
            if(start[1] == '=')
            {
                back->next = cfl_create_token_chain_node(start, 2);
                start += 2;
            }
            else
            {
                back->next = cfl_create_token_chain_node(start, 1);
                ++start;
            }
        }
        else if(*start == '|')
        {
            if(start[1] == '|')
            {
                back->next = cfl_create_token_chain_node(start, 2);
                start += 2;
            }
            else
            {
                back->next = cfl_create_token_chain_node(start, 1);
                ++start;
            }
        }
        else if(*start == '<')
        {
            if(start[1] == '=')
            {
                back->next = cfl_create_token_chain_node(start, 2);
                start += 2;
            }
            else
            {
                back->next = cfl_create_token_chain_node(start, 1);
                ++start;
            }
        }
        else if(*start == '>')
        {
            if(start[1] == '=')
            {
                back->next = cfl_create_token_chain_node(start, 2);
                start += 2;
            }
            else
            {
                back->next = cfl_create_token_chain_node(start, 1);
                ++start;
            }
        }
        else if(!strncmp(start, "&&", 2))
        {
            back->next = cfl_create_token_chain_node(start, 2);
            start += 2;
        }
        else if(cfl_is_number(*start))
        {
            char* pos = start;

            while(pos != end && cfl_is_number(*pos))
                ++pos;

            back->next = cfl_create_token_chain_node(start, pos - start);

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

            back->next = cfl_create_token_chain_node(start, pos - start);

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

            cfl_delete_token_chain(head);

            return 0;
        }

        if(!back->next)
        {
            cfl_delete_token_chain(head);

            return 0;
        }

        back = back->next;
    }

    return true;
}

void cfl_print_token_chain(cfl_token_chain* chain)
{
    while(chain)
    {
        char* pos = chain->start;

        printf("\"");

        while(pos != chain->end)
            printf("%c", *(pos++));

        printf("\"");

        if(chain->next)
            printf(", ");
        else
            printf("\n");

        chain = chain->next;
    }
}

void cfl_delete_token_chain(cfl_token_chain* chain)
{
    while(chain)
    {
        cfl_token_chain* temp = chain;

        chain = chain->next;

        free(temp);
    }
}

bool cfl_token_string_compare(cfl_token_chain* position, char* string, int length)
{
    if(position->end - position->start != length)
        return true;

    return strncmp(position->start, string, length);
}
