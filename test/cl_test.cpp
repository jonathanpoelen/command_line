#include "cli/cli.hpp"

int main(int ac, char** av)
{
  namespace cl = jln::cl;
  using namespace cl::literals;
  auto parser = cl::make_parser(cl::desc = "plop"_s);
  auto cmd = parser.command_line(
    cl::option("v"_c, "version"_s, cl::action = []{}),
    cl::option("a"_c, "abc"_s, cl::short_prefix = "+"_s, cl::action = []{}),
    cl::option("b"_c,          cl::short_prefix = "-"_s, cl::action = []{}),
    cl::option("c"_c, "cde"_s, cl::long_prefix = cl::tuple("/"_s, "++"_s), cl::action = []{}),
    cl::option("o"_c, "output"_s, cl::action = []{})
  );

  auto const tuple = cl::hana::make_tuple(
    cl::tuple(
      boost::hana::string<'v'>{},
      boost::hana::string<'a'>{}),
    cl::tuple(
      boost::hana::string<'b'>{},
      boost::hana::string<'s'>{})
  );

  // cl::get_by_short_name<
  //   boost::hana::string<'v'>
  // >(cl::hana::unpack(tuple, [](auto&... xs) {
  //   return cl::tuple(cl::detail::ref<decltype(xs)>{xs}...);
  // }), 1)([](auto x){
  //   cl::print_signature(x);
  // });

  cmd.parse(ac, av);
}
