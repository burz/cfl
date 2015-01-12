#include "cfl_parser.h"

extern void* cfl_parser_malloc(size_t size);

cfl_node* cfl_parse_string(
        cfl_token_list** end,
        cfl_token_list* position,
        cfl_token_list* block)
{
    if(*position->start != '"')
        return 0;

    char* pos = position->start + 1;

    cfl_list_node head;
    cfl_list_node* list_pos = &head;

    while(pos != position->end - 1)
    {
        if(*pos == '"')
            break;

        list_pos->next = cfl_parser_malloc(sizeof(cfl_list_node));

        if(!list_pos->next)
        {
            cfl_delete_list_nodes(head.next);

            return 0;
        }

        list_pos->next->node = cfl_create_new_node_char(*pos);

        if(!list_pos->next->node)
        {
            free(list_pos->next);

            list_pos->next = 0;

            cfl_delete_list_nodes(head.next);

            return 0;
        }

        list_pos = list_pos->next;

        ++pos;
    }

    list_pos->next = 0;

    return cfl_create_new_node_list(head.next);
}
