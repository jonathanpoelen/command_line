/* The MIT License (MIT)
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/**
* \author    Jonathan Poelen jonathan.poelen+falcon@gmail.com
* \version   0.1
* \brief
*/

#pragma once

#include <utility>
#include <iostream>

#include <boost/hana.hpp>

namespace jln { namespace cl {

namespace hana = boost::hana;

template<class Param, class T>
struct arg
{
  using tag = Param;
  using type = T;
  T value;
};

template<class T>
using tag_ = typename T::tag;

template<class T>
using tag_t = hana::basic_type<tag_<T>>;

template<class T>
using dtag = tag_<std::decay_t<T>>;

template<class T>
using dtag_t = hana::basic_type<dtag<T>>;

template<class T>
using type_ = typename T::type;

struct unpack_type_t
{
  template<class T>
  constexpr type_<T> operator()(T) const
  {
    return {};
  }
};
constexpr unpack_type_t unpack_type {};

template<class T>
using dtype = type_<std::decay_t<T>>;

constexpr class next_arg_t {} next_arg {};
constexpr class random_t {} random {};
constexpr class ended_t {} ended {};

constexpr class default_value_parser_t {} default_value_parser {};

constexpr class unspecified_t {} unspecified {};

template<class... Ts>
struct tuple_t : Ts...
{
  constexpr tuple_t(Ts... xs)
    : Ts{std::forward<Ts>(xs)}...
  {}

  constexpr tuple_t() = default;

private:
  template<class Tag, class T>
  static constexpr arg<Tag, T> const& at_impl(arg<Tag, T> const& x)
  {
    return x;
  }

public:
  template<class Tag>
  constexpr auto operator[](Tag) const
  -> decltype(at_impl<Tag>(*this).value)
  {
    return at_impl<Tag>(*this).value;
  }

  template<class Tag>
  constexpr auto get(Tag) const
  -> decltype(at_impl<Tag>(*this))
  {
    return at_impl<Tag>(*this);
  }

  template<class F>
  void each(F&& f)
  {
    (void)std::initializer_list<int>{((f(static_cast<Ts&>(*this))), 1)...};
  }

  template<class F>
  auto apply(F&& f) const
  -> decltype(f(std::declval<Ts const&>()...))
  {
    return f(static_cast<Ts const&>(*this)...);
  }
};


template<>
struct tuple_t<>
{
  constexpr tuple_t() = default;

  template<class F>
  void each(F&&) const
  {}

  template<class F>
  auto apply(F&& f) const
  {
    return f();
  }
};

template<class... Ts>
constexpr tuple_t<Ts...> tuple(Ts... xs)
{
  return tuple_t<Ts...>{xs...};
}

namespace param_types
{
  struct string
  {
    template<char... c>
    hana::string<c...>
    constexpr operator()(hana::string<c...>) const noexcept
    { return {}; }

    template<char c>
    hana::string<c>
    constexpr operator()(std::integral_constant<char, c>) const noexcept
    { return {}; }
  };

  struct string_list
  {
    template<class T>
    auto constexpr operator()(T x) const noexcept
    {
      return tuple(impl(x));
    }

    template<class... Ts>
    constexpr auto operator()(tuple_t<Ts...> t) const noexcept
    {
      return t.apply([](auto... xs){
        return tuple(impl(xs)...);
      });
    }

  private:
    template<char... c>
    hana::string<c...>
    static constexpr impl(hana::string<c...>) noexcept
    { return {}; }

    template<char c>
    hana::string<c>
    static constexpr impl(std::integral_constant<char, c>) noexcept
    { return {}; }
  };

  struct char_list
  {
    template<char... c>
    tuple_t<hana::string<c>...>
    constexpr operator()(hana::string<c...>) const noexcept
    { return {}; }

    template<char c>
    tuple_t<hana::string<c>>
    constexpr operator()(std::integral_constant<char, c>) const noexcept
    { return {}; }

    template<class... Ts>
    constexpr auto operator()(tuple_t<Ts...> t) const noexcept
    {
      return t.apply([](auto... xs){
        return tuple(tuple_elem(xs)...);
      });
    }

  private:
    template<char c>
    tuple_t<hana::string<c>>
    static constexpr tuple_elem(hana::string<c>) noexcept
    { return {}; }

    template<char c>
    tuple_t<hana::string<c>>
    static constexpr tuple_elem(std::integral_constant<char, c>) noexcept
    { return {}; }
  };

  struct suffix
  {
    template<class T>
    auto constexpr operator()(T x) const noexcept
    {
      return tuple(impl(x));
    }

    template<class... Ts>
    constexpr auto operator()(tuple_t<Ts...> t) const noexcept
    {
      return t.apply([](auto... xs){
        return tuple(impl(xs)...);
      });
    }

  private:
    template<char... c>
    hana::string<c...>
    static constexpr impl(hana::string<c...>) noexcept
    {
      static_assert(sizeof...(c) <= 1, "too many character");
      return {};
    }

    template<char c>
    hana::string<c>
    static constexpr impl(std::integral_constant<char, c>) noexcept
    { return {}; }

    next_arg_t
    static constexpr impl(next_arg_t) noexcept
    { return {}; }
  };

  struct positional
  {
    random_t
    constexpr operator()(random_t) const noexcept
    { return {}; }
  };

  struct action
  {
    template<class T>
    constexpr T operator()(T x) const noexcept
    { return x; }
  };

  struct value_parser
  {
    template<class T>
    constexpr T operator()(T x) const noexcept
    { return x; }
  };
}

template<class T>
using disable_unspecified = std::enable_if_t<!std::is_same<std::decay_t<T>, unspecified_t>::value, bool>;

template<class Param, class ValueMaker>
struct param_maker
{
  using tag = Param;

  template<class T, disable_unspecified<T> = 1>
  constexpr auto operator()(T&& x) const noexcept
  -> arg<tag, decltype(ValueMaker{}(std::forward<T>(x)))>
  { return {ValueMaker{}(std::forward<T>(x))}; }

  constexpr auto operator()(unspecified_t) const noexcept
  -> arg<tag, unspecified_t>
  { return {}; }
};

#define CL_MAKE_PARAM(name, value_maker)             \
  struct name##_ : param_maker<name##_, value_maker> \
  {                                                  \
    template<class T>                                \
    constexpr auto operator=(T&& x) const noexcept   \
    {                                                \
      return operator()(std::forward<T>(x));         \
    }                                                \
  };                                                 \
  constexpr name##_ name {}

CL_MAKE_PARAM(desc, param_types::string);

CL_MAKE_PARAM(long_prefix, param_types::string_list);
CL_MAKE_PARAM(long_optional_suffix, param_types::suffix);
CL_MAKE_PARAM(long_required_suffix, param_types::suffix);

CL_MAKE_PARAM(short_prefix, param_types::char_list);
CL_MAKE_PARAM(short_optional_suffix, param_types::suffix);
CL_MAKE_PARAM(short_required_suffix, param_types::suffix);

CL_MAKE_PARAM(positional_opt, param_types::positional);
CL_MAKE_PARAM(sentinel_value, param_types::string);

CL_MAKE_PARAM(short_name, param_types::string);
CL_MAKE_PARAM(long_name, param_types::string);
CL_MAKE_PARAM(action, param_types::action);
CL_MAKE_PARAM(value_parser, param_types::value_parser);

#undef CL_MAKE_PARAM

template<char... c>
struct arg<short_name_, hana::string<c...>>
{
  static_assert(sizeof...(c) <= 1, "too many character");
  using tag = short_name_;
  using type = hana::string<c...>;
  type value;
};

namespace literals
{
  template<class CharT, CharT... c>
  constexpr hana::string<c...> operator "" _s()
  {
    return {};
  }

  template<class CharT, CharT... c>
  constexpr arg<short_name_, hana::string<c...>> operator "" _c()
  {
    return {};
  }
}

namespace detail
{
  template<class TupleTag, class... Ts>
  constexpr std::size_t check_tag(Ts const&...) noexcept
  {
    int a[]{1, ((void)static_cast<tag_<Ts>>(TupleTag{}), 1)...};
    return sizeof(a)+1;
  }

  template<class T>
  struct ref
  {
    T & x;

    constexpr T get() const
    { return std::forward<T>(x); }
  };

  template<class Tag, class Default, class TupleRef>
  constexpr auto extract_arg(arg<Tag, Default>, TupleRef t, int)
  -> std::decay_t<decltype(t[Tag{}].get())>
  {
    return t[Tag{}].get();
  }

  template<class Tag, class Default, class TupleRef>
  constexpr arg<Tag, Default> extract_arg(arg<Tag, Default> x, TupleRef, char)
  {
    return x;
  }

  template<template<class...> class Class, class Tuple, class... Param>
  constexpr auto make_arguments_wrapper(Tuple t, tuple_t<Param...>)
  {
    return Class<decltype(extract_arg(Param{}, t, 1))...>{
      {extract_arg(Param{}, t, 1)...}
    };
  }
}

template<class T>
void print_signature(T const&);

template<class... Ts>
struct Option
{
  tuple_t<Ts...> t;
};

template<char... c>
constexpr char const * c_str(hana::string<c...> s) noexcept
{
  return hana::to<char const*>(s);
}

template<class Param>
struct at_
{
  template<class Tuple>
  constexpr auto operator()(Tuple const& t) const
  {
    return t[Param{}];
  }
};

// TODO by
template<class Param>
constexpr at_<Param> at(Param)
{ return {}; }

struct to_hana_tuple_
{
  template<class Tuple>
  constexpr auto operator()(Tuple const& t) const
  {
    return t.apply(hana::make_tuple);
  }
};

constexpr to_hana_tuple_ to_hana_tuple {};

// hana::concat
template<char... c1, char... c2>
hana::string<c1..., c2...> strcat(hana::string<c1...>, hana::string<c2...>)
{
  return {};
}

template<class Prefix, class Cat, class I>
auto concat_prefixes_and_name(hana::string<>, Prefix, Cat, I)
{
  return hana::make_tuple();
}

template<class Name, class Prefixes, class Cat, class I>
auto concat_prefixes_and_name(Name name, Prefixes prefixes, Cat cat, I i)
{
  return prefixes.apply([=](auto ...s1) {
    return hana::make_tuple(hana::make_tuple(
      i, cat, strcat(s1, name)
    )...);
  });
}

constexpr class short_cat_t {} short_cat {};
constexpr class long_cat_t {} long_cat {};

constexpr hana::true_ operator == (short_cat_t, short_cat_t) { return {}; }
constexpr hana::true_ operator == (long_cat_t, long_cat_t) { return {}; }
constexpr hana::false_ operator == (short_cat_t, long_cat_t) { return {}; }
constexpr hana::false_ operator == (long_cat_t, short_cat_t) { return {}; }

using hana::literals::operator"" _c;

constexpr auto iidx = 0_c;
constexpr auto icat = 1_c;
constexpr auto istr = 2_c;
constexpr auto isub = 3_c;

struct group_tree_cmp_t
{
  template<class Tuple>
  auto operator()(Tuple const& t) const
  {
    return hana::at(t[istr], 0_c);
  }
};

template<class... Option>
struct Options
{
  hana::tuple<Option...> opts;

  template<class NameOpts>
  struct ArgsParser;

  void parse(int ac, char /*const*/** av) const
  {
    print_signature(opts);

    auto name_options = hana::flatten(hana::zip_with(
      [](auto& opt, auto i) {
        auto short_names = concat_prefixes_and_name(
          at(short_name)(opt), at(short_prefix)(opt), short_cat, i);
        auto long_names = concat_prefixes_and_name(
          at(long_name)(opt), at(long_prefix)(opt), long_cat, i);
        print_signature(short_names);
        print_signature(long_names);
        return hana::concat(short_names, long_names);
      },
      opts,
      hana::to_tuple(hana::make_range(hana::int_c<0>, hana::size(opts)))
    ));

    auto sorted_name_options = hana::sort(name_options, hana::ordering(
      hana::reverse_partial(hana::at, istr)));

    print_signature(sorted_name_options);

    auto get_char_from_str_at = [&](auto ichar){
      return [&](auto i){
        return hana::at(sorted_name_options[i][istr], ichar);
      };
    };

    for (int i = 1; i <= ac; ++i, ++av)
    {
      if (!**av)
      {
        std::cout << "empty positional\n";
        continue;
      }

      auto s = *av;

      auto selected_option = [&](auto i, auto depth){
        auto option = sorted_name_options[i];
        auto name = option[istr];
        std::cout << " idx = " << i << " = " << hana::to<char const*>(name) << "\n";
        auto remaining = hana::int_c<hana::size(name)> - depth;
        auto r = hana::fold(
          hana::make_range(0_c, remaining),
          hana::true_c,
          [&](auto state, auto istr){
            return state && hana::at(name, depth+istr) == *s
              ? (++s, true) : false;
          }
        );
        if (r) {
          std::cout << "ok\n";
          at(action)(this->opts[option[iidx]])();
        }
        else {
          std::cout << "fail\n";
        }
      };

      auto parse_arg = [&](auto rec, auto indexes, auto ichar) -> bool {
        auto get_char_at = get_char_from_str_at(ichar);

        auto group_i_options = hana::group(indexes, hana::comparing(get_char_at));

        return hana::fold(
          group_i_options,
          hana::true_c,
          [&](auto state, auto & gindexes){
            if (!state) {
              return false;
            }
            auto c = get_char_at(gindexes[0_c]);
            return hana::eval_if(
              c == hana::char_c<'\0'>,
              [&](auto _){
                selected_option(gindexes[_(0_c)], ichar);
                return false;
              },
              [&](auto _){
                if (c == *s) {
                  std::cout << *s << "\n";
                  ++s;
                  return hana::eval_if(
                    hana::size(gindexes) == hana::size_c<1>,
                    [&](auto _){
                      selected_option(gindexes[_(0_c)], ichar + 1_c);
                      return false;
                    },
                    [&](auto _){ return rec(gindexes, ichar + _(1_c)); }
                  );
                }
                return true;
              }
            );
          }
        );
      };

      hana::fix(parse_arg)(
        hana::to_tuple(hana::make_range(0_c, hana::size(sorted_name_options))),
        0_c);

      // ArgsParser<decltype(sorted_name_options)>{sorted_name_options, s}
        // .parse_arg(tree, 0_c);
    }
  }

  template<class NameOpts>
  struct ArgsParser
  {
    NameOpts name_opts;
    const char* s;

    template<class Tree, class Depth>
    void parse_arg(Tree t, Depth depth)
    {
      parse_next_arg(t, 0_c, hana::llong_c<hana::size(t)>, depth);
    }

    template<class Tree, class I, class IEnd, class Depth>
    void parse_next_arg(Tree t, I i, IEnd iend, Depth depth)
    {
      if (t[i][0_c] == *s) {
        std::cout << *s << "\n";
        ++s;
        parse_sub_tree(t[i][1_c], depth);
      }
      else {
        parse_next_arg(t, i+1_c, iend, depth);
      }
    }

    template<class Tree, class I, class Depth>
    void parse_next_arg(Tree, I, I, Depth)
    {}

    template<class Depth, class T, T N>
    void parse_sub_tree(hana::integral_constant<T, N> i, Depth)
    {
      std::cout << " idx = " << i << " = " << hana::to<char const*>(name_opts[i][istr]) << "\n";
    }

    template<class Depth, class SubTree>
    void parse_sub_tree(SubTree sub, Depth depth)
    {
      parse_arg(sub, depth+1_c);
    }
  };
};

template<class... Ts>
Options<Ts...> options(Ts... xs)
{
  return Options<Ts...>{{xs...}};
}

template<class... Ts>
struct Parser
{
  tuple_t<Ts...> t;

  template<class Arg>
  static Arg get_default(Arg arg)
  {
    return arg;
  };

  template<class Tag>
  auto get_default(arg<Tag, unspecified_t>) const
  {
    return t.get(Tag{});
  };

  template<class ...Opt>
  auto command_line(Opt ...opt)
  {
    #ifdef IN_IDE_PARSER
    return options();
    #else
    return options(opt.t.apply([this](auto&& ...xs){
      return tuple(this->get_default(xs)...);
    })...);
    #endif
  }
};

template<class... Ts>
constexpr auto check_unknown_argument(hana::tuple<Ts...>)
{
  static_assert(!sizeof...(Ts), "to many arguments, `Ts...` must be empty");
}

template<class... T>
class check_unique_param : T...
{};

template<class... Param, class... X>
constexpr void check_params(tuple_t<Param...>, X const&...)
{
  check_unique_param<tag_<X>...>();
  check_unknown_argument(
    hana::transform(
      hana::remove_if(
        hana::make_tuple(dtag_t<X>{}...),
        hana::partial(hana::contains, hana::make_tuple(tag_t<Param>{}...))),
      unpack_type));
}

struct NoConvert
{
  template<class X>
  X&& operator()(X&& x) const
  { return std::forward<X>(x); }
};

template<class Param>
struct StringTo
{
  template<class X>
  X&& operator()(X&& x) const
  { return std::forward<X>(x); }

  template<char... c>
  auto operator()(hana::string<c...> s) const
  { return Param{} = s; }
};

template<
  template<class...> class Class,
  class DefaultParams,
  class Convert = NoConvert>
struct Creator
{
  template<class... X>
  constexpr auto operator()(X&&... x) const
  {
    return impl(Convert()(x)...);
  }

private:
  template<class... X>
  static constexpr auto impl(X&&... x)
  {
    check_params(DefaultParams{}, x...);
    return detail::make_arguments_wrapper<Class>(
      tuple_t<arg<dtag<decltype(x)>, detail::ref<decltype(x)>>...>({x}...),
      DefaultParams{});
  }
};


//#define FUN(Type, name, params)                    \
//  struct Type##Params : decltype(tuple params) {}; \
//  /*constexpr */auto name = Creator<Parser, Type##Params>()

using literals::operator""_s;

struct ParserParams : decltype(tuple(
  desc = ""_s,
  sentinel_value = "--"_s,
  short_prefix = "-"_s,
  short_optional_suffix = tuple(""_s, next_arg),
  short_required_suffix = tuple(""_s, next_arg),
  long_prefix = "--"_s,
  long_optional_suffix = tuple("="_s),
  long_required_suffix = tuple("="_s, next_arg),
  positional_opt = random
)) {};

constexpr auto make_parser = Creator<Parser, ParserParams, StringTo<desc_>>();


struct OptionParams : decltype(tuple(
  short_name = ""_s,
  long_name = ""_s,
  short_prefix = unspecified,
  short_optional_suffix = unspecified,
  short_required_suffix = unspecified,
  long_prefix = unspecified,
  long_optional_suffix = unspecified,
  long_required_suffix = unspecified,
  desc = ""_s,
  action = unspecified,
  value_parser = default_value_parser
)) {};

constexpr auto option = Creator<Option, OptionParams, StringTo<long_name_>>();

// help()
// action()
// parser()
// prefix()
// suffix()
// short_prefix()
// short_suffix()
// long_prefix()
// long_suffix()
// multi() // prefix, suffix, name, ...

} }



template<class T>
void jln::cl::print_signature(T const&)
{
  std::cout << __PRETTY_FUNCTION__ << "\n";
}
