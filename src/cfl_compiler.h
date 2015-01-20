#ifndef _CFL_COMPILER_H_
#define _CFL_COMPILER_H_

extern "C" {
#include "cfl_program.h"
}

#include <string>

bool cfl_compile(cfl_program* program, std::string& destination_file);

#endif
