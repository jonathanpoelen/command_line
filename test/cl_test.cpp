#include "cli/cli.hpp"

int main(int ac, char** av)
{
  namespace cl = jln::cl;
  using namespace cl::literals;
  auto parser = cl::make_parser(cl::desc = "plop"_s);
  parser.parse(ac, av);
}
