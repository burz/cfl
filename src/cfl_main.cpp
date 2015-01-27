extern "C" {
#include "cfl_parser.h"
#include "cfl_program.h"
#include "cfl_type.h"
#include "cfl_typed_program.h"
#include "cfl_eval.h"

#include <stdio.h>
#include <string.h>
}

#include "cfl_compiler.h"

#define EQUATION_HASH_TABLE_LENGTH 503

static char usage[] = "USAGE: cfl filename       :: compile the program\n"
                      "           -ast filename  :: print the AST of the program\n"
                      "           -type filename :: print the high level types of the "
                                                   "program\n"
                      "           -deep filename :: print all the types of the program\n"
                      "           -eval filename :: evaluate the program\n"
                      "           -jit filename  :: evaluate the program using Just-In"
                                                    "-Time compiling";

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
        else if(!strcmp(argv[1], "-type"))
        {
            program = cfl_parse_file(argv[2]);

            if(!program)
                return 1;

            if(!cfl_typecheck(program, EQUATION_HASH_TABLE_LENGTH))
            {
                cfl_free_program(program);

                return 1;
            }

            cfl_print_program_type(program);

            cfl_free_program(program);
        }
        else if(!strcmp(argv[1], "-eval"))
        {
            program = cfl_parse_file(argv[2]);

            if(!program)
                return 1;

            if(!cfl_typecheck(program, EQUATION_HASH_TABLE_LENGTH))
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
        else if(!strcmp(argv[1], "-deep"))
        {
            program = cfl_parse_file(argv[2]);

            if(!program)
                return 1;

            cfl_typed_program* typed_program =
                cfl_generate_typed_program(program, EQUATION_HASH_TABLE_LENGTH);

            if(!typed_program)
                return 1;

            cfl_print_typed_program(typed_program);

            cfl_free_typed_program(typed_program);
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

        if(!cfl_typecheck(program, EQUATION_HASH_TABLE_LENGTH))
        {
            cfl_free_program(program);

            return 1;
        }

        cfl_typed_program* typed_program =
            cfl_generate_typed_program(program, EQUATION_HASH_TABLE_LENGTH);

        if(!typed_program)
            return 1;

        std::string file = argv[1];

        size_t extension_location = file.rfind(".");

        std::string filename_head = file.substr(0, extension_location);

        Cfl::Compiler compiler;

        if(!compiler.compile(typed_program, filename_head))
            return 1;

        cfl_free_typed_program(typed_program);
    }

    return 0;
}
