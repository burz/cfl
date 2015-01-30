#ifndef _CFL_PARSER_ERROR_H_
#define _CFL_PARSER_ERROR_H_

bool cfl_get_parse_error_flag(void);
void cfl_parse_error_unexpected_char(char x); 
void cfl_parse_error_integer_overflow(char* start, int length);
void cfl_parse_error_expected(char* expecting, char* after, char* start, char* end);
void cfl_parse_error_no_equal_after_def(char* name);
void cfl_parse_error_bad_arguments_after_def(char* name);
void cfl_parse_error_bad_division(void);
void cfl_parse_error_complex_function_name(void);
void cfl_parse_error_partial_program(void);
void cfl_parse_error_missing_main(void);
void cfl_parse_error_main_has_arguments(void);
void cfl_parse_error_redeclaration(char* name);
void cfl_parse_error_unparseable_file(char* filename);

#endif
