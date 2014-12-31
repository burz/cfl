#include "cfl_ast.h"

#include <stdlib.h>

void cfl_delete_node(cfl_node* node)
{
    if(node->data)
        free(node->data);

    int i = 0;

    for( ; i < node->number_of_children; ++i)
    {
        cfl_delete_node(&node->children[i]);

        free(node);
    }

    node->type = CFL_NODE_NULL;
}
