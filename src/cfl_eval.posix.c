#include "cfl_eval.h"
#include "cfl_malloc.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define MAX_THREADS 6

extern void* cfl_eval_malloc(size_t size);

static pthread_mutex_t cfl_eval_mutex;
static int available_threads = MAX_THREADS;

bool cfl_initialize_eval(void)
{
    srand(time(0));

    if(pthread_mutex_init(&cfl_eval_mutex, 0))
    {
        fprintf(stderr, "Error: could not create an evaluation mutex\n");

        return false;
    }

    return true;
}

void cfl_cleanup_eval(void)
{
    pthread_mutex_destroy(&cfl_eval_mutex);
}

static bool cfl_can_multithread(void)
{
    bool result = false;

    pthread_mutex_lock(&cfl_eval_mutex);

    if(available_threads)
    {
        --available_threads;

        result = true;
    }

    pthread_mutex_unlock(&cfl_eval_mutex);

    return result;
}

static void cfl_increment_available_threads(void)
{
    pthread_mutex_lock(&cfl_eval_mutex);

    ++available_threads;

    pthread_mutex_unlock(&cfl_eval_mutex);
}

static bool cfl_evaluate_multithreaded(cfl_node* node, cfl_definition_list* definitions);

typedef struct {
    cfl_node* node;
    cfl_definition_list* definitions;
} cfl_evaluate_thread_argument;

static void* cfl_evaluate_thread(void* argument)
{
    cfl_evaluate_thread_argument* arg =
        *((cfl_evaluate_thread_argument**) argument);

    if(!cfl_evaluate_multithreaded(arg->node, arg->definitions))
        return 0;

    cfl_increment_available_threads();

    return (void*) 1;
}

static bool cfl_evaluate_in_parallel(
        pthread_t* thread,
        void* argument)
{
    if(pthread_create(thread, 0, &cfl_evaluate_thread, &argument))
    {
        fprintf(stderr, "ERROR: Could not create a new thread.\n");

        return false;
    }

    return true;
}

static bool cfl_wait_for_parallel_termination(pthread_t* thread)
{
    void* result;

    if(pthread_join(*thread, &result))
    {
        fprintf(stderr, "ERROR: Could not join thread\n");

        return false;
    }

    return result;
}

static bool cfl_is_basic_type(cfl_node* node)
{
    if(node->type == CFL_NODE_BOOL ||
       node->type == CFL_NODE_INTEGER ||
       node->type == CFL_NODE_CHAR ||
       node->type == CFL_NODE_FUNCTION)
        return true;

    return false;
}

static bool cfl_evaluate_binary_children(cfl_node* node, cfl_definition_list* definitions)
{
    if(node->children[0]->type == CFL_NODE_VARIABLE)
        return cfl_evaluate_multithreaded(node->children[0], definitions) &&
               cfl_evaluate_multithreaded(node->children[0], definitions);
    else if(node->children[1]->type == CFL_NODE_VARIABLE)
        return cfl_evaluate_multithreaded(node->children[0], definitions) &&
               cfl_evaluate_multithreaded(node->children[0], definitions);
    else if(!cfl_is_basic_type(node->children[0]))
    {
        if(!cfl_is_basic_type(node->children[1]))
        {
            if(cfl_can_multithread())
            {
                pthread_t thread;
                cfl_evaluate_thread_argument argument;
                argument.node = node->children[0];
                argument.definitions = definitions;

                if(!cfl_evaluate_in_parallel(&thread, &argument))
                    return false;

                bool success = cfl_evaluate_multithreaded(node->children[1], definitions);

                if(!cfl_wait_for_parallel_termination(&thread) || !success)
                    return false;
            }
            else
                return cfl_evaluate_multithreaded(node->children[0], definitions) &&
                       cfl_evaluate_multithreaded(node->children[1], definitions);
        }
        else
            return cfl_evaluate_multithreaded(node->children[0], definitions);
    }
    else if(!cfl_is_basic_type(node->children[1]))
        return cfl_evaluate_multithreaded(node->children[1], definitions);

    return true;
}

static bool cfl_evaluate_multithreaded(cfl_node* node, cfl_definition_list* definitions)
{
    if(node->type == CFL_NODE_VARIABLE)
    {
        cfl_definition_list* pos = definitions;

        while(pos && strcmp(pos->name->data, node->data))
            pos = pos->next;

        if(pos)
        {
            cfl_node* result = cfl_copy_new_node(pos->definition);

            if(!result)
                return false;

            cfl_delete_node(node);

            *node = *result;

            cfl_free(result);
        }
    }
    else if(node->type == CFL_NODE_LIST)
    {
        cfl_list_node* pos = node->data;

        while(pos)
        {
            if(!cfl_evaluate_multithreaded(pos->node, definitions))
                return false;

            pos = pos->next;
        }
    }
    else if(node->type == CFL_NODE_TUPLE)
    {
        int i = 0;
        for( ; i < node->number_of_children; ++i)
            if(!cfl_evaluate_multithreaded(node->children[i], definitions))
                return false;
    }
    else if(node->type == CFL_NODE_AND)
    {
        if(!cfl_evaluate_multithreaded(node->children[0], definitions))
            return false;

        if(!*((bool*) node->children[0]->data))
        {
            if(!cfl_reinitialize_node_bool(node, false))
                return false;

            return true;
        }

        if(!cfl_evaluate_multithreaded(node->children[1], definitions))
            return false;

        bool result = *((bool*) node->children[1]->data);

        if(!cfl_reinitialize_node_bool(node, result))
            return false;
    }
    else if(node->type == CFL_NODE_OR)
    {
        if(!cfl_evaluate_multithreaded(node->children[0], definitions))
            return false;

        if(*((bool*) node->children[0]->data))
        {
            if(!cfl_reinitialize_node_bool(node, true))
                return false;

            return true;
        }

        if(!cfl_evaluate_multithreaded(node->children[1], definitions))
            return false;

        bool result = *((bool*) node->children[1]->data);

        if(!cfl_reinitialize_node_bool(node, result))
            return false;
    }
    else if(node->type == CFL_NODE_NOT)
    {
        if(!cfl_evaluate_multithreaded(node->children[0], definitions))
            return false;

        bool result = !*((bool*) node->children[0]->data);

        if(!cfl_reinitialize_node_bool(node, result))
            return false;
    }
    else if(node->type == CFL_NODE_ADD)
    {
        if(!cfl_evaluate_binary_children(node, definitions))
            return false;

        int result = *((int*) node->children[0]->data) +
                     *((int*) node->children[1]->data);

        if(!cfl_reinitialize_node_integer(node, result))
            return false;
    }
    else if(node->type == CFL_NODE_MULTIPLY)
    {
        if(!cfl_evaluate_binary_children(node, definitions))
            return false;

        int result = *((int*) node->children[0]->data) *
                     *((int*) node->children[1]->data);

        if(!cfl_reinitialize_node_integer(node, result))
            return false;
    }
    else if(node->type == CFL_NODE_DIVIDE)
    {
        if(!cfl_evaluate_binary_children(node, definitions))
            return false;

        if(*((int*) node->children[1]->data) == 0)
        {
            fprintf(stderr, "EVALUATION ERROR: Division by zero\n");

            return false;
        }

        int result = *((int*) node->children[0]->data) /
                     *((int*) node->children[1]->data);

        if(!cfl_reinitialize_node_integer(node, result))
            return false;
    }
    else if(node->type == CFL_NODE_EQUAL)
    {
        if(!cfl_evaluate_binary_children(node, definitions))
            return false;

        bool result = *((int*) node->children[0]->data) ==
                      *((int*) node->children[1]->data);

        if(!cfl_reinitialize_node_bool(node, result))
            return false;
    }
    else if(node->type == CFL_NODE_LESS)
    {
        if(!cfl_evaluate_binary_children(node, definitions))
            return false;

        bool result = *((int*) node->children[0]->data) <
                      *((int*) node->children[1]->data);

        if(!cfl_reinitialize_node_bool(node, result))
            return false;
    }
    else if(node->type == CFL_NODE_APPLICATION)
    {
        if(!cfl_evaluate_binary_children(node, definitions))
            return false;

        if(node->children[0]->type == CFL_NODE_VARIABLE &&
           !strcmp(node->children[0]->data, "random"))
        {
            int result = cfl_evaluate_global_function_random(
                    *((int*) node->children[1]->data));

            cfl_reinitialize_node_integer(node, result);

            return true;
        }

        if(!cfl_complex_substitute(node->children[0]->children[1],
                                   node->children[0]->children[0],
                                   node->children[1]))
            return false;

        cfl_free_node(node->children[0]->children[0]);
        cfl_free_node(node->children[1]);

        cfl_node* result = node->children[0]->children[1];

        cfl_free(node->children[0]->children);
        cfl_free(node->children[0]);
        cfl_free(node->children);

        *node = *result;

        cfl_free(result);

        return cfl_evaluate_multithreaded(node, definitions);
    }
    else if(node->type == CFL_NODE_IF)
    {
        if(!cfl_evaluate_multithreaded(node->children[0], definitions))
            return false;

        if(*((bool*) node->children[0]->data))
        {
            if(!cfl_evaluate_multithreaded(node->children[1], definitions))
                return false;

            cfl_node** temp = node->children;

            cfl_free_node(node->children[0]);
            cfl_free_node(node->children[2]);

            *node = *temp[1];

            cfl_free(temp[1]);
            cfl_free(temp);
        }
        else
        {
            if(!cfl_evaluate_multithreaded(node->children[2], definitions))
                return false;

            cfl_node** temp = node->children;

            cfl_free_node(node->children[0]);
            cfl_free_node(node->children[1]);

            *node = *temp[2];

            cfl_free(temp[2]);
            cfl_free(temp);
        }
    }
    else if(node->type == CFL_NODE_LET_REC)
    {
        cfl_node* temp0 = cfl_copy_new_node(node->children[0]);

        if(!temp0)
            return false;

        cfl_node* temp1 = cfl_copy_new_node(node->children[1]);

        if(!temp1)
        {
            cfl_free_node(temp0);

            return false;
        }

        cfl_node* temp2 = cfl_create_new_node_application(temp0, temp1);

        if(!temp2)
            return false;

        temp0 = cfl_copy_new_node(node->children[0]);

        if(!temp0)
        {
            cfl_free_node(temp2);

            return false;
        }

        temp1 = cfl_copy_new_node(node->children[1]);

        if(!temp1)
        {
            cfl_free_node(temp0);
            cfl_free_node(temp2);

            return false;
        }

        cfl_node* temp3 = cfl_copy_new_node(node->children[2]);

        if(!temp3)
        {
            cfl_free_node(temp0);
            cfl_free_node(temp1);
            cfl_free_node(temp2);

            return false;
        }

        cfl_node* temp4 = cfl_create_new_node_let_rec(temp0, temp1, temp3, temp2);

        if(!temp4)
            return false;

        temp0 = cfl_copy_new_node(node->children[1]);

        if(!temp0)
        {
            cfl_free_node(temp4);

            return false;
        }

        temp1 = cfl_create_new_node_function(temp0, temp4);

        if(!temp1)
            return false;

        if(!cfl_substitute(node->children[2], node->children[0]->data, temp1))
        {
            cfl_free_node(temp1);

            return false;
        }

        cfl_free_node(temp1);

        temp0 = cfl_create_new_node_function(node->children[1], node->children[2]);

        if(!temp0)
            return false;

        if(!cfl_substitute(node->children[3], node->children[0]->data, temp0))
        {
            cfl_free_node(temp0);
            cfl_free_node(node->children[0]);
            cfl_free_node(node->children[3]);
            cfl_free(node->children);
            node->number_of_children = 0;

            return false;
        }

        cfl_free_node(temp0);

        temp0 = node->children[3];

        cfl_free_node(node->children[0]);
        cfl_free(node->children);

        *node = *temp0;

        cfl_free(temp0);

        return cfl_evaluate_multithreaded(node, definitions);
    }
    else if(node->type == CFL_NODE_PUSH)
    {
        if(!cfl_evaluate_binary_children(node, definitions))
            return false;

        cfl_node* list_node = node->children[1];

        if(!list_node->data)
        {
            list_node->data = cfl_eval_malloc(sizeof(cfl_list_node));

            if(!list_node->data)
                return false;

            ((cfl_list_node*) list_node->data)->next = 0;
        }
        else
        {
            cfl_list_node* new_node = cfl_eval_malloc(sizeof(cfl_list_node));

            if(!new_node)
                return false;

            new_node->next = list_node->data;

            list_node->data = new_node;
        }

        ((cfl_list_node*) list_node->data)->node = node->children[0];

        cfl_free(node->children);

        *node = *list_node;

        cfl_free(list_node);
    }
    else if(node->type == CFL_NODE_CONCATENATE)
    {
        if(!cfl_evaluate_binary_children(node, definitions))
            return false;

        cfl_node* list_node = node->children[0];

        if(!list_node->data)
        {
            list_node->data = node->children[1]->data;
        }
        else
        {
            cfl_list_node* pos = list_node->data;

            while(pos->next != 0)
                pos = pos->next;

            pos->next = node->children[1]->data;
        }

        cfl_free(node->children[1]);
        cfl_free(node->children);

        *node = *list_node;

        cfl_free(list_node);
    }
    else if(node->type == CFL_NODE_CASE)
    {
        if(!cfl_evaluate_multithreaded(node->children[0], definitions))
            return false;

        if(node->children[0]->data)
        {
            cfl_node* head = ((cfl_list_node*) node->children[0]->data)->node;

            cfl_list_node* tail_list =
                ((cfl_list_node*) node->children[0]->data)->next;

            cfl_node* tail = cfl_create_new_node_list(tail_list);

            if(!tail)
                return false;

            if(!cfl_complex_substitute(node->children[4], node->children[2], head) ||
               !cfl_substitute(node->children[4], node->children[3]->data, tail))
            {
                cfl_free(tail);

                return false;
            }

            cfl_node* temp = node->children[4];

            cfl_free(tail);
            cfl_free_node(node->children[0]);
            cfl_free_node(node->children[1]);
            cfl_free_node(node->children[2]);
            cfl_free_node(node->children[3]);
            cfl_free(node->children);

            *node = *temp;

            cfl_free(temp);

            return cfl_evaluate_multithreaded(node, definitions);
        }
        else
        {
            if(!cfl_evaluate_multithreaded(node->children[1], definitions))
                return false;

            cfl_node* temp = node->children[1];

            cfl_free_node(node->children[0]);
            cfl_free_node(node->children[2]);
            cfl_free_node(node->children[3]);
            cfl_free_node(node->children[4]);
            cfl_free(node->children);

            *node = *temp;

            cfl_free(temp);
        }
    }

    return true;
}

bool cfl_eval(
        cfl_node* node,
        cfl_definition_list* definitions,
        bool multithreaded)
{
    if(multithreaded)
        return cfl_evaluate_multithreaded(node, definitions);
    else
        return cfl_evaluate(node, definitions);
}
