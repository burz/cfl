#include "cfl_ast.h"

#include <stdio.h>
#include <string.h>

char* reserved_words[] = { "true", "false", "function",
                           "if", "then", "else", "let", "in",
                           "case", "of" };

int reserved_word_size[] = { 4, 5, 8, 2, 4, 4, 3, 2, 4, 2 };

extern void* cfl_ast_malloc(size_t size);

cfl_node* cfl_create_new_node_variable(char* string)
{
    cfl_node* node = cfl_ast_malloc(sizeof(cfl_node));

    if(!node)
        return 0;

    node->type = CFL_NODE_VARIABLE;
    node->number_of_children = 0;
    node->data = cfl_ast_malloc(sizeof(char) * MAX_IDENTIFIER_LENGTH);

    if(!node->data)
    {
        free(node);

        return 0;
    }

    strncpy(node->data, string, MAX_IDENTIFIER_LENGTH);

    ((char*) node->data)[MAX_IDENTIFIER_LENGTH - 1] = 0;

    return node;
}

cfl_node* cfl_create_new_node_variable_n(int string_length, char* string)
{
    cfl_node* node = cfl_ast_malloc(sizeof(cfl_node));

    if(!node)
        return 0;

    node->type = CFL_NODE_VARIABLE;
    node->number_of_children = 0;
    node->data = cfl_ast_malloc(sizeof(char) * (string_length + 1));

    if(!node->data)
    {
        free(node);

        return 0;
    }

    strncpy(node->data, string, string_length);

    ((char*) node->data)[string_length] = 0;

    return node;
}

cfl_node* cfl_create_new_node_bool(bool value)
{
    cfl_node* node = cfl_ast_malloc(sizeof(cfl_node));

    if(!node)
        return 0;

    node->type = CFL_NODE_BOOL;
    node->number_of_children = 0;
    node->data = cfl_ast_malloc(sizeof(bool));

    if(!node->data)
    {
        free(node);

        return 0;
    }

    *((bool*) node->data) = value;

    return node;
}

cfl_node* cfl_create_new_node_integer(int value)
{
    cfl_node* node = cfl_ast_malloc(sizeof(cfl_node));

    if(!node)
        return 0;

    node->type = CFL_NODE_INTEGER;
    node->number_of_children = 0;
    node->data = cfl_ast_malloc(sizeof(int));

    if(!node->data)
    {
        free(node);

        return 0;
    }

    *((int*) node->data) = value;

    return node;
}

cfl_node* cfl_create_new_node_char(char value)
{
    cfl_node* node = cfl_ast_malloc(sizeof(cfl_node));

    if(!node)
        return 0;

    node->type = CFL_NODE_CHAR;
    node->number_of_children = 0;
    node->data = cfl_ast_malloc(sizeof(char));

    if(!node->data)
    {
        free(node);

        return 0;
    }

    *((char*) node->data) = value;

    return node;
}

cfl_node* cfl_create_new_node_function(cfl_node* argument, cfl_node* body)
{
    cfl_node* node = cfl_ast_malloc(sizeof(cfl_node));

    if(!node)
    {
        cfl_free_node(argument);
        cfl_free_node(body);

        return 0;
    }

    node->type = CFL_NODE_FUNCTION;
    node->number_of_children = 2;
    node->data = 0;
    node->children = cfl_ast_malloc(sizeof(cfl_node*) * 2);

    if(!node->children)
    {
        cfl_free_node(argument);
        cfl_free_node(body);
        free(node);

        return 0;
    }

    node->children[0] = argument;
    node->children[1] = body;

    return node;
}

void cfl_delete_list_nodes(cfl_list_node* list)
{
    while(list)
    {
        cfl_list_node* temp = list;

        list = list->next;

        cfl_free_node(temp->node);
        free(temp);
    }
}

cfl_node* cfl_create_new_node_list(cfl_list_node* list)
{
    cfl_node* node = cfl_ast_malloc(sizeof(cfl_node));

    if(!node)
    {
        cfl_delete_list_nodes(list);

        return 0;
    }

    node->type = CFL_NODE_LIST;
    node->number_of_children = 0;
    node->data = list;

    return node;
}

cfl_node* cfl_create_new_node_tuple(
        unsigned int number_of_children,
        cfl_node** children)
{
    cfl_node* node = cfl_ast_malloc(sizeof(cfl_node));

    if(!node)
    {
        if(number_of_children)
        {
            int i = 0;
            for( ; i < number_of_children; ++i)
                free(children[i]);

            free(children);
        }

        return 0;
    }

    node->type = CFL_NODE_TUPLE;
    node->number_of_children = number_of_children;
    node->data = 0;
    node->children = children;

    return node;
}

cfl_node* cfl_create_new_node_and(cfl_node* left, cfl_node* right)
{
    cfl_node* node = cfl_ast_malloc(sizeof(cfl_node));

    if(!node)
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    node->type = CFL_NODE_AND;
    node->number_of_children = 2;
    node->data = 0;
    node->children = cfl_ast_malloc(sizeof(cfl_node*) * 2);

    if(!node->children)
    {
        cfl_free_node(left);
        cfl_free_node(right);
        free(node);

        return 0;
    }

    node->children[0] = left;
    node->children[1] = right;

    return node;
}

cfl_node* cfl_create_new_node_or(cfl_node* left, cfl_node* right)
{
    cfl_node* node = cfl_ast_malloc(sizeof(cfl_node));

    if(!node)
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    node->type = CFL_NODE_OR;
    node->number_of_children = 2;
    node->data = 0;
    node->children = cfl_ast_malloc(sizeof(cfl_node*) * 2);

    if(!node->children)
    {
        cfl_free_node(left);
        cfl_free_node(right);
        free(node);

        return 0;
    }

    node->children[0] = left;
    node->children[1] = right;

    return node;
}

cfl_node* cfl_create_new_node_not(cfl_node* child)
{
    cfl_node* node = cfl_ast_malloc(sizeof(cfl_node));

    if(!node)
    {
        cfl_free_node(child);

        return 0;
    }

    node->type = CFL_NODE_NOT;
    node->number_of_children = 1;
    node->data = 0;
    node->children = cfl_ast_malloc(sizeof(cfl_node*));

    if(!node->children)
    {
        cfl_free_node(child);
        free(node);

        return 0;
    }

    *node->children = child;

    return node;
}

cfl_node* cfl_create_new_node_add(cfl_node* left, cfl_node* right)
{
    cfl_node* node = cfl_ast_malloc(sizeof(cfl_node));

    if(!node)
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    node->type = CFL_NODE_ADD;
    node->number_of_children = 2;
    node->data = 0;
    node->children = cfl_ast_malloc(sizeof(cfl_node*) * 2);

    if(!node->children)
    {
        cfl_free_node(left);
        cfl_free_node(right);
        free(node);

        return 0;
    }

    node->children[0] = left;
    node->children[1] = right;

    return node;
}

cfl_node* cfl_create_new_node_multiply(cfl_node* left, cfl_node* right)
{
    cfl_node* node = cfl_ast_malloc(sizeof(cfl_node));

    if(!node)
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    node->type = CFL_NODE_MULTIPLY;
    node->number_of_children = 2;
    node->data = 0;
    node->children = cfl_ast_malloc(sizeof(cfl_node*) * 2);

    if(!node->children)
    {
        cfl_free_node(left);
        cfl_free_node(right);
        free(node);

        return 0;
    }

    node->children[0] = left;
    node->children[1] = right;

    return node;
}

cfl_node* cfl_create_new_node_divide(cfl_node* left, cfl_node* right)
{
    cfl_node* node = cfl_ast_malloc(sizeof(cfl_node));

    if(!node)
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    node->type = CFL_NODE_DIVIDE;
    node->number_of_children = 2;
    node->data = 0;
    node->children = cfl_ast_malloc(sizeof(cfl_node*) * 2);

    if(!node->children)
    {
        cfl_free_node(left);
        cfl_free_node(right);
        free(node);

        return 0;
    }

    node->children[0] = left;
    node->children[1] = right;

    return node;
}

cfl_node* cfl_create_new_node_equal(cfl_node* left, cfl_node* right)
{
    cfl_node* node = cfl_ast_malloc(sizeof(cfl_node));

    if(!node)
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    node->type = CFL_NODE_EQUAL;
    node->number_of_children = 2;
    node->data = 0;
    node->children = cfl_ast_malloc(sizeof(cfl_node*) * 2);

    if(!node->children)
    {
        cfl_free_node(left);
        cfl_free_node(right);
        free(node);

        return 0;
    }

    node->children[0] = left;
    node->children[1] = right;

    return node;
}

cfl_node* cfl_create_new_node_less(cfl_node* left, cfl_node* right)
{
    cfl_node* node = cfl_ast_malloc(sizeof(cfl_node));

    if(!node)
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    node->type = CFL_NODE_LESS;
    node->number_of_children = 2;
    node->data = 0;
    node->children = cfl_ast_malloc(sizeof(cfl_node*) * 2);

    if(!node->children)
    {
        cfl_free_node(left);
        cfl_free_node(right);
        free(node);

        return 0;
    }

    node->children[0] = left;
    node->children[1] = right;

    return node;
}

cfl_node* cfl_create_new_node_application(
        cfl_node* function,
        cfl_node* argument)
{
    cfl_node* node = cfl_ast_malloc(sizeof(cfl_node));

    if(!node)
    {
        cfl_free_node(function);
        cfl_free_node(argument);

        return 0;
    }

    node->type = CFL_NODE_APPLICATION;
    node->number_of_children = 2;
    node->data = 0;
    node->children = cfl_ast_malloc(sizeof(cfl_node*) * 2);

    if(!node->children)
    {
        cfl_free_node(function);
        cfl_free_node(argument);
        free(node);

        return 0;
    }

    node->children[0] = function;
    node->children[1] = argument;

    return node;
}

cfl_node* cfl_create_new_node_if(
        cfl_node* condition,
        cfl_node* then_node,
        cfl_node* else_node)
{
    cfl_node* node = cfl_ast_malloc(sizeof(cfl_node));

    if(!node)
    {
        cfl_free_node(condition);
        cfl_free_node(then_node);
        cfl_free_node(else_node);

        return 0;
    }

    node->type = CFL_NODE_IF;
    node->number_of_children = 3;
    node->data = 0;
    node->children = cfl_ast_malloc(sizeof(cfl_node*) * 3);

    if(!node->children)
    {
        cfl_free_node(condition);
        cfl_free_node(then_node);
        cfl_free_node(else_node);
        free(node);

        return 0;
    }

    node->children[0] = condition;
    node->children[1] = then_node;
    node->children[2] = else_node;

    return node;
}

cfl_node* cfl_create_new_node_let_rec(
        cfl_node* name,
        cfl_node* argument,
        cfl_node* procedure,
        cfl_node* body)
{
    cfl_node* node = cfl_ast_malloc(sizeof(cfl_node));

    if(!node)
    {
        cfl_free_node(name);
        cfl_free_node(argument);
        cfl_free_node(procedure);
        cfl_free_node(body);

        return 0;
    }

    node->type = CFL_NODE_LET_REC;
    node->number_of_children = 4;
    node->data = 0;
    node->children = cfl_ast_malloc(sizeof(cfl_node*) * 4);

    if(!node->children)
    {
        cfl_free_node(name);
        cfl_free_node(argument);
        cfl_free_node(procedure);
        cfl_free_node(body);
        free(node);

        return 0;
    }

    node->children[0] = name;
    node->children[1] = argument;
    node->children[2] = procedure;
    node->children[3] = body;

    return node;
}

cfl_node* cfl_create_new_node_push(cfl_node* left, cfl_node* right)
{
    cfl_node* node = cfl_ast_malloc(sizeof(cfl_node));

    if(!node)
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    node->type = CFL_NODE_PUSH;
    node->number_of_children = 2;
    node->data = 0;
    node->children = cfl_ast_malloc(sizeof(cfl_node*) * 2);

    if(!node->children)
    {
        cfl_free_node(left);
        cfl_free_node(right);
        free(node);

        return 0;
    }

    node->children[0] = left;
    node->children[1] = right;

    return node;
}

cfl_node* cfl_create_new_node_concatenate(cfl_node* left, cfl_node* right)
{
    cfl_node* node = cfl_ast_malloc(sizeof(cfl_node));

    if(!node)
    {
        cfl_free_node(left);
        cfl_free_node(right);

        return 0;
    }

    node->type = CFL_NODE_CONCATENATE;
    node->number_of_children = 2;
    node->data = 0;
    node->children = cfl_ast_malloc(sizeof(cfl_node*) * 2);

    if(!node->children)
    {
        cfl_free_node(left);
        cfl_free_node(right);
        free(node);

        return 0;
    }

    node->children[0] = left;
    node->children[1] = right;

    return node;
}

cfl_node* cfl_create_new_node_case(
        cfl_node* list,
        cfl_node* empty,
        cfl_node* head,
        cfl_node* tail,
        cfl_node* nonempty)
{
    cfl_node* node = cfl_ast_malloc(sizeof(cfl_node));

    if(!node)
    {
        cfl_free_node(list);
        cfl_free_node(empty);
        cfl_free_node(head);
        cfl_free_node(tail);
        cfl_free_node(nonempty);

        return 0;
    }

    node->type = CFL_NODE_CASE;
    node->number_of_children = 5;
    node->data = 0;
    node->children = cfl_ast_malloc(sizeof(cfl_node*) * 5);

    if(!node->children)
    {
        cfl_free_node(list);
        cfl_free_node(empty);
        cfl_free_node(head);
        cfl_free_node(tail);
        cfl_free_node(nonempty);
        free(node);

        return 0;
    }

    node->children[0] = list;
    node->children[1] = empty;
    node->children[2] = head;
    node->children[3] = tail;
    node->children[4] = nonempty;

    return node;
}

cfl_node* cfl_copy_new_node(cfl_node* node)
{
    if(node->type == CFL_NODE_VARIABLE)
        return cfl_create_new_node_variable(node->data);
    else if(node->type == CFL_NODE_BOOL)
        return cfl_create_new_node_bool(*((bool*) node->data));
    else if(node->type == CFL_NODE_INTEGER)
        return cfl_create_new_node_integer(*((int*) node->data));
    else if(node->type == CFL_NODE_CHAR)
        return cfl_create_new_node_char(*((char*) node->data));
    else if(node->type == CFL_NODE_LIST)
    {
        if(node->data == 0)
            return cfl_create_new_node_list(0);

        cfl_list_node* start =
            cfl_ast_malloc(sizeof(cfl_list_node));

        if(!start)
            return 0;

        cfl_list_node* node_pos = node->data;

        start->node = cfl_copy_new_node(node_pos->node);

        if(!start->node)
        {
            free(start);

            return 0;
        }

        node_pos = node_pos->next;

        cfl_list_node* target_pos = start;

        while(node_pos)
        {
            target_pos->next =
                cfl_ast_malloc(sizeof(cfl_list_node));

            if(!target_pos->next)
            {
                while(start)
                {
                    target_pos = start;

                    start = start->next;

                    cfl_free_node(target_pos->node);
                    free(target_pos);
                }

                return 0;
            }

            target_pos->next->node = cfl_copy_new_node(node_pos->node);

            if(!target_pos->next->node)
            {
                free(target_pos->next);

                target_pos->next = 0;

                while(start->next != 0)
                {
                    target_pos = start;

                    start = start->next;

                    cfl_free_node(target_pos->node);
                    free(target_pos);
                }

                free(start->node);
                free(start);

                return 0;
            }

            target_pos = target_pos->next;

            node_pos = node_pos->next;
        }

        target_pos->next = 0;

        return cfl_create_new_node_list(start);
    }
    else
    {
        cfl_node* target = cfl_ast_malloc(sizeof(cfl_node));

        if(!target)
            return 0;

        target->type = node->type;
        target->number_of_children = node->number_of_children;
        target->data = 0;

        if(node->number_of_children)
        {
            target->children = cfl_ast_malloc(sizeof(cfl_node*) *
                                              target->number_of_children);

            if(!target->children)
            {
                free(target);

                return 0;
            }

            int i = 0;

            for( ; i < target->number_of_children; ++i)
            {
                cfl_node* child = cfl_copy_new_node(node->children[i]);

                if(!child)
                {
                    int j = 0;

                    for( ; j < i; ++j)
                        free(target->children[j]);

                    free(target->children);
                    free(target);

                    return 0;
                }

                target->children[i] = child;
            }
        }

        return target;
    }
}

bool cfl_reinitialize_node_bool(cfl_node* node, bool value)
{
    if(node->type == CFL_NODE_BOOL)
    {
        *((bool*) node->data) = value;

        return true;
    }

    cfl_delete_node(node);

    node->type = CFL_NODE_BOOL;
    node->number_of_children = 0;
    node->data = cfl_ast_malloc(sizeof(bool));

    if(!node->data)
        return false;

    *((bool*) node->data) = value;

    return true;
}

bool cfl_reinitialize_node_integer(cfl_node* node, int value)
{
    if(node->type == CFL_NODE_INTEGER)
    {
        *((int*) node->data) = value;

        return true;
    }

    cfl_delete_node(node);

    node->type = CFL_NODE_INTEGER;
    node->number_of_children = 0;
    node->data = cfl_ast_malloc(sizeof(int));

    if(!node->data)
        return false;

    *((int*) node->data) = value;

    return true;
}

void cfl_delete_node(cfl_node* node)
{
    if(node->data)
    {
        if(node->type == CFL_NODE_LIST)
        {
            cfl_list_node* pos = node->data;

            while(pos)
            {
                cfl_list_node* temp = pos;

                pos = pos->next;

                cfl_free_node(temp->node);
                free(temp);
            }
        }
        else
            free(node->data);
    }

    if(node->number_of_children)
    {
        int i = 0;

        for( ; i < node->number_of_children; ++i)
            cfl_free_node(node->children[i]);

        free(node->children);
    }
}

void cfl_free_node(cfl_node* node)
{
    cfl_delete_node(node);

    free(node);
}

static bool cfl_is_bound_in(char* name, cfl_node* node)
{
    if(node->type == CFL_NODE_VARIABLE)
        return !strcmp(name, node->data);

    int i = 0;
    for( ; i < node->number_of_children; ++i)
        if(cfl_is_bound_in(name, node->children[i]))
            return true;

    return false;
}

bool cfl_is_free(char* name, cfl_node* node)
{
    int i;
    cfl_list_node* pos;

    switch(node->type)
    {
        case CFL_NODE_VARIABLE:
            if(!strcmp(name, node->data))
                return true;
            break;
        case CFL_NODE_BOOL:
        case CFL_NODE_INTEGER:
        case CFL_NODE_CHAR:
            break;
        case CFL_NODE_FUNCTION:
            if(!cfl_is_bound_in(name, node->children[0]))
                return cfl_is_free(name, node->children[1]);
            break;
        case CFL_NODE_LIST:
            for(pos = node->data; pos; pos = pos->next)
                if(cfl_is_free(name, pos->node))
                    return true;
            break;
        case CFL_NODE_LET_REC:
            if(strcmp(name, node->children[0]->data))
            {
                if(!cfl_is_bound_in(name, node->children[1]))
                    return cfl_is_free(name, node->children[2]) ||
                           cfl_is_free(name, node->children[3]);
                else
                    return cfl_is_free(name, node->children[3]);
            }
            break;
        case CFL_NODE_CASE:
            if(cfl_is_bound_in(name, node->children[2]) ||
               !strcmp(name, node->children[3]->data))
                return cfl_is_free(name, node->children[0]) ||
                       cfl_is_free(name, node->children[1]);
            else
                return cfl_is_free(name, node->children[0]) ||
                       cfl_is_free(name, node->children[1]) ||
                       cfl_is_free(name, node->children[4]);
            break;
        case CFL_NODE_AND:
        case CFL_NODE_OR:
        case CFL_NODE_NOT:
        case CFL_NODE_ADD:
        case CFL_NODE_MULTIPLY:
        case CFL_NODE_DIVIDE:
        case CFL_NODE_EQUAL:
        case CFL_NODE_LESS:
        case CFL_NODE_APPLICATION:
        case CFL_NODE_IF:
        case CFL_NODE_PUSH:
        case CFL_NODE_CONCATENATE:
        case CFL_NODE_TUPLE:
            for(i = 0; i < node->number_of_children; ++i)
                if(cfl_is_free(name, node->children[i]))
                    return true;
            break;
        default:
            break;
    }

    return false;
}

static void cfl_print_node_inner(cfl_node* node)
{
    cfl_list_node* pos;
    int i;

    switch(node->type)
    {
        case CFL_NODE_VARIABLE:
            printf("%s", (char*) node->data);
            break;
        case CFL_NODE_BOOL:
            printf(*((bool*) node->data) ? "true" : "false");
            break;
        case CFL_NODE_INTEGER:
            printf("%d", *((int*) node->data));
            break;
        case CFL_NODE_CHAR:
            printf("'%c'", *((char*) node->data));
            break;
        case CFL_NODE_FUNCTION:
            printf("function ");
            cfl_print_node_inner(node->children[0]);
            printf(" -> (");
            cfl_print_node_inner(node->children[1]);
            printf(")");
            break;
        case CFL_NODE_LIST:
            if(!node->data)
                printf("[]");
            else if(((cfl_list_node*) node->data)->node->type == CFL_NODE_CHAR)
            {
                printf("\"");
                pos = node->data;
                while(pos)
                {
                    printf("%c", *((char*) pos->node->data));
                    pos = pos->next;
                }
                printf("\"");
            }
            else
            {
                printf("[");
                pos = node->data;
                while(pos)
                {
                    cfl_print_node_inner(pos->node);
                    if(pos->next)
                        printf(", ");
                    pos = pos->next;
                }
                printf("]");
            }
            break;
        case CFL_NODE_TUPLE:
            printf("(");
            for(i = 0; i < node->number_of_children; ++i)
            {
                cfl_print_node_inner(node->children[i]);

                if(i < node->number_of_children - 1)
                    printf(", ");
            }
            printf(")");
            break;
        case CFL_NODE_AND:
            printf("(");
            cfl_print_node_inner(node->children[0]);
            printf(") && (");
            cfl_print_node_inner(node->children[1]);
            printf(")");
            break;
        case CFL_NODE_OR:
            printf("(");
            cfl_print_node_inner(node->children[0]);
            printf(") || (");
            cfl_print_node_inner(node->children[1]);
            printf(")");
            break;
        case CFL_NODE_NOT:
            printf("!(");
            cfl_print_node_inner(node->children[0]);
            printf(")");
            break;
        case CFL_NODE_ADD:
            printf("(");
            cfl_print_node_inner(node->children[0]);
            printf(") + (");
            cfl_print_node_inner(node->children[1]);
            printf(")");
            break;
        case CFL_NODE_MULTIPLY:
            printf("(");
            cfl_print_node_inner(node->children[0]);
            printf(") * (");
            cfl_print_node_inner(node->children[1]);
            printf(")");
            break;
        case CFL_NODE_DIVIDE:
            printf("(");
            cfl_print_node_inner(node->children[0]);
            printf(") / (");
            cfl_print_node_inner(node->children[1]);
            printf(")");
            break;
        case CFL_NODE_EQUAL:
            printf("(");
            cfl_print_node_inner(node->children[0]);
            printf(") == (");
            cfl_print_node_inner(node->children[1]);
            printf(")");
            break;
        case CFL_NODE_LESS:
            printf("(");
            cfl_print_node_inner(node->children[0]);
            printf(") < (");
            cfl_print_node_inner(node->children[1]);
            printf(")");
            break;
        case CFL_NODE_APPLICATION:
            printf("(");
            cfl_print_node_inner(node->children[0]);
            printf(") (");
            cfl_print_node_inner(node->children[1]);
            printf(")");
            break;
        case CFL_NODE_IF:
            printf("if (");
            cfl_print_node_inner(node->children[0]);
            printf(") then (");
            cfl_print_node_inner(node->children[1]);
            printf(") else (");
            cfl_print_node_inner(node->children[2]);
            printf(")");
            break;
        case CFL_NODE_LET_REC:
            printf("let rec ");
            cfl_print_node_inner(node->children[0]);
            printf(" ");
            cfl_print_node_inner(node->children[1]);
            printf(" = (");
            cfl_print_node_inner(node->children[2]);
            printf(") in (");
            cfl_print_node_inner(node->children[3]);
            printf(")");
            break;
        case CFL_NODE_PUSH:
            printf("(");
            cfl_print_node_inner(node->children[0]);
            printf(") : (");
            cfl_print_node_inner(node->children[1]);
            printf(")");
            break;
        case CFL_NODE_CONCATENATE:
            printf("(");
            cfl_print_node_inner(node->children[0]);
            printf(") ++ (");
            cfl_print_node_inner(node->children[1]);
            printf(")");
            break;
        case CFL_NODE_CASE:
            printf("case (");
            cfl_print_node_inner(node->children[0]);
            printf(") of [] -> (");
            cfl_print_node_inner(node->children[1]);
            printf(") | (");
            cfl_print_node_inner(node->children[2]);
            printf(" : ");
            cfl_print_node_inner(node->children[3]);
            printf(") -> (");
            cfl_print_node_inner(node->children[4]);
            printf(")");
            break;
        default:
            break;
    }
}

void cfl_print_node(cfl_node* node)
{
    cfl_print_node_inner(node);

    printf("\n");
}
