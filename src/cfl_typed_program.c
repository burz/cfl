#include "cfl_typed_program.h"
#include "cfl_type.h"

#include <stdio.h>
#include <string.h>

extern void* cfl_ast_malloc(size_t size);

static bool cfl_is_bound_in_complex_variable(char* name, cfl_typed_node* node)
{
    if(node->node_type == CFL_NODE_VARIABLE)
        return !strcmp(name, node->data);

    int i = 0;
    for( ; i < node->number_of_children; ++i)
        if(cfl_is_bound_in_complex_variable(name, node->children[i]))
            return true;

    return false;
}

bool cfl_is_free_in_typed_node(char* name, cfl_typed_node* node)
{
    int i;
    cfl_typed_node_list* pos;

    switch(node->node_type)
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
            if(!cfl_is_bound_in_complex_variable(name, node->children[0]))
                return cfl_is_free_in_typed_node(name, node->children[1]);
            break;
        case CFL_NODE_LIST:
            for(pos = node->data; pos; pos = pos->next)
                if(cfl_is_free_in_typed_node(name, pos->node))
                    return true;
            break;
        case CFL_NODE_LET_REC:
            if(strcmp(name, node->children[0]->data))
            {
                if(!cfl_is_bound_in_complex_variable(name, node->children[1]))
                    return cfl_is_free_in_typed_node(name, node->children[2]) ||
                           cfl_is_free_in_typed_node(name, node->children[3]);
                else
                    return cfl_is_free_in_typed_node(name, node->children[3]);
            }
            break;
        case CFL_NODE_CASE:
            if(cfl_is_bound_in_complex_variable(name, node->children[2]) ||
               !strcmp(name, node->children[3]->data))
                return cfl_is_free_in_typed_node(name, node->children[0]) ||
                       cfl_is_free_in_typed_node(name, node->children[1]);
            else
                return cfl_is_free_in_typed_node(name, node->children[0]) ||
                       cfl_is_free_in_typed_node(name, node->children[1]) ||
                       cfl_is_free_in_typed_node(name, node->children[4]);
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
                if(cfl_is_free_in_typed_node(name, node->children[i]))
                    return true;
            break;
        default:
            break;
    }

    return false;
}

cfl_typed_node* cfl_create_typed_node(
        cfl_node_type node_type,
        cfl_type* resulting_type,
        unsigned int number_of_children,
        void* data,
        cfl_typed_node** children)
{
    cfl_typed_node* result = cfl_ast_malloc(sizeof(cfl_typed_node));

    if(!result)
    {
        cfl_free_type(resulting_type);

        if(data)
            free(data);

        int i = 0;
        for( ; i < number_of_children; ++i)
        {
            cfl_free_typed_node(children[i]);
        }

        free(children);

        return 0;
    }

    result->node_type = node_type;
    result->resulting_type = resulting_type;
    result->number_of_children = number_of_children;
    result->data = data;
    result->children = children;

    return result;
}

void cfl_free_typed_node(cfl_typed_node* node)
{
    cfl_free_type(node->resulting_type);

    if(node->data)
    {
        if(node->node_type == CFL_NODE_LIST)
        {
            cfl_typed_node_list* pos = node->data;

            while(pos)
            {
                cfl_typed_node_list* temp = pos;

                pos = pos->next;

                cfl_free_typed_node(temp->node);
                free(temp);
            }
        }
        else
            free(node->data);
    }

    int i = 0;
    for( ; i < node->number_of_children; ++i)
        cfl_free_typed_node(node->children[i]);

    free(node->children);
    free(node);
}

void cfl_free_typed_node_list(cfl_typed_node_list* list)
{
    while(list)
    {
        cfl_typed_node_list* temp = list;

        list = list->next;

        cfl_free_typed_node(temp->node);
        free(temp);
    }
}

void cfl_free_typed_definition_list(cfl_typed_definition_list* list)
{
    while(list)
    {
        cfl_typed_definition_list* temp = list;

        list = list->next;

        free(temp->name);
        cfl_free_typed_node(temp->definition);
        free(temp);
    }
}

void cfl_free_typed_program(cfl_typed_program* program)
{
    cfl_free_typed_definition_list(program->definitions);
    cfl_free_typed_node(program->main);
    free(program);
}

static void cfl_print_typed_node_inner(cfl_typed_node* node)
{
    cfl_typed_node_list* pos;
    int i;

    switch(node->node_type)
    {
        case CFL_NODE_VARIABLE:
            printf("%s :: ", (char*) node->data);
            break;
        case CFL_NODE_BOOL:
            printf(*((bool*) node->data) ? "true :: " : "false :: ");
            break;
        case CFL_NODE_INTEGER:
            printf("%d :: ", *((int*) node->data));
            break;
        case CFL_NODE_CHAR:
            printf("'%c' :: ", *((char*) node->data));
            break;
        case CFL_NODE_FUNCTION:
            printf("function (");
            cfl_print_typed_node_inner(node->children[0]);
            printf(") -> (");
            cfl_print_typed_node_inner(node->children[1]);
            printf(") :: ");
            break;
        case CFL_NODE_LIST:
            if(!node->data)
                printf("[] :: ");
            else if(((cfl_typed_node_list*) node->data)->node->node_type == CFL_NODE_CHAR)
            {
                printf("\"");
                pos = node->data;
                while(pos)
                {
                    printf("%c", *((char*) pos->node->data));
                    pos = pos->next;
                }
                printf("\" :: ");
            }
            else
            {
                printf("[");
                pos = node->data;
                while(pos)
                {
                    cfl_print_typed_node_inner(pos->node);
                    if(pos->next)
                        printf(", ");
                    pos = pos->next;
                }
                printf("] :: ");
            }
            break;
        case CFL_NODE_TUPLE:
            printf("(");
            for(i = 0; i < node->number_of_children; ++i)
            {
                cfl_print_typed_node_inner(node->children[i]);

                if(i < node->number_of_children - 1)
                    printf(", ");
            }
            printf(") :: ");
            break;
        case CFL_NODE_AND:
            printf("(");
            cfl_print_typed_node_inner(node->children[0]);
            printf(") && (");
            cfl_print_typed_node_inner(node->children[1]);
            printf(") :: ");
            break;
        case CFL_NODE_OR:
            printf("(");
            cfl_print_typed_node_inner(node->children[0]);
            printf(") || (");
            cfl_print_typed_node_inner(node->children[1]);
            printf(") :: ");
            break;
        case CFL_NODE_NOT:
            printf("!(");
            cfl_print_typed_node_inner(node->children[0]);
            printf(") :: ");
            break;
        case CFL_NODE_ADD:
            printf("(");
            cfl_print_typed_node_inner(node->children[0]);
            printf(") + (");
            cfl_print_typed_node_inner(node->children[1]);
            printf(") :: ");
            break;
        case CFL_NODE_MULTIPLY:
            printf("(");
            cfl_print_typed_node_inner(node->children[0]);
            printf(") * (");
            cfl_print_typed_node_inner(node->children[1]);
            printf(") :: ");
            break;
        case CFL_NODE_DIVIDE:
            printf("(");
            cfl_print_typed_node_inner(node->children[0]);
            printf(") / (");
            cfl_print_typed_node_inner(node->children[1]);
            printf(") :: ");
            break;
        case CFL_NODE_EQUAL:
            printf("(");
            cfl_print_typed_node_inner(node->children[0]);
            printf(") == (");
            cfl_print_typed_node_inner(node->children[1]);
            printf(") :: ");
            break;
        case CFL_NODE_LESS:
            printf("(");
            cfl_print_typed_node_inner(node->children[0]);
            printf(") < (");
            cfl_print_typed_node_inner(node->children[1]);
            printf(") :: ");
            break;
        case CFL_NODE_APPLICATION:
            printf("(");
            cfl_print_typed_node_inner(node->children[0]);
            printf(") (");
            cfl_print_typed_node_inner(node->children[1]);
            printf(") :: ");
            break;
        case CFL_NODE_IF:
            printf("if (");
            cfl_print_typed_node_inner(node->children[0]);
            printf(") then (");
            cfl_print_typed_node_inner(node->children[1]);
            printf(") else (");
            cfl_print_typed_node_inner(node->children[2]);
            printf(") :: ");
            break;
        case CFL_NODE_LET_REC:
            printf("let rec (");
            cfl_print_typed_node_inner(node->children[0]);
            printf(") (");
            cfl_print_typed_node_inner(node->children[1]);
            printf(") = (");
            cfl_print_typed_node_inner(node->children[2]);
            printf(") in (");
            cfl_print_typed_node_inner(node->children[3]);
            printf(") :: ");
            break;
        case CFL_NODE_PUSH:
            printf("(");
            cfl_print_typed_node_inner(node->children[0]);
            printf(") : (");
            cfl_print_typed_node_inner(node->children[1]);
            printf(") :: ");
            break;
        case CFL_NODE_CONCATENATE:
            printf("(");
            cfl_print_typed_node_inner(node->children[0]);
            printf(") ++ (");
            cfl_print_typed_node_inner(node->children[1]);
            printf(") :: ");
            break;
        case CFL_NODE_CASE:
            printf("case (");
            cfl_print_typed_node_inner(node->children[0]);
            printf(") of [] -> (");
            cfl_print_typed_node_inner(node->children[1]);
            printf(") | ((");
            cfl_print_typed_node_inner(node->children[2]);
            printf(") : (");
            cfl_print_typed_node_inner(node->children[3]);
            printf(")) -> (");
            cfl_print_typed_node_inner(node->children[4]);
            printf(") :: ");
            break;
        default:
            break;
    }

    cfl_print_type_inner(node->resulting_type);
}

void cfl_print_typed_program(cfl_typed_program* program)
{
    cfl_typed_definition_list* pos = program->definitions;

    while(pos)
    {
        printf("%s ~>> ", pos->name);

        cfl_print_typed_node_inner(pos->definition);            

        printf("\n");

        pos = pos->next;
    }

    printf("main ~>> ");

    cfl_print_typed_node_inner(program->main);

    printf("\n");
}
