#ifndef _CFL_COMPILER_H_
#define _CFL_COMPILER_H_

extern "C" {
#include "cfl_program.h"
}

bool cfl_compile(cfl_program* program, char* destination_file);

#endif
