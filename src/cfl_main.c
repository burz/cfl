#include "cfl_malloc.h"
#include "cfl_parser.h"
#include "cfl_program.h"
#include "cfl_type.h"
#include "cfl_eval.h"

#include <stdio.h>
#include <string.h>

#define EQUATION_HASH_TABLE_LENGTH 503

static char usage[] = "USAGE: cfl filename\n"
                      "           -ast filename\n"
                      "           -type filename\n"
                      "           -single filename";

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        fprintf(stderr, "%s\n", usage);

        return 1;
    }

    if(!cfl_initialize_malloc())
        return 1;

    cfl_program* program;

    if(argc > 2)
    {
        if(!strcmp(argv[1], "-ast"))
        {
            program = cfl_parse_file(argv[2]);

            if(!program)
            {
                cfl_cleanup_malloc();

                return 1;
            }

            cfl_print_program(program);
        }
        else if(argc > 2 && !strcmp(argv[1], "-type"))
        {
            program = cfl_parse_file(argv[2]);

            if(!program)
            {
                cfl_cleanup_malloc();

                return 1;
            }

            if(!cfl_typecheck(program, EQUATION_HASH_TABLE_LENGTH))
            {
                cfl_free_program(program);
                cfl_cleanup_malloc();

                return 1;
            }

            cfl_print_program_type(program);
        }
        else if(argc > 2 && !strcmp(argv[1], "-single"))
        {
            program = cfl_parse_file(argv[2]);

            if(!program)
                return 1;

            if(!cfl_typecheck(program, EQUATION_HASH_TABLE_LENGTH))
            {
                cfl_free_program(program);
                cfl_cleanup_malloc();

                return 1;
            }

            if(!cfl_initialize_eval())
            {
                cfl_free_program(program);
                cfl_cleanup_malloc();

                return 1;
            }

            if(!cfl_evaluate_program(program, false))
            {
                cfl_free_program(program);
                cfl_cleanup_eval();
                cfl_cleanup_malloc();

                return 1;
            }

            cfl_cleanup_eval();

            cfl_print_node(program->main);
        }
        else
        {
            fprintf(stderr, "ERROR: unrecognized option \"%s\"\n%s\n",
                            argv[1], usage);

            cfl_cleanup_malloc();

            return 1;
        }
    }
    else
    {
        program = cfl_parse_file(argv[1]);

        if(!program)
            return 1;

        if(!cfl_typecheck(program, EQUATION_HASH_TABLE_LENGTH))
        {
            cfl_free_program(program);
            cfl_cleanup_malloc();

            return 1;
        }

        if(!cfl_initialize_eval())
        {
            cfl_free_program(program);
            cfl_cleanup_malloc();

            return 1;
        }

        if(!cfl_evaluate_program(program, true))
        {
            cfl_free_program(program);
            cfl_cleanup_eval();
            cfl_cleanup_malloc();

            return 1;
        }

        cfl_cleanup_eval();

        cfl_print_node(program->main);
    }

    cfl_free_program(program);
    cfl_cleanup_malloc();

    return 0;
}
