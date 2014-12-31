#include "cfl_parser.h"

int main(int argc, char* argv[])
{
    cfl_node node;

    cfl_parse_file(&node, "lol.cfl");

    return 0;
}
