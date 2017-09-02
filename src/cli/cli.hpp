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

#include <type_traits>
#include <utility>
#include <iostream>

namespace jln { namespace cl {

template<char... c>
struct lstring
{
  constexpr char operator[](std::size_t i) const noexcept
  {
    constexpr char s[]{c...};
    return s[i];
  }
};

template<class Tag, class T>
struct tagged
{
  using tag = Tag;
  using type = T;
  T value;
};

template<class T>
using tag_ = typename T::tag;

template<class T>
using type_ = typename T::type;

template<class T>
using dtag = tag_<std::decay_t<T>>;

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
  static constexpr tagged<Tag, T> const& at_impl(tagged<Tag, T> const& x)
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
  auto apply(F&& f)
  -> decltype(f(std::declval<Ts&>()...))
  {
    return f(static_cast<Ts&>(*this)...);
  }
};


template<>
struct tuple_t<>
{
  constexpr tuple_t() = default;

  template<class F>
  void each(F&&)
  {}

  template<class F>
  auto apply(F&& f)
  {
    return f();
  }
};

template<class... Ts>
tuple_t<Ts...> tuple(Ts... xs)
{
  return tuple_t<Ts...>{xs...};
}

namespace param_types
{
  struct string
  {
    template<char... c>
    lstring<c...>
    constexpr operator()(lstring<c...>) const noexcept
    { return {}; }

    template<char c>
    lstring<c>
    constexpr operator()(std::integral_constant<char, c>) const noexcept
    { return {}; }
  };

  struct string_list_base
  {
    template<char... c>
    lstring<c...>
    constexpr operator()(lstring<c...>) const noexcept
    { return {}; }

    template<char c>
    lstring<c>
    constexpr operator()(std::integral_constant<char, c>) const noexcept
    { return {}; }
  };

  struct string_list
  {
    template<class T>
    auto constexpr operator()(T x) const noexcept
    {
      return tuple(string_list_base{}(x));
    }

    template<class... Ts>
    constexpr auto operator()(tuple_t<Ts...> t) const noexcept
    {
      return t.apply([](auto... xs){
        return tuple(string_list{}(xs)...);
       });
    }
  };

  struct suffix_base
  {
    template<char... c>
    lstring<c...>
    constexpr operator()(lstring<c...>) const noexcept
    {
      static_assert(sizeof...(c) <= 1, "too many character");
      return {};
    }

    template<char c>
    lstring<c>
    constexpr operator()(std::integral_constant<char, c>) const noexcept
    { return {}; }

    next_arg_t
    constexpr operator()(next_arg_t) const noexcept
    { return {}; }
  };

  struct suffix
  {
    template<class T>
    auto constexpr operator()(T x) const noexcept
    {
      return tuple(suffix_base{}(x));
    }

    template<class... Ts>
    constexpr auto operator()(tuple_t<Ts...> t) const noexcept
    {
      return t.apply([](auto... xs){
        return tuple(suffix_base{}(xs)...);
      });
    }
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

template<class Tag, class ValueMaker>
struct param
{
  using tag = Tag;

  template<class T, disable_unspecified<T> = 1>
  constexpr auto operator()(T&& x) const noexcept
  -> tagged<tag, decltype(ValueMaker{}(std::forward<T>(x)))>
  { return {ValueMaker{}(std::forward<T>(x))}; }

  template<class T, disable_unspecified<T> = 1>
  constexpr auto operator=(T&& x) const noexcept
  -> tagged<tag, decltype(ValueMaker{}(std::forward<T>(x)))>
  { return {ValueMaker{}(std::forward<T>(x))}; }

  constexpr auto operator()(unspecified_t) const noexcept
  -> tagged<tag, unspecified_t>
  { return {}; }

  constexpr auto operator=(unspecified_t) const noexcept
  -> tagged<tag, unspecified_t>
  { return {}; }
};

#define CL_MAKE_PARAM(tag_name, value_maker) \
  namespace tags { class tag_name{}; } \
  constexpr param<tags::tag_name, value_maker> tag_name

CL_MAKE_PARAM(desc, param_types::string);

CL_MAKE_PARAM(long_prefix, param_types::string_list);
CL_MAKE_PARAM(long_optional_suffix, param_types::suffix);
CL_MAKE_PARAM(long_required_suffix, param_types::suffix);

CL_MAKE_PARAM(short_prefix, param_types::string);
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
struct tagged<tags::short_name, lstring<c...>>
{
  static_assert(sizeof...(c) <= 1, "too many character");
  using tag = tags::short_name;
  using type = lstring<c...>;
  type value;
};

namespace literals
{
  template<class CharT, CharT... c>
  constexpr lstring<c...> operator "" _s()
  {
    return {};
  }

  template<class CharT, CharT... c>
  constexpr tagged<tags::short_name, lstring<c...>> operator "" _c()
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
  constexpr auto extract_param(tagged<Tag, Default>, TupleRef t, int)
  -> std::decay_t<decltype(t[Tag{}].get())>
  {
    return t[Tag{}].get();
  }

  template<class Tag, class Default, class TupleRef>
  constexpr tagged<Tag, Default> extract_param(tagged<Tag, Default> x, TupleRef, char)
  {
    return x;
  }
}

template<template<class...> class Class, class... Ts>
auto make_params(Ts... xs)
{
  struct TupleTag : tag_<Ts>... {};
  return [xs...](auto&&... ys){
    static_assert(detail::check_tag<TupleTag>(ys...), "");
    tuple_t<tagged<dtag<decltype(ys)>, detail::ref<decltype(ys)>>...> t({ys}...);
    // return tagged_tuple<tags::cli_parser>(detail::extract_param(xs, t, 1)...);
    return Class<decltype(detail::extract_param(xs, t, 1))...>{
      {detail::extract_param(xs, t, 1)...}
    };
  };
}

// #include <iostream>

template<class T>
void print_signature(T const&);

template<class... Ts>
struct Option
{
  tuple_t<Ts...> t;
};

template<class... Ts>
struct Options
{
  tuple_t<Ts...> t;

  // static_assert(a, "");

  void parse(int ac, char /*const*/** av)
  {
    t.each([](auto const & x){ print_signature(x); });
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

  template<class Param>
  auto get(Param param)
  {
    return t[typename decltype(param)::tag{}];
  };

  template<class Tagged>
  static Tagged get_default(Tagged tagged)
  {
    return tagged;
  };

  template<class Tag>
  auto get_default(tagged<Tag, unspecified_t>) const
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

  void parse(int ac, char /*const*/** av)
  {
    t.each([](auto const & x){ print_signature(x); });

    for (int i = 1; i <= ac; ++i, ++av)
    {
      if (!**av)
      {
        std::cout << "empty positional\n";
        continue;
      }

      if (get(long_prefix).apply([av](auto prefix){
        auto s = *av;
        if (prefix[0])
        {
          auto iprefix = 0u;
          for (; prefix[iprefix] == *s; ++s)
          {
            ++iprefix;
            if (!prefix[iprefix])
            {
              std::cout << "long: " << ++s << "\n";
              return true;
            }
          }
        }
        return false;
      })) {
        continue;
      }

      if (get(short_prefix)[0])
      {
        auto s = *av;
        auto iprefix = 0u;
        for (; get(short_prefix)[iprefix]; ++s)
        {
          if (get(short_prefix)[iprefix] == *s)
          {
            std::cout << "short: " << ++s << "\n";
            break;
          }
          ++iprefix;
        }
        if (get(short_prefix)[iprefix])
        {
          continue;
        }
      }

      std::cout << "positional: " << *av << "\n";
    }
  }
};

// FUN(Parser, make_parser, (desc...))

template<class... Ts>
auto make_parser(Ts&&... xs)
{
  using literals::operator""_s;

  return make_params<Parser>(
    desc = ""_s,
    sentinel_value = "--"_s,
    short_prefix = "-"_s,
    short_optional_suffix = tuple(""_s, next_arg),
    short_required_suffix = tuple(""_s, next_arg),
    long_prefix = "--"_s,
    long_optional_suffix = tuple("="_s, next_arg),
    long_required_suffix = tuple("="_s, next_arg),
    positional_opt = random
  )(std::forward<Ts>(xs)...);
}

template<class... Ts>
auto option(Ts&&... xs)
{
  using literals::operator""_s;

  return make_params<Option>(
    short_prefix = unspecified,
    short_optional_suffix = unspecified,
    short_required_suffix = unspecified,
    long_prefix = unspecified,
    long_optional_suffix = unspecified,
    long_required_suffix = unspecified,
    short_name = ""_s,
    long_name = ""_s,
    desc = ""_s,
    action = unspecified,
    value_parser = default_value_parser
  )(std::forward<Ts>(xs)...);
}

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
