#ifndef _CFL_TYPE_ERROR_H_
#define _CFL_TYPE_ERROR_H_

void cfl_reset_type_error_flag(void);
bool cfl_get_type_error_flag(void);
void cfl_type_error_undefined_variable(char* name);
void cfl_type_error_bad_definition(char* name);
void cfl_type_error_failure(void);

#endif
