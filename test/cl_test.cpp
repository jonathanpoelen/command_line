#include "cli/cli.hpp"

int main(int ac, char** av)
{
// #define LA [&]{ i += __LINE__; }
#define LA []{ std::cout << "line: " << __LINE__; }
  namespace cl = jln::cl;
  using namespace cl::literals;
  auto parser = cl::make_parser(cl::desc = "plop"_s);
  int i = 0;
  auto cmd = parser.command_line(
    cl::option("v"_c, "version"_s, cl::action = LA),
    cl::option("a"_c, "abc"_s, cl::short_prefix = "+"_s, cl::action = LA),
    cl::option("b"_c,          cl::short_prefix = "-"_s, cl::action = LA),
    cl::option("c"_c, "cde"_s, cl::long_prefix = cl::tuple("/"_s, "++"_s), cl::action = LA),
    cl::option("o"_c, "output"_s, cl::action = LA),
    cl::option("O"_c, "output2"_s, cl::action = LA)
  );

  cmd.parse(ac, av);
  return i;
}
