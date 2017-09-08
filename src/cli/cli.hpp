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

template<class ShortName>
struct get_by_short_name_t
{
  template<class... T>
  constexpr auto
  operator()(detail::ref<tuple_t<ShortName, T...> const&> r) const
  -> decltype(r.x)
  {
    return r.x;
  }
};

template<class ShortName>
constexpr auto get_by_short_name
  = hana::sfinae(get_by_short_name_t<ShortName>{});

template<class LongName>
struct get_by_long_name_t
{
  template<class ShortName, class... T>
  constexpr auto
  operator()(detail::ref<tuple_t<ShortName, LongName, T...> const&> r) const
  -> decltype(r.x)
  {
    return r.x;
  }
};

template<class LongName>
constexpr auto get_by_long_name
  = hana::sfinae(get_by_long_name_t<LongName>{});

template<class Opt, class NameParam, class PrefixParam>
auto enlarge_name_options(
  Opt& opt, NameParam name_param, PrefixParam prefix_param)
{
  auto name = at(name_param)(opt);
  return hana::eval_if(
    hana::is_empty(name),
    hana::make_tuple,
    [&](){
      return hana::transform(
        to_hana_tuple(at(prefix_param)(opt)),
        // concat
        [=](auto s1) {
          return hana::unpack(s1, [=](auto ...c1){
            return hana::unpack(name, [=](auto ...c2){
              return hana::make_string(c1..., c2...);
            });
          });
        }
      );
    }
  );
};

constexpr class short_cat_t {} short_cat {};
constexpr class long_cat_t {} long_cat {};

constexpr hana::true_ operator == (short_cat_t, short_cat_t) { return {}; }
constexpr hana::true_ operator == (long_cat_t, long_cat_t) { return {}; }
constexpr hana::false_ operator == (short_cat_t, long_cat_t) { return {}; }
constexpr hana::false_ operator == (long_cat_t, short_cat_t) { return {}; }

using hana::literals::operator"" _c;

constexpr auto iidx = 0_c;
constexpr auto istr = 1_c;
constexpr auto icat = 2_c;
constexpr auto isub = 3_c;

template<class... Option>
struct Options
{
  hana::tuple<Option...> opts;

  void parse(int ac, char /*const*/** av) const
  {
    print_signature(opts);

    auto name_options = hana::flatten(hana::zip_with(
      [](auto& opt, auto i) {
        auto short_names = enlarge_name_options(opt, short_name, short_prefix);
        auto long_names = enlarge_name_options(opt, long_name, long_prefix);
        print_signature(short_names);
        print_signature(long_names);
        return hana::flatten(hana::make_tuple(
          hana::transform(short_names, [i](auto s){
            return hana::make_tuple(i, s, short_cat);
          }),
          hana::transform(long_names, [i](auto s){
            return hana::make_tuple(i, s, long_cat);
          })
        ));
      },
      opts,
      hana::to_tuple(hana::make_range(hana::int_c<0>, hana::size(opts)))
    ));

    auto sorted_name_options = hana::sort(name_options, hana::ordering(
      hana::reverse_partial(hana::at, istr)));

    print_signature(sorted_name_options);

    auto make_tree = [](auto rec, auto name_options){
      auto group_name_options = hana::group(
        name_options,
        hana::comparing([](auto t){
          return hana::at(t[istr], 0_c);
        })
      );

      return hana::transform(
        group_name_options,
        [rec](auto tt){
          return hana::eval_if(
            hana::size(tt) != hana::size_c<1>,
            [tt, rec](auto _){
              auto subtree = hana::transform(tt, [](auto t){
                return hana::make_tuple(t[iidx], hana::drop_front(t[istr]), t[icat]);
              });
              return hana::make_tuple(
                hana::at(tt[0_c][istr], 0_c),
                _(rec)(subtree)
              );
            },
            hana::always(tt)
          );
        }
      );
    };

    auto tree = hana::fix(make_tree)(sorted_name_options);

    print_signature(tree);

    // auto short_names = hana::transform(opts, at(short_name));
    // auto sorted_short_names = hana::remove_if(
    //   hana::sort(short_names), hana::is_empty);
    // print_signature(sorted_short_names);
    //
    // auto long_names = hana::transform(opts, at(long_name));
    // auto sorted_long_names = hana::remove_if(
    //   hana::sort(long_names), hana::is_empty);
    // print_signature(sorted_long_names);
    //
    // auto short_prefixes = hana::chain(
    //   hana::transform(opts, at(short_prefix)), to_hana_tuple);
    // auto sorted_short_prefixes = hana::unique(hana::sort(short_prefixes));
    // auto long_prefixes = hana::chain(
    //   hana::transform(opts, at(long_prefix)), to_hana_tuple);
    // auto sorted_long_prefixes = hana::unique(hana::sort(long_prefixes));
    // print_signature(sorted_short_prefixes);
    // print_signature(sorted_long_prefixes);
    // auto prefixes = hana::concat(sorted_short_prefixes, sorted_long_prefixes);
    // auto sorted_prefixes = hana::unique(hana::sort(prefixes));
    // print_signature(sorted_prefixes);

    // constexpr auto sorted_prefixes_ = hana::value(sorted_prefixes);
    // constexpr char const* c_prefixes = hana::unpack(
    //   sorted_prefixes_, hana::to<char const*>);
    //
    // for (auto s : c_prefixes) {
    //   std::cout << s << "\n";
    // }

    // using varg = jln::cl::arg<jln::cl::short_name_, boost::hana::string<'v'> >;
    // auto const my_tuple = hana::unpack(opts, [](auto&... xs) {
    //   return tuple(detail::ref<decltype(xs)>{xs}...);
    // });
    //
    // auto e = get_by_short_name<varg>(my_tuple);
    // hana::eval_if(
    //   not hana::is_nothing(e),
    //   [&](){ print_signature(e); },
    //   []{}
    // );

    // for (int i = 1; i <= ac; ++i, ++av)
    // {
    //   if (!**av)
    //   {
    //     std::cout << "empty positional\n";
    //     continue;
    //   }
    //
    //   if (get(long_prefix).apply([av](auto prefix_){
    //     auto prefix = hana::to<char const*>(prefix_);
    //     auto s = *av;
    //     if (prefix[0])
    //     {
    //       auto iprefix = 0u;
    //       for (; prefix[iprefix] == *s; ++s)
    //       {
    //         ++iprefix;
    //         if (!prefix[iprefix])
    //         {
    //           std::cout << "long: " << ++s << "\n";
    //           return true;
    //         }
    //       }
    //     }
    //     return false;
    //    })) {
    //           continue;
    //     }
    //
    //   auto prefix = hana::to<char const*>(get(short_prefix));
    //
    //   if (prefix[0])
    //   {
    //     auto s = *av;
    //     auto iprefix = 0u;
    //     for (; prefix[iprefix]; ++s)
    //     {
    //       if (prefix[iprefix] == *s)
    //       {
    //         std::cout << "short: " << ++s << "\n";
    //         break;
    //       }
    //       ++iprefix;
    //     }
    //     if (prefix[iprefix])
    //     {
    //       continue;
    //     }
    //   }
    //
    //   std::cout << "positional: " << *av << "\n";
    // }
  }
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
    return options(opt.t.apply([this](auto&& ...xs){
      return tuple(this->get_default(xs)...);
    })...);
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
