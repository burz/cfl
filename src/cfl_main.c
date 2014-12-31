#include "cfl_parser.h"

int main(int argc, char* argv[])
{
    cfl_node node;

    cfl_parse_file(&node, "lol.cfl");

    cfl_print_node(&node);

    cfl_delete_node(&node);

    return 0;
}
