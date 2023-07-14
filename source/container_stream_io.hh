#pragma once

#include <algorithm>  // copy find_if for_each (limits:numeric_limits)
// #include <cstddef>    // size_t
#include <iostream>
#include <set>
#include <string>
#include <tuple>
#include <utility>

#include <iomanip>   // setfill, setw
// #include <iterator>  // begin, end
// #include <type_traits>  // true_type, false_type

#if (__cplusplus < 201103L)
#error "container_stream_io only supports C++11 and above"
#endif  // pre-C++11

namespace std {

#if (__cplusplus < 201402L)

// No feature test macro found for decay_t.
template<class T>
using decay_t = typename decay<T>::type;

// No feature test macro found for enable_if_t.
template< bool B, class T = void >
using enable_if_t = typename enable_if<B,T>::type;

// Use of variable template is_(parse/print)able_as_container_v below ellided by
//   testing for feature test macro __cpp_variable_templates.

// Use of generic lambda in to_stream(not tuple or pair) below ellided by
//   testing for feature test macro __cpp_generic_lambdas.

#endif  // pre-C++14

#if (__cplusplus < 201703L)

#  ifndef __cpp_lib_void_t
#define __cpp_lib_void_t 201411L
template<typename...>
using void_t = void;
#  endif  // undefined __cpp_lib_void_t

#endif  // pre-C++17

}  // namespace std


namespace container_stream_io {

namespace traits {

/**
 * @brief Base case for the testing of STL compatible container types.
 */
template <typename Type, typename = void>
struct is_parseable_as_container : public std::false_type
{};

// !!! unlike the generic version of is_printable_as_container, this generic
//   does not include std::array, which lacks clear()
// std::vector, std::deque
// std::forward_list, std::list
// std::(unordered_)(multi)set, std::(unordered_)(multi)map
// not std::stack, std::queue, std::priority_queue (have value_type, but not clear())
/**
 * @brief Specialization to ensure that Standard Library compatible containers that are
 * move constructible and have members `value_type` and `clear()` are treated as
 * parseable containers.
 */
template <typename Type>
struct is_parseable_as_container<
    Type, std::void_t<
              typename Type::value_type, decltype(std::declval<Type&>().clear())>>
    : public std::integral_constant<
    bool, std::is_move_constructible<typename Type::value_type>::value>
{};

/**
 * @brief Specialization to treat std::pair<...> as a parseable container type.
 */
template <typename FirstType, typename SecondType>
struct is_parseable_as_container<std::pair<FirstType, SecondType>> : public std::true_type
{};

/**
 * @brief Specialization to treat std::tuple<...> as a parseable container type.
 */
template <typename... Args>
struct is_parseable_as_container<std::tuple<Args...>> : public std::true_type
{};

/**
 * @brief Specialization to treat STL arrays as parseable container types.
 */
template <typename ArrayType, std::size_t ArraySize>
struct is_parseable_as_container<std::array<ArrayType, ArraySize>> : public std::true_type
{};

/**
 * @brief Specialization to treat C arrays as parseable container types.
 */
template <typename ArrayType, std::size_t ArraySize>
struct is_parseable_as_container<ArrayType[ArraySize]> : public std::true_type
{};

/**
 * @brief Narrow character array specialization meant to ensure that we print character arrays
 * as strings and not as delimiter containers of individual characters.
 */
template <std::size_t ArraySize>
struct is_parseable_as_container<char[ArraySize]> : public std::false_type
{};

/**
 * @brief Wide character array specialization meant to ensure that we print character arrays
 * as strings and not as delimiter containers of individual characters.
 */
template <std::size_t ArraySize>
struct is_parseable_as_container<wchar_t[ArraySize]> : public std::false_type
{};

/**
 * @brief String specialization meant to ensure that we treat strings as nothing more than
 * strings.
 */
template <typename CharacterType, typename CharacterTraitsType, typename AllocatorType>
struct is_parseable_as_container<
    std::basic_string<CharacterType, CharacterTraitsType, AllocatorType>> : public std::false_type
{};

#ifdef __cpp_variable_templates
/**
 * @brief Helper variable template.
 */
template <typename Type>
constexpr bool is_parseable_as_container_v = is_parseable_as_container<Type>::value;

#endif
/**
 * @brief Base case for the testing of STL compatible container types.
 */
template <typename Type, typename = void>
struct is_printable_as_container : public std::false_type
{};

/**
 * @brief Specialization to ensure that Standard Library compatible containers that have
 * `begin()`, `end()`, and `empty()` member functions are treated as printable containers.
 */
template <typename Type>
struct is_printable_as_container<
    Type, std::void_t<
              typename Type::iterator, decltype(std::declval<Type&>().begin()),
              decltype(std::declval<Type&>().end()), decltype(std::declval<Type&>().empty())>>
    : public std::true_type
{};

/**
 * @brief Specialization to treat std::pair<...> as a printable container type.
 */
template <typename FirstType, typename SecondType>
struct is_printable_as_container<std::pair<FirstType, SecondType>> : public std::true_type
{};

/**
 * @brief Specialization to treat std::tuple<...> as a printable container type.
 */
template <typename... Args>
struct is_printable_as_container<std::tuple<Args...>> : public std::true_type
{};

/**
 * @brief Specialization to treat arrays as printable container types.
 */
template <typename ArrayType, std::size_t ArraySize>
struct is_printable_as_container<ArrayType[ArraySize]> : public std::true_type
{};

/**
 * @brief Narrow character array specialization meant to ensure that we print character arrays
 * as strings and not as delimiter containers of individual characters.
 */
template <std::size_t ArraySize>
struct is_printable_as_container<char[ArraySize]> : public std::false_type
{};

/**
 * @brief Wide character array specialization meant to ensure that we print character arrays
 * as strings and not as delimiter containers of individual characters.
 */
template <std::size_t ArraySize>
struct is_printable_as_container<wchar_t[ArraySize]> : public std::false_type
{};

/**
 * @brief String specialization meant to ensure that we treat strings as nothing more than
 * strings.
 */
template <typename CharacterType, typename CharacterTraitsType, typename AllocatorType>
struct is_printable_as_container<
    std::basic_string<CharacterType, CharacterTraitsType, AllocatorType>> : public std::false_type
{};

#ifdef __cpp_variable_templates
/**
 * @brief Helper variable template.
 */
template <typename Type>
constexpr bool is_printable_as_container_v = is_printable_as_container<Type>::value;

#endif
/**
 * @brief Helper function to determine if a container is empty.
 */
template <typename ContainerType>
bool is_empty(const ContainerType& container) noexcept
{
    return container.empty();
}

/**
 * @brief Helper function to test arrays for emptiness.
 */
template <typename ArrayType, std::size_t ArraySize>
constexpr bool is_empty(const ArrayType (&)[ArraySize]) noexcept
{
    return ArraySize == 0;
}

template <typename Type, typename = void>
struct has_emplace : public std::false_type
{};

template <typename Type>
struct has_emplace<Type, std::void_t<decltype(std::declval<Type&>().emplace())>>
    : public std::true_type
{};

template <typename Type, typename = void>
struct has_emplace_back : public std::false_type
{};

template <typename Type>
struct has_emplace_back<Type, std::void_t<decltype(std::declval<Type&>().emplace_back())>>
    : public std::true_type
{};

template <typename Type, typename = void>
struct has_emplace_after : public std::false_type
{};

template <typename Type>
struct has_emplace_after<Type, std::void_t<decltype(std::declval<Type&>().emplace_after())>>
    : public std::true_type
{};

} // namespace traits

namespace decorator {

/**
 * @brief Struct to neatly wrap up all the additional characters we'll need in order to
 * print out the containers.
 */
template <typename CharacterType>
struct delim_wrapper
{
    const CharacterType* prefix;
    const CharacterType* separator;
    const CharacterType* whitespace;
    const CharacterType* suffix;
};

/**
 * @brief Base definition for the delimiters. This probably won't ever get invoked.
 */
template <typename /*ContainerType*/, typename CharacterType>
struct delimiters
{
    static const delim_wrapper<CharacterType> values;
};

/**
 * @brief Default narrow character specialization for any container type that isn't even
 * more specialized.
 */
template <typename ContainerType>
struct delimiters<ContainerType, char>
{
    static constexpr delim_wrapper<char> values { "[", ",", " ", "]" };
};

/**
 * @brief Default wide character specialization for any container type that isn't even
 * more specialized.
 */
template <typename ContainerType>
struct delimiters<ContainerType, wchar_t>
{
    static constexpr delim_wrapper<wchar_t> values { L"[", L",", L" ", L"]" };
};

/**
 * @brief Narrow character specialization for std::set<...> instances.
 */
template <typename DataType, typename ComparatorType, typename AllocatorType>
struct delimiters<std::set<DataType, ComparatorType, AllocatorType>, char>
{
    static constexpr delim_wrapper<char> values { "{", ",", " ", "}" };
};

/**
 * @brief Wide character specialization for std::set<...> instances.
 */
template <typename DataType, typename ComparatorType, typename AllocatorType>
struct delimiters<std::set<DataType, ComparatorType, AllocatorType>, wchar_t>
{
    static constexpr delim_wrapper<wchar_t> values { L"{", L",", L" ", L"}" };
};

/**
 * @brief Narrow character specialization for std::multiset<...> instances.
 */
template <typename DataType, typename ComparatorType, typename AllocatorType>
struct delimiters<std::multiset<DataType, ComparatorType, AllocatorType>, char>
{
    static constexpr delim_wrapper<char> values { "{", ",", " ", "}" };
};

/**
 * @brief Wide character specialization for std::multiset<...> instances.
 */
template <typename DataType, typename ComparatorType, typename AllocatorType>
struct delimiters<std::multiset<DataType, ComparatorType, AllocatorType>, wchar_t>
{
    static constexpr delim_wrapper<wchar_t> values { L"{", L",", L" ", L"}" };
};

/**
 * @brief Narrow character specialization for std::pair<...> instances.
 */
template <typename FirstType, typename SecondType>
struct delimiters<std::pair<FirstType, SecondType>, char>
{
    static constexpr delim_wrapper<char> values { "(", ",", " ", ")" };
};

/**
 * @brief Wide character specialization for std::pair<...> instances.
 */
template <typename FirstType, typename SecondType>
struct delimiters<std::pair<FirstType, SecondType>, wchar_t>
{
    static constexpr delim_wrapper<wchar_t> values { L"(", L",", L" ", L")" };
};

/**
 * @brief Narrow character specialization for std::tuple<...> instances.
 */
template <typename... DataType>
struct delimiters<std::tuple<DataType...>, char>
{
    static constexpr delim_wrapper<char> values { "<", ",", " ", ">" };
};

/**
 * @brief Wide character specialization for std::tuple<...> instances.
 */
template <typename... DataType>
struct delimiters<std::tuple<DataType...>, wchar_t>
{
    static constexpr delim_wrapper<wchar_t> values { L"<", L",", L" ", L">" };
};

} // namespace decorator

namespace strings {

namespace detail {

// labels for flag values used to set string representation type
enum class repr { literal, quoted };

template<typename CharacterType>
struct escape_seq
{
    CharacterType actual;
    CharacterType symbol;
};

// char and wchar_t share the same set of standard escape sequences
// currently no plans to support unicode
// wrapping in struct to avoid variable template use for C++11 compliance
template<typename CharacterType>
struct ascii_escape
{
    static constexpr std::array<escape_seq<CharacterType>, 8> seqs
    {
        {
            {'\a', 'a'}, {'\b', 'b'}, {'\f', 'f'}, {'\n', 'n'},
            {'\r', 'r'}, {'\t', 't'}, {'\v', 'v'}, {'\0', '0'}
            // \', \", and \\ handled by choosing custom delim/escape chars per task
            // \? only extracted into ? by operator>>(string_repr)
            // trigraphs ignored for now, see:
            //   - https://en.cppreference.com/w/c/language/operator_alternative
        }
    };
};

/**
 * @brief Struct for string literal representations.
 */
template<typename StringType, typename CharacterType>
struct string_repr
{
    static_assert(std::is_pointer<StringType>::value ||
                  std::is_reference<StringType>::value,
                  "String type must be a pointer or reference");

    StringType string;
    CharacterType delim;
    CharacterType escape;

    repr type;

    // using array instead of map due to small, fixed set of keys, and
    //   need to lookup by both actual and symbol
    static constexpr decltype(ascii_escape<CharacterType>::seqs) escape_seqs {
        ascii_escape<CharacterType>::seqs};

    string_repr(void) = delete;
    string_repr(StringType str, CharacterType dlm, CharacterType esc, repr t)
	: string(str), delim{dlm}, escape{esc}, type{t}
    {
        // relies on implicit conversion of char to wchar_t in iswprint()
        if (!std::iswprint(dlm) || !std::iswprint(esc))
            throw(std::invalid_argument(
                      "delim and escape must be printable characters"));
    }

    string_repr& operator=(string_repr&) = delete;
};

// linker needs declaration of static constexpr members outside class for
//   standards below C++17, see:
//   - https://en.cppreference.com/w/cpp/language/static
//   - https://stackoverflow.com/a/28846608
#if (__cplusplus < 201703L)
template<typename StringType, typename CharacterType>
constexpr decltype(ascii_escape<CharacterType>::seqs) string_repr<
    StringType, CharacterType>::escape_seqs;
#endif  // pre-C++17

// stream index getter for use with iword/pword to set literalrepr/quotedrepr
static inline int get_manip_i()
{
    static int i {std::ios_base::xalloc()};
    return i;
}

/**
 * @brief Inserter for quoted strings.
 */
template<typename CharacterType, typename TraitsType, typename AllocatorType>
std::basic_ostream<CharacterType, TraitsType>& operator<<(
    std::basic_ostream<CharacterType, TraitsType>& ostream,
    const string_repr<
    const std::basic_string<CharacterType, TraitsType, AllocatorType>&, CharacterType>& str)
{
    std::basic_ostringstream<CharacterType, TraitsType> oss;
    // currently only supporting 2 char types, of those only wchar_t takes a prefix
    if (std::is_same<CharacterType, wchar_t>::value)
        oss << CharacterType('L');
    oss << str.delim;
    for (const auto &c : str.string)
    {
        // string_repr ctor enforces printable delim and escape
        // relies on implicit conversion of char to wchar_t in iswprint()
        if (std::iswprint(c))
        {
            if (c == str.delim || c == str.escape)
                oss << str.escape;
            oss << c;
        }
        else if (str.type == repr::literal)
        {
            oss << str.escape;
            auto esc_it {std::find_if(
                    std::begin(str.escape_seqs), std::end(str.escape_seqs),
                    [&c](const escape_seq<CharacterType>& seq){
                        return seq.actual == c; })};
            if (esc_it != std::end(str.escape_seqs))  // standard escape sequence
            {
                oss << esc_it->symbol;
            }
            else  // custom hex escape sequence
            {
                oss << CharacterType('x') <<
                    std::hex << std::setfill(CharacterType('0')) << std::setw(2) <<
                    (0xff & (unsigned int)c);
            }
        }
    }
    oss << str.delim;

    return ostream << oss.str();
}

/**
 * @brief Extractor for delimited strings.
 *
 * Differs from iomanip + bits/quoted_string.h version in that failure to extract
 * leading and trailing delimiters counts as extraction failure.
 */
template<typename CharacterType, typename TraitsType, typename AllocatorType>
std::basic_istream<CharacterType, TraitsType>& operator>>(
    std::basic_istream<CharacterType, TraitsType>& istream,
    const string_repr<std::basic_string<CharacterType, TraitsType, AllocatorType>&, CharacterType>& str)
{
    CharacterType c;
    if (std::is_same<CharacterType, wchar_t>::value)
    {
        istream >> c;
        if (c != L'L')
            istream.setstate(std::ios_base::failbit);
        if (!istream.good())
            return istream;
    }
    istream >> c;
    if (c != str.delim)
        istream.setstate(std::ios_base::failbit);
    if (!istream.good())
        return istream;
    std::decay_t<decltype(str.string)> temp;
    std::ios_base::fmtflags flags
        = istream.flags(istream.flags() & ~std::ios_base::skipws);
    for (istream >> c; istream.good() && c != str.delim; istream >> c) {
        if (c != str.escape)
        {
            temp += c;
            continue;
        }
        istream >> c;
        if (!istream.good())
            break;
        if (c == str.escape || c == str.delim)
        {
            temp += c;
            continue;
        }
        if (str.type == repr::literal)
        {
            // "\?" -> '?' omitted from ascii_escapes as it is only relevant during extraction
            if (c == CharacterType('?'))
            {
                temp += c;
                continue;
            }
            auto esc_it {std::find_if(
                    std::begin(str.escape_seqs), std::end(str.escape_seqs),
                    [&c](const escape_seq<CharacterType>& seq){ return seq.symbol == c; })};
            if (esc_it != std::end(str.escape_seqs))  // standard escape sequence
            {
                temp += esc_it->actual;
                continue;
            }
            else if (c == CharacterType('x')) // custom hex escape sequence
            {
                int hex_val;
                istream >> std::hex >> std::setw(2) >> hex_val;
                temp += CharacterType(hex_val);
                continue;
            }
        }
        // invalid escape
        istream.setstate(std::ios_base::failbit);
    };
    if (c != str.delim)
        istream.setstate(std::ios_base::failbit);
    istream.setf(flags);
    if (!istream.fail() && !istream.bad())
        str.string = std::move(temp);
    return istream;
}

}  // namespace detail

// default is string literals, with most ASCII escapes plus hex escapes
template<typename CharacterType, typename TraitsType>
std::basic_ios<CharacterType, TraitsType>& literalrepr(
    std::basic_ios<CharacterType, TraitsType>& stream)
{
    stream.iword(detail::get_manip_i()) = detail::repr::literal;
    return stream;
}

// only escape char and delimiter escaped
template<typename CharacterType, typename TraitsType>
std::basic_ios<CharacterType, TraitsType>& quotedrepr(
    std::basic_ios<CharacterType, TraitsType>& stream)
{
    stream.iword(detail::get_manip_i()) = detail::repr::quoted;
    return stream;
}

/**
 * @brief Manipulator for quoted strings.
 * @param string String to quote.
 * @param delim  Character to quote string with.
 * @param escape Escape character to escape itself or quote character.
 */
template<typename CharacterType>
inline auto quoted(
    const CharacterType* string, CharacterType delim = CharacterType('"'),
    CharacterType escape = CharacterType('\\')) ->
    detail::string_repr<const CharacterType*, CharacterType>
{
    return detail::string_repr<
        std::basic_string<CharacterType>, CharacterType>(
            std::basic_string<CharacterType>(string), delim, escape, detail::repr::quoted);
}

template<typename CharacterType, typename TraitsType, typename AllocatorType>
inline auto quoted(
    const std::basic_string<CharacterType, TraitsType, AllocatorType>& string,
    CharacterType delim = CharacterType('"'), CharacterType escape = CharacterType('\\')) ->
    detail::string_repr<
	const std::basic_string<CharacterType, TraitsType, AllocatorType>&, CharacterType>
{
    return detail::string_repr<
	const std::basic_string<CharacterType, TraitsType, AllocatorType>&, CharacterType>(
	    string, delim, escape, detail::repr::quoted);
}

template<typename CharacterType, typename TraitsType, typename AllocatorType>
inline auto quoted(
    std::basic_string<CharacterType, TraitsType, AllocatorType>& string,
       CharacterType delim = CharacterType('"'), CharacterType escape = CharacterType('\\')) ->
    detail::string_repr<
        std::basic_string<CharacterType, TraitsType, AllocatorType>&, CharacterType>
{
    return detail::string_repr<
        std::basic_string<CharacterType, TraitsType, AllocatorType>&, CharacterType>(
            string, delim, escape, detail::repr::quoted);
}

template<typename CharacterType>
inline auto literal(
    const CharacterType* string, CharacterType delim = CharacterType('"'),
    CharacterType escape = CharacterType('\\')) ->
    detail::string_repr<const CharacterType*, CharacterType>
{
    return detail::string_repr<
        std::basic_string<CharacterType>, CharacterType>(
            std::basic_string<CharacterType>(string), delim, escape, detail::repr::literal);
}

template<typename CharacterType, typename TraitsType, typename AllocatorType>
inline auto literal(
    const std::basic_string<CharacterType, TraitsType, AllocatorType>& string,
    CharacterType delim = CharacterType('"'), CharacterType escape = CharacterType('\\')) ->
    detail::string_repr<
	const std::basic_string<CharacterType, TraitsType, AllocatorType>&, CharacterType>
{
    return detail::string_repr<
	const std::basic_string<CharacterType, TraitsType, AllocatorType>&, CharacterType>(
	    string, delim, escape, detail::repr::literal);
}

template<typename CharacterType, typename TraitsType, typename AllocatorType>
inline auto literal(
    std::basic_string<CharacterType, TraitsType, AllocatorType>& string,
       CharacterType delim = CharacterType('"'), CharacterType escape = CharacterType('\\')) ->
    detail::string_repr<
        std::basic_string<CharacterType, TraitsType, AllocatorType>&, CharacterType>
{
    return detail::string_repr<
        std::basic_string<CharacterType, TraitsType, AllocatorType>&, CharacterType>(
            string, delim, escape, detail::repr::literal);
}

}  // namespace strings


namespace input {

template <typename CharacterType>
static void extract_token(
    std::basic_istream<CharacterType>& istream, const CharacterType* token) {
    if (token == nullptr) {
        istream.setstate(std::ios_base::failbit);
        return;
    }
    auto token_s {
#if (__cplusplus >= 201703L)
        std::basic_string_view<CharacterType>{token}
#else
        std::basic_string<CharacterType>{token}
#endif
    };
    istream >> std::ws;
    auto it_1 {token_s.begin()};
    while (!istream.eof() && it_1 != token_s.end() &&
           CharacterType(istream.peek()) == *it_1) {
        istream.get();
        ++it_1;
    }
    if (it_1 != token_s.end()) {
        // only partial delim match, return chars to stream
        for (auto it_2 {token_s.begin()}; it_2 != it_1; ++it_2)
            istream.putback(*it_2);
        istream.setstate(std::ios_base::failbit);
    }
    return;
}

template <typename ContainerType, typename StreamType>
struct default_formatter
{
    static constexpr auto decorators = container_stream_io::decorator::delimiters<
        ContainerType, typename StreamType::char_type>::values;

    static void parse_prefix(StreamType& istream) noexcept
    {
        extract_token(istream, decorators.prefix);
    }

    template <typename ElementType>
    static void parse_element(StreamType& istream, ElementType& element) noexcept
    {
        istream >> std::ws >> element;
    }

    template <typename CharacterType>
    static void parse_char(StreamType& istream, CharacterType& element) noexcept
    {
        std::basic_string<CharacterType> s;
        using repr = strings::detail::repr;
        if ((repr)istream.iword(strings::detail::get_manip_i()) == repr::quoted)
            istream >> std::ws >> strings::quoted(s, CharacterType('\''));
        else
            istream >> std::ws >> strings::literal(s, CharacterType('\''));
        if (s.size() != 1)
        {
            istream.setstate(std::ios_base::failbit);
            return;
        }
        element = s[0];
    }

    static void parse_element(StreamType& istream, char& element) noexcept
    {
        parse_char<char>(istream, element);
    }

    static void parse_element(StreamType& istream, wchar_t& element) noexcept
    {
        parse_char<wchar_t>(istream, element);
    }

    template <typename CharacterType, std::size_t ArraySize>
    static void parse_char_array(StreamType& istream, CharacterType (&element)[ArraySize]) noexcept
    {
        std::basic_string<CharacterType> s;
        using repr = strings::detail::repr;
        if ((repr)istream.iword(strings::detail::get_manip_i()) == repr::quoted)
            istream >> std::ws >> strings::quoted(s);
        else
            istream >> std::ws >> strings::literal(s);
        if (s.size() > ArraySize - 1)
        {
            istream.setstate(std::ios_base::failbit);
            return;
        }
        auto it {std::copy(s.begin(), s.end(), std::begin(element))};
        std::fill(it, std::end(element), CharacterType('\0'));
    }

    template <std::size_t ArraySize>
    static void parse_element(StreamType& istream, char (&element)[ArraySize]) noexcept
    {
        parse_char_array<char, ArraySize>(istream, element);
    }

    template <std::size_t ArraySize>
    static void parse_element(StreamType& istream, wchar_t (&element)[ArraySize]) noexcept
    {
        parse_char_array<char, ArraySize>(istream, element);
    }

    template<typename CharacterType>
    static void parse_element(StreamType& istream,
                              std::basic_string<CharacterType>& element) noexcept
    {
        using repr = strings::detail::repr;
        if ((repr)istream.iword(strings::detail::get_manip_i()) == repr::quoted)
            istream >> std::ws >> strings::quoted(element);
        else
            istream >> std::ws >> strings::literal(element);
    }

#if (__cplusplus >= 201703L)
    template<typename CharacterType>
    static void parse_element(StreamType& istream,
                              std::basic_string_view<CharacterType>& element) noexcept
    {
        std::basic_string<CharacterType> s;
        using repr = strings::detail::repr;
        if ((repr)istream.iword(strings::detail::get_manip_i()) == repr::quoted)
            istream >> std::ws >> strings::quoted(s);
        else
            istream >> std::ws >> strings::literal(s);
        element = std::basic_string_view<CharacterType> {s.c_str()};
    }

#endif
    // std::forward_list
    template <typename ElementType>
    static auto insert_element(
        ContainerType& container, const ElementType& element) noexcept -> std::enable_if_t<
            container_stream_io::traits::has_emplace_after<ContainerType>::value &&
            std::is_move_assignable<ElementType>::value, void>
    {
        container.emplace_after(container.end(), element);
    }

    // std::vector, std::deque, std::list
    template<typename ElementType>
    static auto insert_element(
        ContainerType& container, const ElementType& element) noexcept -> std::enable_if_t<
            container_stream_io::traits::has_emplace_back<ContainerType>::value &&
            std::is_move_assignable<ElementType>::value, void>
    {
        container.emplace_back(element);
    }

    // std::(unordered_)set (redundant keys ignored)
    // std::(unordered_)map (redundant keys will have value of first appearance in serialization)
    // std::(unordered_)multiset, std::(unordered_)multimap
    template <typename ElementType>
    static auto insert_element(
        ContainerType& container, const ElementType& element) noexcept -> std::enable_if_t<
            container_stream_io::traits::has_emplace<ContainerType>::value &&
            !container_stream_io::traits::has_emplace_back<ContainerType>::value &&
            std::is_move_assignable<ElementType>::value, void>
    {
        container.emplace(element);
    }

    static void parse_separator(StreamType& istream) noexcept
    {
        extract_token(istream, decorators.separator);
    }

    static void parse_suffix(StreamType& istream) noexcept
    {
        extract_token(istream, decorators.suffix);
    }
};

// move-assignable (all STL containers (including std::array))
template<typename ContainerType>
static auto safer_assign(ContainerType& source, ContainerType& target) -> std::enable_if_t<
        std::is_move_assignable<ContainerType>::value, void>
{
    target = std::move(source);
}

template<typename ElementType, std::size_t ArraySize>
static void safer_assign(ElementType (&source)[ArraySize], ElementType (&target)[ArraySize])
{
    // TBD: optimize with C memcpy?
    // memcpy(&target[0], &source[0], ArraySize * sizeof ElementType);
    auto t_end {std::end(target)};
    for (auto s_it {std::begin(source)}, t_it {std::begin(target)};
         t_it != t_end; ++s_it, ++t_it)
    {
        safer_assign(*s_it, *t_it);
    }
}

// tried enable_if with SFINAE struct is_array which would be true for std::array
//   and C arrays, but it appeared that the resolution of is_array caused the
//   decay of the C arrays to pointers, so they would then not be recognized as
//   T[N] by the time they were used in from_stream overload resolution
template <typename ContainerType, typename StreamType, typename FormatterType>
static StreamType& array_from_stream(
    StreamType& istream, ContainerType& container,
    const FormatterType& formatter)
{
    formatter.parse_prefix(istream);
    if (!istream.good())
        return istream;

    if (container_stream_io::traits::is_empty(container)) {
        formatter.parse_suffix(istream);
        return istream;
    }

    ContainerType temp_container;
    auto tc_it {std::begin(temp_container)};
    auto tc_end {std::end(temp_container)};

    formatter.parse_element(istream, *tc_it);
    if (!istream.good())
        return istream;
    ++tc_it;

    for (; !istream.eof() && tc_it != tc_end; ++tc_it) {
        formatter.parse_separator(istream);
        if (!istream.good())
            return istream;

        formatter.parse_element(istream, *tc_it);
        if (!istream.good())
            return istream;
    }

    if (tc_it != tc_end) {  // serialization too short
        istream.setstate(std::ios_base::failbit);
        return istream;
    }

    formatter.parse_suffix(istream);  // fails if serialization too long
    if (!istream.bad() && !istream.fail())
        safer_assign(temp_container, container);
    return istream;
}

template <typename ElementType, std::size_t ArraySize,
          typename StreamType, typename FormatterType>
static StreamType& from_stream(
    StreamType& istream, ElementType (&container)[ArraySize],
    const FormatterType& formatter)
{
    return array_from_stream(istream, container, formatter);
}

template <typename ElementType, std::size_t ArraySize,
          typename StreamType, typename FormatterType>
static StreamType& from_stream(
    StreamType& istream, std::array<ElementType, ArraySize>& container,
    const FormatterType& formatter)
{
    return array_from_stream(istream, container, formatter);
}

/**
 * @brief Recursive tuple handler struct meant to unpack and print std::tuple<...> elements.
 */
template <typename TupleType, std::size_t Index, std::size_t Last>
struct tuple_handler
{
    template <typename StreamType, typename FormatterType>
    static void
    parse(StreamType& istream, TupleType& container, const FormatterType& formatter)
    {
        istream >> std::get<Index>(container);
        formatter.parse_separator(istream);
        if (istream.good())
            tuple_handler<TupleType, Index + 1, Last>::parse(istream, container, formatter);
    }
};

/**
 * @brief Specialization of tuple handler to deal with the last element in the std::tuple<...>
 * object.
 */
template <typename TupleType, std::size_t Index>
struct tuple_handler<TupleType, Index, Index>
{
    template <typename StreamType, typename FormatterType>
    static void
    parse(StreamType& istream, TupleType& tuple, const FormatterType& formatter) noexcept
    {
        if (istream.good())
            formatter.parse_element(istream, std::get<Index>(tuple));
    }
};

/**
 * @brief Specialization of tuple handler to deal with empty std::tuple<...> objects.
 */
template <typename TupleType>
struct tuple_handler<TupleType, 0, std::numeric_limits<std::size_t>::max()>
{
    template <typename StreamType, typename FormatterType>
    static void parse(
        StreamType& /*istream*/, const TupleType& /*tuple*/,
        const FormatterType& /*formatter*/) noexcept
    {}
};

/**
 * @brief Overload to deal with std::tuple<...> objects.
 */
template <typename StreamType, typename FormatterType, typename... TupleArgs>
static StreamType& from_stream(
    StreamType& istream, std::tuple<TupleArgs...>& container,
    const FormatterType& formatter)
{
    using ContainerType = std::decay_t<decltype(container)>;

    ContainerType temp;
    formatter.parse_prefix(istream);
    container_stream_io::input::tuple_handler<
        ContainerType, 0, sizeof...(TupleArgs) - 1>::parse(istream, temp, formatter);
    formatter.parse_suffix(istream);
    if (!istream.fail() && !istream.bad())
        container = std::move(temp);
    return istream;
}

template <typename FirstType, typename SecondType,
          typename StreamType, typename FormatterType>
static StreamType& from_stream(
    StreamType& istream, std::pair<FirstType, SecondType>& container,
    const FormatterType& formatter)
{
    formatter.parse_prefix(istream);
    if (!istream.good())
        return istream;

    FirstType first;
    SecondType second;

    formatter.parse_element(istream, first);
    if (!istream.good())
        return istream;

    formatter.parse_separator(istream);
    if (!istream.good())
        return istream;

    formatter.parse_element(istream, second);
    if (!istream.good())
        return istream;

    formatter.parse_suffix(istream);
    if (istream.bad() || istream.fail())
        return istream;

    safer_assign(first, container.first);
    safer_assign(second, container.second);

    return istream;
}

// "generic" overload, but requires value_type, clear(), move assignment of container and elements
template <typename ContainerType, typename StreamType, typename FormatterType>
static StreamType& from_stream(
    StreamType& istream, ContainerType& container,
    const FormatterType& formatter)
{
    formatter.parse_prefix(istream);
    if (!istream.good())
        return istream;

    ContainerType new_container;
    typename ContainerType::value_type temp_elem;

    // parse suffix to check for empty container
    formatter.parse_suffix(istream);
    if (!istream.bad()) {
        if (!istream.fail()) {
            container.clear();
            return istream;
        } else {
            istream.clear();
        }
    }

    formatter.parse_element(istream, temp_elem);
    if (!istream.good())
        return istream;
    formatter.insert_element(new_container, temp_elem);

    while (!istream.eof()) {
        // parse suffix first to detect end of serialization
        formatter.parse_suffix(istream);
        if (!istream.bad()) {
            if (!istream.fail())
                break;
            else
                istream.clear();
        }

        formatter.parse_separator(istream);
        if (!istream.good())
            return istream;

        formatter.parse_element(istream, temp_elem);
        if (!istream.good())
            return istream;
        formatter.insert_element(new_container, temp_elem);
    }

    if (!istream.fail() && !istream.bad())
        container = std::move(new_container);
    return istream;
}

} // namespace input

namespace output {

/**
 * @brief Default container formatter that will be used to print prefix,
 * element, separator, and suffix strings to an output stream.
 */
template <typename ContainerType, typename StreamType>
struct default_formatter
{
    static constexpr auto decorators = container_stream_io::decorator::delimiters<
        ContainerType, typename StreamType::char_type>::values;

    static void print_prefix(StreamType& ostream) noexcept
    {
        ostream << decorators.prefix;
    }

    template <typename ElementType>
    static void print_element(StreamType& ostream, const ElementType& element) noexcept
    {
        ostream << element;
    }

    template<typename CharacterType>
    static void print_char(StreamType& ostream, const CharacterType& element) noexcept
    {
        using repr = strings::detail::repr;
        if ((repr)ostream.iword(strings::detail::get_manip_i()) == repr::quoted)
        {
            ostream << strings::quoted(
                std::basic_string<CharacterType>({element}), CharacterType('\''));
        }
        else
        {
            ostream << strings::literal(
                std::basic_string<CharacterType>({element}), CharacterType('\''));
        }
    }

    static void print_element(StreamType& ostream, const char& element) noexcept
    {
        print_char(ostream, element);
    }

    static void print_element(StreamType& ostream, const wchar_t& element) noexcept
    {
        print_char(ostream, element);
    }

    template <typename StringType>
    static void print_string(StreamType& ostream, const StringType& element) noexcept
    {
        using repr = strings::detail::repr;
        if ((repr)ostream.iword(strings::detail::get_manip_i()) == repr::quoted)
            ostream << strings::quoted(element);
        else
            ostream << strings::literal(element);
    }

    template <std::size_t ArraySize>
    static void print_element(StreamType& ostream, const char (&element)[ArraySize]) noexcept
    {
        print_string(ostream, element);
    }

    template <std::size_t ArraySize>
    static void print_element(StreamType& ostream, const wchar_t (&element)[ArraySize]) noexcept
    {
        print_string(ostream, element);
    }

    template<typename CharacterType>
    static void print_element(StreamType& ostream,
                              const std::basic_string<CharacterType>& element) noexcept
    {
        print_string(ostream, element);
    }

#if (__cplusplus >= 201703L)
    template<typename CharacterType>
    static void print_element(StreamType& ostream,
                              const std::basic_string_view<CharacterType>& element) noexcept
    {
        print_string(ostream, element);
    }

#endif
    static void print_separator(StreamType& ostream) noexcept
    {
        ostream << decorators.separator << decorators.whitespace;
    }

    static void print_suffix(StreamType& ostream) noexcept
    {
        ostream << decorators.suffix;
    }
};

/**
 * @brief Recursive tuple handler struct meant to unpack and print std::tuple<...> elements.
 */
template <typename TupleType, std::size_t Index, std::size_t Last>
struct tuple_handler
{
    template <typename StreamType, typename FormatterType>
    static void
    print(StreamType& ostream, const TupleType& container, const FormatterType& formatter)
    {
        ostream << std::get<Index>(container);
        formatter.print_separator(ostream);
        tuple_handler<TupleType, Index + 1, Last>::print(ostream, container, formatter);
    }
};

/**
 * @brief Specialization of tuple handler to deal with the last element in the std::tuple<...>
 * object.
 */
template <typename TupleType, std::size_t Index>
struct tuple_handler<TupleType, Index, Index>
{
    template <typename StreamType, typename FormatterType>
    static void
    print(StreamType& ostream, const TupleType& tuple, const FormatterType& formatter) noexcept
    {
        formatter.print_element(ostream, std::get<Index>(tuple));
    }
};

/**
 * @brief Specialization of tuple handler to deal with empty std::tuple<...> objects.
 */
template <typename TupleType>
struct tuple_handler<TupleType, 0, std::numeric_limits<std::size_t>::max()>
{
    template <typename StreamType, typename FormatterType>
    static void print(
        StreamType& /*ostream*/, const TupleType& /*tuple*/,
        const FormatterType& /*formatter*/) noexcept
    {}
};

/**
 * @brief Overload to deal with std::tuple<...> objects.
 */
template <typename StreamType, typename FormatterType, typename... TupleArgs>
static StreamType& to_stream(
    StreamType& ostream, const std::tuple<TupleArgs...>& container,
    const FormatterType& formatter)
{
    using ContainerType = std::decay_t<decltype(container)>;

    formatter.print_prefix(ostream);
    container_stream_io::output::tuple_handler<
        ContainerType, 0, sizeof...(TupleArgs) - 1>::print(ostream, container, formatter);
    formatter.print_suffix(ostream);

    return ostream;
}

/**
 * @brief Overload to handle std::pair<...> objects.
 */
template <typename FirstType, typename SecondType, typename StreamType, typename FormatterType>
static StreamType& to_stream(
    StreamType& ostream, const std::pair<FirstType, SecondType>& container,
    const FormatterType& formatter)
{
    formatter.print_prefix(ostream);
    formatter.print_element(ostream, container.first);
    formatter.print_separator(ostream);
    formatter.print_element(ostream, container.second);
    formatter.print_suffix(ostream);

    return ostream;
}

/**
 * @brief Overload to handle containers that support the notion of "emptiness,"
 * and forward-iterability.
 */
template <typename ContainerType, typename StreamType, typename FormatterType>
static StreamType& to_stream(
    StreamType& ostream, const ContainerType& container,
    const FormatterType& formatter)
{
    formatter.print_prefix(ostream);

    if (container_stream_io::traits::is_empty(container)) {
        formatter.print_suffix(ostream);

        return ostream;
    }

    auto begin = std::begin(container);
    formatter.print_element(ostream, *begin);

    std::advance(begin, 1);

    std::for_each(begin, std::end(container),
#ifdef __cpp_generic_lambdas
                  [&ostream, &formatter](const auto& element) {
#else
                  [&ostream, &formatter](const decltype(*begin)& element) {
#endif
        formatter.print_separator(ostream);
        formatter.print_element(ostream, element);
    });

    formatter.print_suffix(ostream);

    return ostream;
}

} // namespace output

} // namespace container_stream_io

/**
 * @brief Overload of the stream output operator for compatible containers.
 */
template <typename ContainerType, typename StreamType>
auto operator<<(StreamType& ostream, const ContainerType& container) -> std::enable_if_t<
#ifdef __cpp_variable_templates
    container_stream_io::traits::is_printable_as_container_v<ContainerType>,
#else
    container_stream_io::traits::is_printable_as_container<ContainerType>::value,
#endif
    StreamType&>
{
    using formatter_type =
        container_stream_io::output::default_formatter<ContainerType, StreamType>;
    container_stream_io::output::to_stream(ostream, container, formatter_type{});

    return ostream;
}

/**
 * @brief Overload of the stream output operator for compatible containers.
 */
template <typename ContainerType, typename StreamType>
auto operator>>(StreamType& istream, ContainerType& container) -> std::enable_if_t<
#ifdef __cpp_variable_templates
    container_stream_io::traits::is_parseable_as_container_v<ContainerType>,
#else
    container_stream_io::traits::is_parseable_as_container<ContainerType>::value,
#endif
    StreamType&>
{
    using formatter_type =
        container_stream_io::input::default_formatter<ContainerType, StreamType>;
    container_stream_io::input::from_stream(istream, container, formatter_type{});

    return istream;
}
