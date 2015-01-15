#include "cfl_parser.h"
#include "cfl_program.h"
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

    cfl_program* program;

    if(argc > 2)
    {
        if(!strcmp(argv[1], "-ast"))
        {
            program = cfl_parse_file(argv[2]);

            if(!program)
                return 1;

            cfl_print_program(program);

            cfl_free_program(program);
        }
        else if(argc > 2 && !strcmp(argv[1], "-type"))
        {
            program = cfl_parse_file(argv[2]);

            if(!program)
                return 1;

            if(!cfl_typecheck(program))
            {
                cfl_free_program(program);

                return 1;
            }

            cfl_print_program_type(program);

            cfl_free_program(program);
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
        program = cfl_parse_file(argv[1]);

        if(!program)
            return 1;

        if(!cfl_typecheck(program))
        {
            cfl_free_program(program);

            return 1;
        }

        cfl_initialize_eval();

        if(!cfl_evaluate_program(program))
        {
            cfl_free_program(program);

            return 1;
        }

        cfl_print_node(program->main);

        cfl_free_program(program);
    }

    return 0;
}
