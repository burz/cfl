#include "cfl_parser.h"
#include "cfl_type.h"
#include "cfl_eval.h"

#include <stdio.h>
#include <string.h>

static char usage[] = "USAGE: cfl filename\n"
                      "           -ast filename\n"
                      "           -typecheck filename";

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        fprintf(stderr, "%s\n", usage);

        return 1;
    }

    cfl_node node;

    if(argc > 2)
    {
        if(!strcmp(argv[1], "-ast"))
        {
            if(!cfl_parse_file(&node, argv[2]))
                return 1;

            cfl_print_node(&node);

            cfl_delete_node(&node);
        }
        else if(argc > 2 && !strcmp(argv[1], "-typecheck"))
        {
            if(!cfl_parse_file(&node, argv[2]))
                return 1;

            cfl_type* type = cfl_typecheck(&node);

            if(!type)
            {
                cfl_delete_node(&node);

                return 1;
            }

            cfl_print_type(type);

            cfl_free_type(type);

            cfl_delete_node(&node);
        }
        else
        {
            fprintf(stderr, "ERROR: unrecognized option \"%s\"\n%s\n",
                            argv[1], usage);

            return 1;
        }

        return 0;
    }
    else
    {
        if(!cfl_parse_file(&node, argv[1]))
            return 1;

        cfl_type* type = cfl_typecheck(&node);

        if(!type)
        {
            cfl_delete_node(&node);

            return 1;
        }

        cfl_free_type(type);

        if(!cfl_evaluate(&node))
        {
            cfl_delete_node(&node);

            return 1;
        }

        cfl_print_node(&node);

        cfl_delete_node(&node);

        return 0;
    }
}
