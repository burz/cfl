#include "cfl_type.h"

extern void* cfl_type_malloc(size_t size);

unsigned long long cfl_hash_type(cfl_type* type)
{
    if(type->type == CFL_TYPE_VARIABLE)
        return 2 * type->id;
    else if(type->type == CFL_TYPE_BOOL)
        return 3;
    else if(type->type == CFL_TYPE_INTEGER)
        return 5;
    else if(type->type == CFL_TYPE_CHAR)
        return 7;
    else if(type->type == CFL_TYPE_LIST)
        return 9 * cfl_hash_type(type->input);
    else if(type->type == CFL_TYPE_TUPLE)
    {
        unsigned long long product = 1;
        int i = 0;
        for( ; i < type->id; ++i)
            product *= cfl_hash_type(((cfl_type**) type->input)[i]);
        return 11 * product;
    }
    else if(type->type == CFL_TYPE_ARROW)
        return 13 * cfl_hash_type(type->input) *
                    cfl_hash_type(type->output);
    else
        return 0;
}

static void cfl_delete_type_list(cfl_type_list_element* list_head)
{
    while(list_head->next)
    {
        cfl_type_list_element* list_pos = list_head->next;

        list_head->next = list_pos->next;

        cfl_free_type(list_pos->type);

        free(list_pos);
    }
}

static cfl_type_list_element* cfl_copy_type_list(cfl_type_list_element* list)
{
    cfl_type_list_element new_list;
    cfl_type_list_element* pos = &new_list;

    while(list)
    {
        pos->next = cfl_type_malloc(sizeof(cfl_type_list_element));

        if(!pos->next)
        {
            cfl_delete_type_list(&new_list);

            return 0;
        }

        pos->next->type = cfl_type_malloc(sizeof(cfl_type));

        if(!pos->next->type)
        {
            free(pos->next);

            pos->next = 0;

            cfl_delete_type_list(&new_list);

            return 0;
        }

        if(!cfl_copy_type(pos->next->type, list->type))
        {
            free(pos->next->type);
            free(pos->next);

            pos->next = 0;

            cfl_delete_type_list(&new_list);

            return 0;
        }

        pos = pos->next;
        list = list->next;
    }

    pos->next = 0;

    return new_list.next;
}

static void cfl_delete_type_hash(cfl_type_hash_element* hash_head)
{
    while(hash_head->next)
    {
        cfl_type_hash_element* temp = hash_head->next;

        hash_head->next = temp->next;

        cfl_free_type(temp->type);
        cfl_delete_type_list(&temp->variable_head);
        cfl_delete_type_list(&temp->typed_head);

        free(temp);
    }
}

static cfl_type_hash_element* cfl_copy_type_hash(cfl_type_hash_element* hash)
{
    cfl_type_hash_element new_hash;
    cfl_type_hash_element* pos = &new_hash;

    while(hash)
    {
        pos->next = cfl_type_malloc(sizeof(cfl_type_hash_element));

        if(!pos->next)
        {
            cfl_delete_type_hash(&new_hash);

            return 0;
        }

        pos->next->type = cfl_type_malloc(sizeof(cfl_type));

        if(!pos->next->type)
        {
            free(pos->next);

            pos->next = 0;

            cfl_delete_type_hash(&new_hash);

            return 0;
        }

        if(!cfl_copy_type(pos->next->type, hash->type))
        {
            free(pos->next->type);
            free(pos->next);

            pos->next = 0;

            cfl_delete_type_hash(&new_hash);

            return 0;
        }

        if(hash->variable_head.next)
        {
            pos->next->variable_head.next = cfl_copy_type_list(hash->variable_head.next);

            if(!pos->next->variable_head.next)
            {
                free(pos->next->type);
                free(pos->next);

                pos->next = 0;

                cfl_delete_type_hash(&new_hash);

                return 0;
            }
        }
        else
            pos->next->variable_head.next = 0;

        if(hash->typed_head.next)
        {
            pos->next->typed_head.next = cfl_copy_type_list(hash->typed_head.next);

            if(!pos->next->typed_head.next)
            {
                cfl_delete_type_list(&pos->next->typed_head);
                free(pos->next->type);
                free(pos->next);

                pos->next = 0;

                cfl_delete_type_hash(&new_hash);

                return 0;
            }
        }
        else
            pos->next->typed_head.next = 0;

        pos = pos->next;
        hash = hash->next;
    }

    pos->next = 0;

    return new_hash.next;
}

cfl_type_equations* cfl_copy_type_equations(cfl_type_equations* equations)
{
    cfl_type_equations* result = cfl_type_malloc(sizeof(cfl_type_equations));

    if(!result)
        return 0;

    if(!cfl_initialize_type_equations(result, equations->equation_hash_table_length))
    {
        free(result);

        return 0;
    }

    int i = 0;
    for( ; i < equations->equation_hash_table_length; ++i)
    {
        if(!equations->hash_table[i].next)
        {
            result->hash_table[i].next = 0;

            continue;
        }

        result->hash_table[i].next = cfl_copy_type_hash(equations->hash_table[i].next);

        if(!result->hash_table[i].next)
        {
            cfl_delete_type_equations(result);

            return 0;
        }
    }

    return result;
}

bool cfl_initialize_type_equations(
        cfl_type_equations* equations,
        unsigned int equation_hash_table_length)
{
    equations->hash_table = cfl_type_malloc(sizeof(cfl_type_hash_element) *
                                            equation_hash_table_length);

    if(!equations->hash_table)
        return false;

    equations->equation_hash_table_length = equation_hash_table_length;

    int i = 0;
    for( ; i < equation_hash_table_length; ++i)
        equations->hash_table[i].next = 0;

    return true;
}

void cfl_delete_type_equations(cfl_type_equations* equations)
{
    int i = 0;
    for( ; i < equations->equation_hash_table_length; ++i)
        while(equations->hash_table[i].next)
        {
            cfl_type_hash_element* pos = equations->hash_table[i].next;

            equations->hash_table[i].next = pos->next;

            cfl_free_type(pos->type);

            cfl_delete_type_list(&pos->variable_head);
            cfl_delete_type_list(&pos->typed_head);

            free(pos);
        }

    free(equations->hash_table);
}

int cfl_add_type_equation(
        cfl_type_equations* equations,
        cfl_type* left,
        cfl_type* right)
{
    unsigned long long left_hash = cfl_hash_type(left) %
                                   equations->equation_hash_table_length;

    cfl_type_hash_element* pos = &equations->hash_table[left_hash];

    while(pos->next)
    {
        int result = cfl_compare_type(left, pos->next->type);

        if(result > 0)
            break;
        else if(!result)
        {
            cfl_free_type(left);

            cfl_type_list_element* list_pos;

            if(right->type == CFL_TYPE_VARIABLE)
                list_pos = &pos->next->variable_head;
            else
                list_pos = &pos->next->typed_head;

            while(list_pos->next)
            {
                int result = cfl_compare_type(right, list_pos->next->type);

                if(result > 0)
                    break;
                else if(!result)
                {
                    cfl_free_type(right);

                    return -1;
                }

                list_pos = list_pos->next;
            }

            cfl_type_list_element* new_list_element =
                    cfl_type_malloc(sizeof(cfl_type_list_element));

            if(!new_list_element)
            {
                cfl_free_type(right);

                return 0;
            }

            new_list_element->type = right;
            new_list_element->next = list_pos->next;

            list_pos->next = new_list_element;

            return 1;
        }

        pos = pos->next;
    }

    cfl_type_hash_element* new_element =
            cfl_type_malloc(sizeof(cfl_type_hash_element));

    if(!new_element)
    {
        cfl_free_type(left);
        cfl_free_type(right);

        return 0;
    }

    new_element->type = left;

    cfl_type_list_element* new_list_element =
            cfl_type_malloc(sizeof(cfl_type_list_element));

    if(!new_list_element)
    {
        cfl_free_type(left);
        cfl_free_type(right);
        free(new_element);

        return 0;
    }

    new_list_element->type = right;
    new_list_element->next = 0;

    if(right->type == CFL_TYPE_VARIABLE)
    {
        new_element->variable_head.next = new_list_element;
        new_element->typed_head.next = 0;
    }
    else
    {
        new_element->variable_head.next = 0;
        new_element->typed_head.next = new_list_element;
    }

    new_element->next = pos->next;

    pos->next = new_element;

    return 1;
}

int cfl_add_type_equations(
        cfl_type_equations* equations,
        cfl_type* left,
        cfl_type* right)
{
    cfl_type* left_copy = cfl_type_malloc(sizeof(cfl_type));

    if(!left_copy)
    {
        cfl_free_type(left);
        cfl_free_type(right);

        return 0;
    }

    if(!cfl_copy_type(left_copy, left))
    {
        cfl_free_type(left);
        cfl_free_type(right);
        free(left_copy);

        return 0;
    }

    cfl_type* right_copy = cfl_type_malloc(sizeof(cfl_type));

    if(!right_copy)
    {
        cfl_free_type(left);
        cfl_free_type(right);
        cfl_free_type(left_copy);

        return 0;
    }

    if(!cfl_copy_type(right_copy, right))
    {
        cfl_free_type(left);
        cfl_free_type(right);
        cfl_free_type(left_copy);
        free(right_copy);

        return 0;
    }

    int result0 = cfl_add_type_equation(equations, left_copy, right_copy);

    if(!result0)
    {
        cfl_free_type(left);
        cfl_free_type(right);

        return 0;
    }

    int result1 = cfl_add_type_equation(equations, right, left);

    if(!result1)
        return 0;
    else if(result0 > 0 || result1 > 0)
        return 1;
    else
        return -1;
}

int cfl_add_type_equations_from_copies(
        cfl_type_equations* equations,
        cfl_type* left,
        cfl_type* right)
{
    cfl_type* left_copy = cfl_type_malloc(sizeof(cfl_type));

    if(!left_copy)
        return 0;

    if(!cfl_copy_type(left_copy, left))
    {
        free(left_copy);

        return 0;
    }

    cfl_type* right_copy = cfl_type_malloc(sizeof(cfl_type));

    if(!right_copy)
    {
        cfl_free_type(left_copy);

        return 0;
    }

    if(!cfl_copy_type(right_copy, right))
    {
        cfl_free_type(left_copy);
        free(right_copy);

        return 0;
    }

    return cfl_add_type_equations(equations, left_copy, right_copy);
}

static int cfl_add_transative_type_equations(
        cfl_type_equations* equations,
        cfl_type* left,
        cfl_type* right)
{
    unsigned long long hash = cfl_hash_type(right);

    cfl_type_hash_element* pos =
            equations->hash_table[hash % equations->equation_hash_table_length].next;

    bool changes = false;

    while(pos)
    {
        if(!cfl_compare_type(right, pos->type))
        {
            cfl_type_list_element* list_pos = pos->variable_head.next;

            while(list_pos)
            {
                int result = cfl_add_type_equations_from_copies(equations,
                                                                left,
                                                                list_pos->type);

                if(!result)
                    return 0;
                else if(result > 0)
                    changes = true;

                list_pos = list_pos->next;
            }

            list_pos = pos->typed_head.next;

            while(list_pos)
            {
                int result = cfl_add_type_equations_from_copies(equations,
                                                                left,
                                                                list_pos->type);

                if(!result)
                    return 0;
                else if(result > 0)
                    changes = true;

                list_pos = list_pos->next;
            }

            break;
        }

        pos = pos->next;
    }

    if(changes)
        return 1;
    else
        return -1;
}

static int cfl_close_type_equations_iteration(
        cfl_type_equations* equations,
        cfl_type_hash_element* element)
{
    bool changes = false;

    cfl_type_list_element* pos = element->variable_head.next;

    while(pos)
    {
        int result = cfl_add_transative_type_equations(equations,
                                                       element->type,
                                                       pos->type);

        if(!result)
            return 0;
        else if(result > 0)
            changes = true;

        pos = pos->next;
    }

    pos = element->typed_head.next;

    if(element->type->type == CFL_TYPE_LIST)
    {
        while(pos)
        {
            if(!pos->type == CFL_TYPE_LIST)
            {
                cfl_type_error_failure();

                return 0;
            }

            int result = cfl_add_type_equations_from_copies(equations,
                                                            element->type->input,
                                                            pos->type->input);

            if(!result)
                return 0;
            else if(result > 0)
                changes = true;

            result = cfl_add_transative_type_equations(equations,
                                                       element->type,
                                                       pos->type);

            if(!result)
                return 0;
            else if(result > 0)
                changes = true;

            pos = pos->next;
        }
    }
    else if(element->type->type == CFL_TYPE_TUPLE)
    {
        while(pos)
        {
            if(!pos->type == CFL_TYPE_TUPLE || element->type->id != pos->type->id)
            {
                cfl_type_error_failure();

                return 0;
            }

            int result;
            int i = 0;

            for( ; i < element->type->id; ++i)
            {
                 result = cfl_add_type_equations_from_copies(
                        equations, ((cfl_type**) element->type->input)[i],
                                   ((cfl_type**) element->type->input)[i]);

                if(!result)
                    return 0;
                else if(result > 0)
                    changes = true;
            }

            result = cfl_add_transative_type_equations(equations,
                                                       element->type,
                                                       pos->type);

            if(!result)
                return 0;
            else if(result > 0)
                changes = true;

            pos = pos->next;
        }
    }
    else if(element->type->type == CFL_TYPE_ARROW)
    {
        while(pos)
        {
            if(!pos->type == CFL_TYPE_ARROW)
            {
                cfl_type_error_failure();

                return 0;
            }

            int result0 = cfl_add_type_equations_from_copies(equations,
                                                             element->type->input,
                                                             pos->type->input);

            if(!result0)
                return 0;

            int result1 = cfl_add_type_equations_from_copies(equations,
                                                             element->type->output,
                                                             pos->type->output);

            if(!result1)
                return 0;
            else if(result0 > 0 || result1 > 0)
                changes = true;

            result0 = cfl_add_transative_type_equations(equations,
                                                        element->type,
                                                        pos->type);

            if(!result0)
                return 0;
            else if(result0 > 0)
                changes = true;

            pos = pos->next;
        }
    }
    else
        while(pos)
        {
            int result = cfl_add_transative_type_equations(equations,
                                                           element->type,
                                                           pos->type);

            if(!result)
                return 0;
            else if(result > 0)
                changes = true;

            pos = pos->next;
        }

    if(changes)
        return 1;
    else
        return -1;
}

bool cfl_close_type_equations(cfl_type_equations* equations)
{
    bool changes = true;

    while(changes)
    {
        changes = false;

        int i = 0;
        for( ; i < equations->equation_hash_table_length; ++i)
        {
            if(!equations->hash_table[i].next)
                continue;

            cfl_type_hash_element* pos = equations->hash_table[i].next;

            while(pos)
            {
                int result = cfl_close_type_equations_iteration(equations, pos);

                if(!result)
                    return false;
                else if(result > 0)
                    changes = true;

                pos = pos->next;
            }
        }
    }

    return true;
}

bool cfl_are_type_equations_consistent(cfl_type_equations* equations)
{
    int i = 0;
    for( ; i < equations->equation_hash_table_length; ++i)
    {
        cfl_type_hash_element* pos = equations->hash_table[i].next;

        while(pos)
        {
            if(pos->type->type == CFL_TYPE_BOOL)
            {
                cfl_type_list_element* list_pos = pos->typed_head.next;

                while(list_pos)
                {
                    if(list_pos->type->type != CFL_TYPE_BOOL)
                        return false;

                    list_pos = list_pos->next;
                }
            }
            else if(pos->type->type == CFL_TYPE_INTEGER)
            {
                cfl_type_list_element* list_pos = pos->typed_head.next;

                while(list_pos)
                {
                    if(list_pos->type->type != CFL_TYPE_INTEGER)
                        return false;

                    list_pos = list_pos->next;
                }
            }
            else if(pos->type->type == CFL_TYPE_CHAR)
            {
                cfl_type_list_element* list_pos = pos->typed_head.next;

                while(list_pos)
                {
                    if(list_pos->type->type != CFL_TYPE_CHAR)
                        return false;

                    list_pos = list_pos->next;
                }
            }
            else if(pos->type->type == CFL_TYPE_LIST)
            {
                cfl_type_list_element* list_pos = pos->typed_head.next;

                while(list_pos)
                {
                    if(list_pos->type->type != CFL_TYPE_LIST)
                        return false;

                    list_pos = list_pos->next;
                }
            }
            else if(pos->type->type == CFL_TYPE_TUPLE)
            {
                cfl_type_list_element* list_pos = pos->typed_head.next;

                while(list_pos)
                {
                    if(list_pos->type->type != CFL_TYPE_TUPLE)
                        return false;

                    list_pos = list_pos->next;
                }
            }
            else if(pos->type->type == CFL_TYPE_ARROW)
            {
                cfl_type_list_element* list_pos = pos->typed_head.next;

                while(list_pos)
                {
                    if(list_pos->type->type != CFL_TYPE_ARROW)
                        return false;

                    list_pos = list_pos->next;
                }
            }

            pos = pos->next;
        }
    }

    return true;
}

bool cfl_simplify_type(cfl_type_equations* equations, cfl_type* node)
{
    if(node->type == CFL_TYPE_BOOL ||
       node->type == CFL_TYPE_INTEGER ||
       node->type == CFL_TYPE_CHAR)
        return true;
    else if(node->type == CFL_TYPE_LIST)
        return cfl_simplify_type(equations, node->input);
    else if(node->type == CFL_TYPE_TUPLE)
    {
        int i = 0;
        for( ; i < node->id; ++i)
            if(!cfl_simplify_type(equations, ((cfl_type**) node->input)[i]))
                return false;

        return true;
    }
    else if(node->type == CFL_TYPE_ARROW)
    {
        return cfl_simplify_type(equations, node->input) &&
               cfl_simplify_type(equations, node->output);
    }

    unsigned int best_id = node->id;
    unsigned long long hash = cfl_hash_type(node);
    cfl_type_hash_element* pos =
            equations->hash_table[hash % equations->equation_hash_table_length].next;

    while(pos)
    {
        int result = cfl_compare_type(node, pos->type);

        if(result > 0)
            break;
        else if(!result)
        {
            if(!pos->typed_head.next)
            {
                if(pos->variable_head.next &&
                   pos->variable_head.next->type->id < best_id)
                    best_id = pos->variable_head.next->type->id;
            }
            else if(!cfl_copy_type(node, pos->typed_head.next->type))
                return false;
            else
                return cfl_simplify_type(equations, node);
        }

        pos = pos->next;
    }

    node->id = best_id;

    return true;
}
