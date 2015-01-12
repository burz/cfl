#include "cfl_parser.h"
#include "cfl_type.h"
#include "cfl_eval.h"

#include <stdio.h>
#include <string.h>

static char usage[] = "USAGE: cfl filename\n"
                      "           -ast filename\n"
                      "           -type filename";

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        fprintf(stderr, "%s\n", usage);

        return 1;
    }

    cfl_node* node;

    if(argc > 2)
    {
        if(!strcmp(argv[1], "-ast"))
        {
            node = cfl_parse_file(argv[2]);

            if(!node)
                return 1;

            cfl_print_node(node);

            cfl_free_node(node);
        }
        else if(argc > 2 && !strcmp(argv[1], "-type"))
        {
            node = cfl_parse_file(argv[2]);

            if(!node)
                return 1;

            cfl_type* type = cfl_typecheck(node);

            if(!type)
            {
                cfl_free_node(node);

                return 1;
            }

            cfl_print_type(type);

            cfl_free_type(type);

            cfl_free_node(node);
        }
        else
        {
            fprintf(stderr, "ERROR: unrecognized option \"%s\"\n%s\n",
                            argv[1], usage);

            return 1;
        }
    }
    else
    {
        node = cfl_parse_file(argv[1]);

        if(!node)
            return 1;

        cfl_type* type = cfl_typecheck(node);

        if(!type)
        {
            cfl_free_node(node);

            return 1;
        }

        cfl_free_type(type);

        if(!cfl_evaluate(node))
        {
            cfl_free_node(node);

            return 1;
        }

        cfl_print_node(node);

        cfl_free_node(node);
    }

    return 0;
}
