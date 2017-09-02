#include "cli/cli.hpp"

int main(int ac, char** av)
{
  namespace cl = jln::cl;
  using namespace cl::literals;
  auto parser = cl::make_parser(cl::desc = "plop"_s);
  auto cmd = parser.command_line(
    cl::option("v"_c, cl::long_name = "version"_s, cl::action = []{})
  );

  parser.parse(ac, av);
  cmd.parse(ac, av);
}
