#include "cfl_type.hypothesis.h"
#include "cfl_type.h"

#include <string.h>

extern void* cfl_type_malloc(size_t size);
extern unsigned int cfl_type_get_next_id(void);
extern void cfl_reset_type_generator(void);

bool cfl_push_hypothesis(
        cfl_type_hypothesis_chain* hypothesis_head,
        char* name,
        unsigned int id)
{
    cfl_type_hypothesis_chain* new_hypothesis =
        cfl_type_malloc(sizeof(cfl_type_hypothesis_chain));

    if(!new_hypothesis)
        return false;

    new_hypothesis->name = name;
    new_hypothesis->id = id;
    new_hypothesis->next = hypothesis_head->next;

    hypothesis_head->next = new_hypothesis;

    return true;
}

void cfl_pop_hypothesis(cfl_type_hypothesis_chain* hypothesis_head)
{
    cfl_type_hypothesis_chain* temp = hypothesis_head->next;

    hypothesis_head->next = temp->next;

    free(temp);
}
