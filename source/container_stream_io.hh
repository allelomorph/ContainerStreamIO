#pragma once

#include <cstdint>    // (u)int_XX_t
#include <algorithm>  // copy find_if for_each (limits:numeric_limits)
// #include <cstddef>    // size_t
#include <iostream>
#include <set>
#include <map>
#include <string>
#include <tuple>
#include <forward_list>
#include <utility>

#include <iomanip>   // setfill, setw
// #include <iterator>  // begin, end
// #include <type_traits>  // true_type, false_type
#include "TypeName.hh" // testing

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

// No feature test macro found for remove_const_t.
template< class T >
using remove_const_t = typename remove_const<T>::type;

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

template <typename Type>
struct is_char_variant : public std::false_type
{};

template <>
struct is_char_variant<char> : public std::true_type
{};

template <>
struct is_char_variant<wchar_t> : public std::true_type
{};

#if (__cplusplus > 201703L)
template <>
struct is_char_variant<char8_t> : public std::true_type
{};

#endif
template <>
struct is_char_variant<char16_t> : public std::true_type
{};

template <>
struct is_char_variant<char32_t> : public std::true_type
{};

/**
 * @brief Base case for the testing of STL compatible container types.
 */
template <typename Type, typename = void>
struct is_parseable_as_container : public std::false_type
{};

// !!! this generic omits basic_string_view (which misses clear()), as it should, but it
//   includes basic_string, which necessitates basic_string overload below
// !!! unlike the generic version of is_printable_as_container, this generic
//   does not include std::array, which lacks clear()
// TBD if we can commit to move-assigning a new container into the extraction target,
//   clear() may not be totally necessary
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
 * @brief Specialization to treat non-character C arrays as parseable container types.
 */
template <typename ArrayType, std::size_t ArraySize>
struct is_parseable_as_container<ArrayType[ArraySize],
                                 std::enable_if_t<!is_char_variant<ArrayType>::value,
                                                  void>> : public std::true_type
{};

/**
 * @brief Character C array specialization meant to ensure that we print
 * character arrays as strings and not as delimiter containers of individual
 * characters.
 */
template <typename CharType, std::size_t ArraySize>
struct is_parseable_as_container<CharType[ArraySize],
                                 std::enable_if_t<is_char_variant<CharType>::value,
                                                  void>> : public std::false_type
{};

// see note on default specialization above
/**
 * @brief String specialization meant to ensure that we treat strings as nothing
 * more than strings.
 */
template <typename CharacterType, typename CharacterTraitsType, typename AllocatorType>
struct is_parseable_as_container<
    std::basic_string<CharacterType, CharacterTraitsType, AllocatorType>>
    : public std::false_type
{};

// string_view should fail to meet default specialization
// #if (__cplusplus >= 201703L)
// template <typename CharacterType>
// struct is_printable_as_container<
//     std::basic_string_view<CharacterType>>
//     : public std::false_type
// {};
// #endif

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

// generic
/**
 * @brief Specialization to ensure that Standard Library compatible containers
 * that have begin(), end(), and empty() member functions are treated as
 * printable containers.
 */
template <typename Type>
struct is_printable_as_container<
    Type, std::void_t<typename Type::iterator,
                      decltype(std::declval<Type&>().begin()),
                      decltype(std::declval<Type&>().end()),
                      decltype(std::declval<Type&>().empty())>>
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
 * @brief Specialization to treat non-character C arrays as printable container types.
 */
template <typename ArrayType, std::size_t ArraySize>
struct is_printable_as_container<ArrayType[ArraySize],
                                 std::enable_if_t<!is_char_variant<ArrayType>::value,
                                                  void>> : public std::true_type
{};

/**
 * @brief Character C array specialization meant to ensure that we print
 * character arrays as strings and not as delimiter containers of individual
 * characters.
 */
template <typename CharType, std::size_t ArraySize>
struct is_printable_as_container<CharType[ArraySize],
                                 std::enable_if_t<is_char_variant<CharType>::value,
                                                  void>> : public std::false_type
{};

// exclude strings from generic
/**
 * @brief String specialization meant to ensure that we treat strings as nothing
 * more than strings.
 */
template <typename CharacterType, typename CharacterTraitsType, typename AllocatorType>
struct is_printable_as_container<
    std::basic_string<CharacterType, CharacterTraitsType, AllocatorType>>
    : public std::false_type
{};

#if (__cplusplus >= 201703L)
template <typename CharacterType>
struct is_printable_as_container<
    std::basic_string_view<CharacterType>>
    : public std::false_type
{};
#endif

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
struct has_iterless_emplace : public std::false_type
{};

// !!! This only tests for overloads that can take no args, eg emplace(args...)
//   for std::(unordered_)(multi)set and std::(unordered_)(multi)map. Containers
//   with emplace(const_iterator, args...), eg std::vector, std::list, and
//   std::deque could, be tested for with
//   `.emplace(declval<typename Type::const_iterator>())`, see:
//   detection idiom: https://stackoverflow.com/a/41936999
//   detection idiom mod for variadics: https://stackoverflow.com/a/35669421
template <typename Type>
struct has_iterless_emplace<Type, std::void_t<decltype(std::declval<Type&>().emplace())>>
    : public std::true_type
{};

template <typename Type, typename = void>
struct has_emplace_back : public std::false_type
{};

template <typename Type>
struct has_emplace_back<Type, std::void_t<decltype(std::declval<Type&>().emplace_back())>>
    : public std::true_type
{};

template <typename Type>
struct is_string_variant : public std::false_type
{};

template <typename CharType>
struct is_string_variant<CharType*> :
    public std::integral_constant<bool, is_char_variant<CharType>::value>
{};

template <typename CharType>
struct is_string_variant<const CharType*> :
    public std::integral_constant<bool, is_char_variant<CharType>::value>
{};

template <typename CharType>
struct is_string_variant<std::basic_string<CharType>> :
    public std::integral_constant<bool, is_char_variant<CharType>::value>
{};

// TBD const string and const string_view not really needed when
//   default_formatter print/parse funcs always pass const &
template <typename CharType>
struct is_string_variant<const std::basic_string<CharType>> :
    public std::integral_constant<bool, is_char_variant<CharType>::value>
{};

#if (__cplusplus >= 201703L)
template <typename CharType>
struct is_string_variant<std::basic_string_view<CharType>> :
    public std::integral_constant<bool, is_char_variant<CharType>::value>
{};

template <typename CharType>
struct is_string_variant<const std::basic_string_view<CharType>> :
    public std::integral_constant<bool, is_char_variant<CharType>::value>
{};
#endif  // C++17 and above

} // namespace traits

namespace strings {

namespace compile_time {

// functions to convert ascii string literals in source to appropriate char type,
//   adapted from:
//   - https://stackoverflow.com/a/60770220

#if (__cplusplus >= 201703L)

template<typename CharType>
constexpr const CharType char_literal(
    [[maybe_unused]] const char ac,
    [[maybe_unused]] const wchar_t wc,
#if (__cplusplus > 201703L)
    [[maybe_unused]] const char8_t u8c,
#endif
    [[maybe_unused]] const char16_t u16c,
    [[maybe_unused]] const char32_t u32c)
{
    if      constexpr( std::is_same_v<CharType, char> )      return ac;
    else if constexpr( std::is_same_v<CharType, wchar_t> )   return wc;
#if (__cplusplus > 201703L)
    else if constexpr( std::is_same_v<CharType, char8_t> )   return u8c;
#endif
    else if constexpr( std::is_same_v<CharType, char16_t> )  return u16c;
    else if constexpr( std::is_same_v<CharType, char32_t> )  return u32c;
}

template<typename CharType,
         std::size_t SizeAscii, std::size_t SizeWide,
#if (__cplusplus > 201703L)
         std::size_t SizeUTF8,
#endif
         std::size_t SizeUTF16, std::size_t SizeUTF32,
         std::size_t Size = (
         std::is_same_v<CharType, char>     ? SizeAscii :
         std::is_same_v<CharType, wchar_t>  ? SizeWide  :
#if (__cplusplus > 201703L)
         std::is_same_v<CharType, char8_t>  ? SizeUTF8  :
#endif
         std::is_same_v<CharType, char16_t> ? SizeUTF16 :
         std::is_same_v<CharType, char32_t> ? SizeUTF32 : 0 ) >
constexpr auto string_literal(
    [[maybe_unused]] const char (&as) [SizeAscii],
    [[maybe_unused]] const wchar_t (&ws) [SizeWide],
#if (__cplusplus > 201703L)
    [[maybe_unused]] const char8_t (&u8s) [SizeUTF8],
#endif
    [[maybe_unused]] const char16_t (&u16s) [SizeUTF16],
    [[maybe_unused]] const char32_t (&u32s) [SizeUTF32]
        ) -> const CharType(&)[Size]
{
    if      constexpr( std::is_same_v<CharType, char> )     return as;
    else if constexpr( std::is_same_v<CharType, wchar_t> )  return ws;
#if (__cplusplus > 201703L)
    else if constexpr( std::is_same_v<CharType, char8_t> )  return u8s;
#endif
    else if constexpr( std::is_same_v<CharType, char16_t> ) return u16s;
    else if constexpr( std::is_same_v<CharType, char32_t> ) return u32s;
}

#else // __cplusplus < 201703L

// TBD make char_ and string_literal inline?
// full template specialization appropriate
template<typename CharType>
constexpr CharType char_literal(
    const char ac, const wchar_t wc, const char16_t u16c, const char32_t u32c);

template<>
constexpr char char_literal(
    const char ac, const wchar_t /*wc*/, const char16_t /*u16c*/, const char32_t /*u32c*/)
{
    return ac;
}

template<>
constexpr wchar_t char_literal(
    const char /*ac*/, const wchar_t wc, const char16_t /*u16c*/, const char32_t /*u32c*/)
{
    return wc;
}

template<>
constexpr char16_t char_literal(
    const char /*ac*/, const wchar_t /*wc*/, const char16_t u16c, const char32_t /*u32c*/)
{
    return u16c;
}

template<>
constexpr char32_t char_literal(
    const char /*ac*/, const wchar_t /*wc*/, const char16_t /*u16c*/, const char32_t u32c)
{
    return u32c;
}

// partial function template specialization not allowed by standard, see:
//   - https://stackoverflow.com/a/21218271
//   so instead using overloads each templated by literal sizes and enabled by
//   return char type,
template<typename CharType,
         std::size_t SizeAscii, std::size_t SizeWide,
         std::size_t SizeUTF16, std::size_t SizeUTF32>
constexpr auto string_literal(
    const char (&as)[SizeAscii], const wchar_t (&/*ws*/)[SizeWide],
    const char16_t (&/*u16s*/)[SizeUTF16], const char32_t (&/*u32s*/)[SizeUTF32]
    ) -> std::enable_if_t<
        std::is_same<CharType, char>::value,
        const char(&)[SizeAscii]>
{
    return as;
}

template<typename CharType,
         std::size_t SizeAscii, std::size_t SizeWide,
         std::size_t SizeUTF16, std::size_t SizeUTF32>
constexpr auto string_literal(
    const char (&/*as*/)[SizeAscii], const wchar_t (&ws)[SizeWide],
    const char16_t (&/*u16s*/)[SizeUTF16], const char32_t (&/*u32s*/)[SizeUTF32]
    ) -> std::enable_if_t<
        std::is_same<CharType, wchar_t>::value,
        const wchar_t(&)[SizeWide]>
{
    return ws;
}

template<typename CharType,
         std::size_t SizeAscii, std::size_t SizeWide,
         std::size_t SizeUTF16, std::size_t SizeUTF32>
constexpr auto string_literal(
    const char (&/*as*/)[SizeAscii], const wchar_t (&/*ws*/)[SizeWide],
    const char16_t (&u16s)[SizeUTF16], const char32_t (&/*u32s*/)[SizeUTF32]
    ) -> std::enable_if_t<
        std::is_same<CharType, char16_t>::value,
        const char16_t(&)[SizeUTF16]>
{
    return u16s;
}

template<typename CharType,
         std::size_t SizeAscii, std::size_t SizeWide,
         std::size_t SizeUTF16, std::size_t SizeUTF32>
constexpr auto string_literal(
    const char (&/*as*/)[SizeAscii], const wchar_t (&/*ws*/)[SizeWide],
    const char16_t (&/*u16s*/)[SizeUTF16], const char32_t (&u32s)[SizeUTF32]
    ) -> std::enable_if_t<
        std::is_same<CharType, char32_t>::value,
        const char32_t(&)[SizeUTF32]>
{
    return u32s;
}

#endif

#if (__cplusplus > 201703L)

#define  CHAR_LITERAL(CHAR_T, LITERAL) \
    char_literal<CHAR_T>( LITERAL, L ## LITERAL, u8 ## LITERAL, \
                          u ## LITERAL, U ## LITERAL )

#define  STRING_LITERAL(CHAR_T, LITERAL) \
    string_literal<CHAR_T>( LITERAL, L ## LITERAL, u8 ## LITERAL, \
                            u ## LITERAL, U ## LITERAL )

#else  // __cplusplus <= 201703L

#define  CHAR_LITERAL(CHAR_T, LITERAL) \
    char_literal<CHAR_T>( LITERAL, L ## LITERAL, u ## LITERAL, U ## LITERAL )

#define  STRING_LITERAL(CHAR_T, LITERAL) \
    string_literal<CHAR_T>( LITERAL, L ## LITERAL, u ## LITERAL, U ## LITERAL )

#endif

} // namespace compile_time

// quoted/literal implementation (string_repr, operator<</>>(string_repr),
//   quoted(), literal()) based on glibc C++14 std::quoted (as seen in iomanip
//   and bits/quoted_string.h headers;) redone here to add features and allow
//   use in C++11

namespace detail {

// quoted encoding:
// - only delim/escape escaped
// - can choose any, even non-ascii, delim/escape
// - only enabled if sizeof(StreamCharType) >= sizeof(StringCharType), as
//     non-delim/escape chars passed directly

// literal encoding:
// - limited to printable ascii via:
//   - only printable ascii delim/escape
//   - delim/escape escaped
//   - standard unprintable ascii escape seqs
//   - other unprintable ascii hex escaped
//   - values beyond 7-bit range (0x7f) escaped
// - enabled for any combo of string and stream char type

// quoted decoding:
// - only enabled if sizeof(StreamCharType) <= sizeof(StringCharType), as
//     non-delim/escape chars passed directly

// literal decoding:
// - enabled for any combo of string and stream char type, but failbit will be
//     set by hex widths beyond sizeof(StringCharType)

// labels for flag values used to set string representation type
enum class repr_type { literal, quoted };

// stream index getter for use with iword/pword to set literalrepr/quotedrepr
static inline int get_manip_i()
{
    static int i {std::ios_base::xalloc()};
    return i;
}

template<typename StringType, typename CharType>
struct string_repr
{
private:
    // given char and wchar_t share the same set of standard escapes, and
    //   Unicode code points below 0x7f map to 7-bit ASCII, escapes treated the
    //   same for all char types
    // wrapping in struct to avoid variable template use for C++11 compliance
    struct ascii_escapes
    {
        static std::map<CharType, CharType> by_value;
        static std::map<CharType, CharType> by_symbol;
    };

public:
    // TBD change to SFINAE with enable_if
    static_assert(std::is_pointer<StringType>::value ||
                  std::is_reference<StringType>::value,
                  "String type must be a pointer or reference");

    StringType string;
    CharType delim;
    CharType escape;
    repr_type type;

    static constexpr ascii_escapes escapes {};

    string_repr() = delete;
    string_repr(const StringType str, const CharType dlm,
                const CharType esc, const repr_type typ) :
        string{str}, delim{dlm}, escape{esc}, type{typ}
    {
        if (type == repr_type::literal &&
            (dlm > 0x7f || !std::isprint(dlm) ||
             esc > 0x7f || !std::isprint(esc)))
            throw(std::invalid_argument(
                      "literal delim and escape must be printable 7-bit ASCII characters"));
    }

    string_repr& operator=(string_repr&) = delete;
};

#if (__cplusplus < 201703L)

// linker needs declaration of static constexpr members outside class for
//   standards below C++17, see:
//   - https://en.cppreference.com/w/cpp/language/static
//   - https://stackoverflow.com/a/28846608
template<typename StringType, typename CharType>
constexpr typename string_repr<StringType, CharType>::ascii_escapes string_repr<StringType, CharType>::escapes;

#endif  // pre-C++17

// non-literal (std::map has non-trivial dtor) static members must be
//   initialized outside class definition (per gcc)
// \', \", and \\ handled by choosing custom delim/escape chars
// legacy C trigraphs and "\?" -> '?' escaping ignored for now, see:
//   - https://en.cppreference.com/w/cpp/language/escape
//   - https://en.cppreference.com/w/c/language/operator_alternative
template<typename StringType, typename CharType>
std::map<CharType, CharType> string_repr<StringType, CharType>::ascii_escapes::by_value
{
    {
        {'\a', 'a'}, {'\b', 'b'}, {'\f', 'f'}, {'\n', 'n'},
        {'\r', 'r'}, {'\t', 't'}, {'\v', 'v'}, {'\0', '0'}
    }
};

template<typename StringType, typename CharType>
std::map<CharType, CharType> string_repr<StringType, CharType>::ascii_escapes::by_symbol
{
    {
        {'a', '\a'}, {'b', '\b'}, {'f', '\f'}, {'n', '\n'},
        {'r', '\r'}, {'t', '\t'}, {'v', '\v'}, {'0', '\0'}
    }
};

// insert_literal_prefix and insert_escaped_char help to shorten
//   operator<<(string_repr) overload definitions
template<typename StreamCharType, typename StringCharType>
static void insert_literal_prefix(
        std::basic_ostream<StreamCharType>& os)
{
    using namespace compile_time;  // char_literal, string_literal
    if (std::is_same<StringCharType, wchar_t>::value)
        os << CHAR_LITERAL(StreamCharType, 'L');
#if (__cplusplus > 201703L)
    if (std::is_same<StringCharType, char8_t>::value)
        os << STRING_LITERAL(StreamCharType, "u8");
#endif
    if (std::is_same<StringCharType, char16_t>::value)
        os << CHAR_LITERAL(StreamCharType, 'u');
    if (std::is_same<StringCharType, char32_t>::value)
        os << CHAR_LITERAL(StreamCharType, 'U');
}

template<typename StreamCharType, typename StringType, typename StringCharType>
static void insert_escaped_char(
    std::basic_ostream<StreamCharType>& os,
    const string_repr<StringType, StringCharType>& repr,
    const StringCharType c)
{
    static constexpr uint32_t hex_mask {
        (sizeof(StringCharType) == 1) ? 0xff :
        (sizeof(StringCharType) == 2) ? 0xffff :
                                        0xffffffff };

    if (c < 0x7f && std::isprint(c))
    {
        // literal_repr ctor enforces ASCII-printable delim and escape
        if (c == repr.delim || c == repr.escape)
            os << StreamCharType(repr.escape);
        os << StreamCharType(c);
    }
    else
    {
        os << StreamCharType(repr.escape);
        try {
            os << StreamCharType(repr.escapes.by_value.at(c));
        } catch (const std::out_of_range& /*oor_ex*/) {
            // custom hex escape sequence
            os << StreamCharType('x') <<
                std::hex << std::setfill(StreamCharType('0')) <<
                std::setw(2 * sizeof(StringCharType)) <<
                (hex_mask & static_cast<uint32_t>(c));
        }
    }
}

// TBD maybe throw exeception rather than set failbit on quoted char size failure?

// template<typename StreamCharType, typename StringType, typename StringCharType>
// auto operator<<(
//     std::basic_ostream<StreamCharType>& ostream,
//     const string_repr<StringType, StringCharType>& repr
//     ) -> std::enable_if_t<
//         std::is_same<StringType, StringCharType&>::value ||
//         std::is_same<StringType, const StringCharType&>::value,
//         std::basic_ostream<StreamCharType>&>
// {
//     if (repr.type == repr_type::quoted &&
//         sizeof(StreamCharType) > sizeof(StringCharType))
//     {
//         ostream.setstate(std::ios_base::failbit);
//         return ostream;
//     }
//     std::basic_ostringstream<StreamCharType> oss;
//     insert_literal_prefix<StreamCharType, StringCharType>(oss);
//     oss << StreamCharType(repr.delim);
//     insert_escaped_char(oss, repr, repr.string);
//     oss << StreamCharType(repr.delim);
//     return ostream << oss.str();
// }

template<typename StreamCharType, typename StringCharType>
auto operator<<(
    std::basic_ostream<StreamCharType>& ostream,
    const string_repr<StringCharType&, StringCharType>& repr
    ) -> std::basic_ostream<StreamCharType>&
{
    if (repr.type == repr_type::quoted &&
        sizeof(StreamCharType) < sizeof(StringCharType))
    {
        ostream.setstate(std::ios_base::failbit);
        return ostream;
    }
    std::basic_ostringstream<StreamCharType> oss;
    insert_literal_prefix<StreamCharType, StringCharType>(oss);
    oss << StreamCharType(repr.delim);
    if (repr.type == repr_type::quoted)
    {
        if (repr.string == repr.delim || repr.string == repr.escape)
            oss << StreamCharType(repr.escape);
        oss << StreamCharType(repr.string);
    }
    else
        insert_escaped_char(oss, repr, repr.string);
    oss << StreamCharType(repr.delim);
    return ostream << oss.str();
}

template<typename StreamCharType, typename StringCharType>
auto operator<<(
    std::basic_ostream<StreamCharType>& ostream,
    const string_repr<const StringCharType&, StringCharType>& repr
    ) -> std::basic_ostream<StreamCharType>&
{
    if (repr.type == repr_type::quoted &&
        sizeof(StreamCharType) < sizeof(StringCharType))
    {
        ostream.setstate(std::ios_base::failbit);
        return ostream;
    }
    std::basic_ostringstream<StreamCharType> oss;
    insert_literal_prefix<StreamCharType, StringCharType>(oss);
    oss << StreamCharType(repr.delim);
    if (repr.type == repr_type::quoted)
    {
        if (repr.string == repr.delim || repr.string == repr.escape)
            oss << StreamCharType(repr.escape);
        oss << StreamCharType(repr.string);
    }
    else
        insert_escaped_char(oss, repr, repr.string);
    oss << StreamCharType(repr.delim);
    return ostream << oss.str();
}

// template<typename StreamCharType, typename StringCharType>
// auto operator<<(
//     std::basic_ostream<StreamCharType>& ostream,
//     const string_repr<const StringCharType*, StringCharType>& repr
//     ) -> std::basic_ostream<StreamCharType>&
// {
//     if (repr.type == repr_type::quoted &&
//         sizeof(StreamCharType) < sizeof(StringCharType))
//     {
//         ostream.setstate(std::ios_base::failbit);
//         return ostream;
//     }
//     std::basic_ostringstream<StreamCharType> oss;
//     insert_literal_prefix<StreamCharType, StringCharType>(oss);
//     oss << StreamCharType(repr.delim);
//     for (auto p { repr.string }; *p; ++p)
//     {
//         insert_escaped_char(oss, repr, *p);
//     }
//     oss << StreamCharType(repr.delim);
//     return ostream << oss.str();
// }

template<typename StreamCharType, typename StringCharType>
auto operator<<(
    std::basic_ostream<StreamCharType>& ostream,
    const string_repr<StringCharType*, StringCharType>& repr
    ) -> std::basic_ostream<StreamCharType>&
{
    if (repr.type == repr_type::quoted &&
        sizeof(StreamCharType) < sizeof(StringCharType))
    {
        ostream.setstate(std::ios_base::failbit);
        return ostream;
    }
    std::basic_ostringstream<StreamCharType> oss;
    insert_literal_prefix<StreamCharType, StringCharType>(oss);
    oss << StreamCharType(repr.delim);
    if (repr.type == repr_type::quoted)
    {
        for (auto p { repr.string }; *p; ++p)
        {
            if (*p == repr.delim || *p == repr.escape)
                oss << StreamCharType(repr.escape);
            oss << StreamCharType(*p);
        }
    }
    else
    {
        for (auto p { repr.string }; *p; ++p)
            insert_escaped_char(oss, repr, *p);
    }
    oss << StreamCharType(repr.delim);
    return ostream << oss.str();
}

template<typename StreamCharType, typename StringCharType>
auto operator<<(
    std::basic_ostream<StreamCharType>& ostream,
    const string_repr<const StringCharType*, StringCharType>& repr
    ) -> std::basic_ostream<StreamCharType>&
{
    if (repr.type == repr_type::quoted &&
        sizeof(StreamCharType) < sizeof(StringCharType))
    {
        ostream.setstate(std::ios_base::failbit);
        return ostream;
    }
    std::basic_ostringstream<StreamCharType> oss;
    insert_literal_prefix<StreamCharType, StringCharType>(oss);
    oss << StreamCharType(repr.delim);
    if (repr.type == repr_type::quoted)
    {
        for (auto p { repr.string }; *p; ++p)
        {
            if (*p == repr.delim || *p == repr.escape)
                oss << StreamCharType(repr.escape);
            oss << StreamCharType(*p);
        }
    }
    else
    {
        for (auto p { repr.string }; *p; ++p)
            insert_escaped_char(oss, repr, *p);
    }
    oss << StreamCharType(repr.delim);
    return ostream << oss.str();
}

// template<typename StreamCharType, typename StringType, typename StringCharType>
// auto operator<<(
//     std::basic_ostream<StreamCharType>& ostream,
//     const string_repr<StringType, StringCharType>& repr
//     ) -> std::enable_if_t<
//         !std::is_same<StringType, StringCharType&>::value &&
//         !std::is_same<StringType, const StringCharType&>::value,
//         std::basic_ostream<StreamCharType>&>
// {
//     if (repr.type == repr_type::quoted &&
//         sizeof(StreamCharType) < sizeof(StringCharType))
//     {
//         ostream.setstate(std::ios_base::failbit);
//         return ostream;
//     }
//     std::basic_ostringstream<StreamCharType> oss;
//     insert_literal_prefix<StreamCharType, StringCharType>(oss);
//     oss << StreamCharType(repr.delim);
//     for (const auto c : repr.string)
//     {
//         insert_escaped_char(oss, repr, c);
//     }
//     oss << StreamCharType(repr.delim);
//     return ostream << oss.str();
// }

template<typename StreamCharType, typename StringType, typename StringCharType>
auto operator<<(
    std::basic_ostream<StreamCharType>& ostream,
    const string_repr<StringType, StringCharType>& repr
    ) -> std::basic_ostream<StreamCharType>&
{
    if (repr.type == repr_type::quoted &&
        sizeof(StreamCharType) < sizeof(StringCharType))
    {
        ostream.setstate(std::ios_base::failbit);
        return ostream;
    }
    std::basic_ostringstream<StreamCharType> oss;
    insert_literal_prefix<StreamCharType, StringCharType>(oss);
    oss << StreamCharType(repr.delim);
    if (repr.type == repr_type::quoted)
    {
        for (const auto c : repr.string)
        {
            if (c == repr.delim || c == repr.escape)
                oss << StreamCharType(repr.escape);
            oss << StreamCharType(c);
        }
    }
    else
    {
        for (const auto c : repr.string)
            insert_escaped_char(oss, repr, c);
    }
    oss << StreamCharType(repr.delim);
    return ostream << oss.str();
}

template<typename StreamCharType, typename StringCharType>
static void extract_literal_prefix(
    std::basic_istream<StreamCharType>& istream)
{
    if (std::is_same<StringCharType, wchar_t>::value &&
        istream.get() != StreamCharType('L'))
        istream.setstate(std::ios_base::failbit);

#if (__cplusplus > 201703L)
    if (std::is_same<StringCharType, char8_t>::value &&
        (istream.get() != StreamCharType('u') ||
         istream.get() != StreamCharType('8')))
        istream.setstate(std::ios_base::failbit);
#endif

    if (std::is_same<StringCharType, char16_t>::value &&
        istream.get() != StreamCharType('u'))
        istream.setstate(std::ios_base::failbit);

    if (std::is_same<StringCharType, char32_t>::value &&
        istream.get() != StreamCharType('U'))
        istream.setstate(std::ios_base::failbit);
}

template<typename StreamCharType, typename StringCharType>
static int64_t extract_fixed_width_hex_value(
    std::basic_istream<StreamCharType>& istream)
{
    constexpr uint32_t hex_length { sizeof(StringCharType) * 2 };
    char buff[hex_length + 1] {};
    // strtol expects char* and wcstol expects wchar_t*, which is variable size -
    //   in either case, we can't rely on implicit casting with something like
    //   `istream.get(buff, hex_length + 1);`, as malformed hex strings could
    //   have values larger than StreamCharType max, with unpredictable overflows
    char *p { buff };
    StreamCharType c;
    for (uint32_t i {}; istream.good() && i < hex_length; ++i, ++p)
    {
        istream >> c;
        if (c > 0x7f || !isxdigit(c))
            break;
        *p = c;
    }
    if (p != &buff[hex_length])
        istream.setstate(std::ios_base::failbit);
    // strtol returns signed values, but interprets hex as unsigned
    return std::strtol(buff, nullptr, 16);
}

template<typename StreamCharType, typename StringCharType>
void extract_quoted_repr(
    std::basic_istream<StreamCharType>& istream,
    const string_repr<std::basic_string<StringCharType>&, StringCharType>& repr,
    std::basic_string<StringCharType>& buffer)
{
    StreamCharType c;
    std::ios_base::fmtflags orig_flags {
        istream.flags(istream.flags() & ~std::ios_base::skipws) };
    for (istream >> c;
         istream.good() && c != StreamCharType(repr.delim); istream >> c)
    {
        if (c != StreamCharType(repr.escape))
        {
            buffer += StringCharType(c);
            continue;
        }
        istream >> c;
        if (istream.good())
        {
            if (c == StreamCharType(repr.escape) ||
                c == StreamCharType(repr.delim))
                buffer += StringCharType(c);
            else
               istream.setstate(std::ios_base::failbit);  // invalid quoted encoding
        }
    };
    if (c != StreamCharType(repr.delim))
        istream.setstate(std::ios_base::failbit);
    istream.setf(orig_flags);
}

template<typename StreamCharType, typename StringCharType>
void extract_literal_repr(
    std::basic_istream<StreamCharType>& istream,
    const string_repr<std::basic_string<StringCharType>&, StringCharType>& repr,
    std::basic_string<StringCharType>& buffer)
{
    StreamCharType c;
    std::ios_base::fmtflags orig_flags {
        istream.flags(istream.flags() & ~std::ios_base::skipws) };
    for (istream >> c;
         istream.good() && c != StreamCharType(repr.delim); istream >> c)
    {
        if (c < 0x7f && std::isprint(c))
        {
            if (c != StreamCharType(repr.escape))
            {
                buffer += StringCharType(c);
                continue;
            }
            istream >> c;
            if (c > 0x7f || !std::isprint(c))
                istream.setstate(std::ios_base::failbit);  // invalid escape
            if (!istream.good())
                break;
            if (c == StreamCharType(repr.escape) ||
                c == StreamCharType(repr.delim))
            {
                buffer += StringCharType(c);
                continue;
            }
            try
            {
                buffer += repr.escapes.by_symbol.at(c);
                continue;
            }
            catch (const std::out_of_range& /*oor_ex*/)
            {
                if (c == StreamCharType('x'))
                {
                    // !!? can we pass only StringCharType to template?
                    buffer += StringCharType(
                        extract_fixed_width_hex_value<
                        StreamCharType, StringCharType>(istream));
                    continue;
                }
            }
        }
        istream.setstate(std::ios_base::failbit);  // invalid literal encoding
    };
    if (c != StreamCharType(repr.delim))
        istream.setstate(std::ios_base::failbit);
    istream.setf(orig_flags);
}

template<typename StreamCharType, typename StringCharType>
void extract_string_repr(
    std::basic_istream<StreamCharType>& istream,
    const string_repr<std::basic_string<StringCharType>&, StringCharType>& repr)
{
    // quoted encoding expects full potential range of StreamCharType values,
    //   and so could create overflow if casting to a smaller StringCharType,
    //   whereas literal encoding expects only printable 7-bit ASCII values due
    //   to escapes
    if (repr.type == repr_type::quoted &&
        sizeof(StreamCharType) > sizeof(StringCharType))
    {
        istream.setstate(std::ios_base::failbit);
        return;
    }
    // !!? can we pass only StringCharType to template?
    extract_literal_prefix<StreamCharType, StringCharType>(istream);
    if (!istream.good())
        return;
    // get() returns std::basic_istream<StreamCharType>::int_type
    if (StreamCharType(istream.get()) != StreamCharType(repr.delim))
        istream.setstate(std::ios_base::failbit);
    if (!istream.good())
        return;
    std::basic_string<StringCharType> temp;
    if (repr.type == repr_type::quoted)
        extract_quoted_repr(istream, repr, temp);
    else
        extract_literal_repr(istream, repr, temp);
    if (istream.good())
        repr.string = std::move(temp);
}

template<typename StreamCharType, typename StringCharType>
auto operator>>(
    std::basic_istream<StreamCharType>& istream,
    const string_repr<StringCharType&, StringCharType>& repr
    ) -> std::basic_istream<StreamCharType>&
{
    std::basic_string<StringCharType> temp;
    string_repr<std::basic_string<StringCharType>&, StringCharType> temp_repr {
        temp, repr.delim, repr.escape, repr.type };
    extract_string_repr(istream, temp_repr);
    if (!istream.fail() && temp_repr.string.size() == 1)
        repr.string = temp_repr.string[0];
    else
        istream.setstate(std::ios_base::failbit);
    return istream;
}

template<typename StreamCharType, typename StringCharType>
auto operator>>(
    std::basic_istream<StreamCharType>& istream,
    const string_repr<std::basic_string<StringCharType>&, StringCharType>& repr
    ) -> std::basic_istream<StreamCharType>&
{
    extract_string_repr(istream, repr);
    return istream;
}

}  // namespace detail

// default is string literals, with most ASCII escapes plus hex escapes
template<typename CharacterType, typename TraitsType>
std::basic_ios<CharacterType, TraitsType>& literalrepr(
    std::basic_ios<CharacterType, TraitsType>& stream)
{
    stream.iword(detail::get_manip_i()) =
        static_cast<int>(detail::repr_type::literal);
    return stream;
}

// only escape char and delimiter escaped
template<typename CharacterType, typename TraitsType>
std::basic_ios<CharacterType, TraitsType>& quotedrepr(
    std::basic_ios<CharacterType, TraitsType>& stream)
{
    stream.iword(detail::get_manip_i()) =
        static_cast<int>(detail::repr_type::quoted);
    return stream;
}

/**
 * @brief Manipulator for quoted strings.
 * @param string String to quote.
 * @param delim  Character to quote string with.
 * @param escape Escape character to escape itself or quote character.
 */
template<typename CharType>
inline auto quoted(
    CharType& c,
    CharType delim = CharType('\''), CharType escape = CharType('\\')
    ) -> std::enable_if_t<
        traits::is_char_variant<CharType>::value,
        detail::string_repr<CharType&, CharType>>
{
    return detail::string_repr<CharType&, CharType>(
            c, delim, escape, detail::repr_type::quoted);
}

template<typename CharType>
inline auto quoted(
    const CharType& c,
    CharType delim = CharType('\''), CharType escape = CharType('\\')
    ) -> std::enable_if_t<
        traits::is_char_variant<CharType>::value,
        detail::string_repr<const CharType&, CharType>>
{
    return detail::string_repr<const CharType&, CharType>(
            c, delim, escape, detail::repr_type::quoted);
}

template<typename CharType>
inline auto quoted(
    CharType* string,
    CharType delim = CharType('"'), CharType escape = CharType('\\')
    ) -> detail::string_repr<CharType*, CharType>
{
    return detail::string_repr<CharType*, CharType>(
            string, delim, escape, detail::repr_type::quoted);
}

template<typename CharType>
inline auto quoted(
    const CharType* string,
    CharType delim = CharType('"'), CharType escape = CharType('\\')
    ) -> detail::string_repr<const CharType*, CharType>
{
    return detail::string_repr<const CharType*, CharType>(
            string, delim, escape, detail::repr_type::quoted);
}

template<typename CharType, typename TraitsType, typename AllocType>
inline auto quoted(
    std::basic_string<CharType, TraitsType, AllocType>& string,
    CharType delim = CharType('"'), CharType escape = CharType('\\')
    ) -> detail::string_repr<
        std::basic_string<CharType, TraitsType, AllocType>&, CharType>
{
    return detail::string_repr<
	std::basic_string<CharType, TraitsType, AllocType>&, CharType>(
	    string, delim, escape, detail::repr_type::quoted);
}

// note: using decltype(string) for string_repr<StringType, > fails to assign const reference
template<typename CharType, typename TraitsType, typename AllocType>
inline auto quoted(
    const std::basic_string<CharType, TraitsType, AllocType>& string,
    CharType delim = CharType('"'), CharType escape = CharType('\\')
    ) -> detail::string_repr<
        const std::basic_string<CharType, TraitsType, AllocType>&, CharType>
{
    return detail::string_repr<
	const std::basic_string<CharType, TraitsType, AllocType>&, CharType>(
	    string, delim, escape, detail::repr_type::quoted);
}

#if __cplusplus >= 201703L
template<typename CharType, typename TraitsType>
inline auto quoted(
    std::basic_string_view<CharType, TraitsType>& string_view,
    CharType delim = CharType('"'), CharType escape = CharType('\\')
    ) -> detail::string_repr<
        std::basic_string_view<CharType, TraitsType>&, CharType>
{
    return detail::string_repr<
        std::basic_string_view<CharType, TraitsType>&, CharType>(
	    string_view, delim, escape, detail::repr_type::quoted);
}

template<typename CharType, typename TraitsType>
inline auto quoted(
    const std::basic_string_view<CharType, TraitsType>& string_view,
    CharType delim = CharType('"'), CharType escape = CharType('\\')
    ) -> detail::string_repr<
        const std::basic_string_view<CharType, TraitsType>&, CharType>
{
    return detail::string_repr<
        const std::basic_string_view<CharType, TraitsType>&, CharType>(
	    string_view, delim, escape, detail::repr_type::quoted);
}
#endif  // C++17

template<typename CharType>
inline auto literal(
    CharType& c,
    CharType delim = CharType('\''), CharType escape = CharType('\\')
    ) -> std::enable_if_t<
        traits::is_char_variant<CharType>::value,
        detail::string_repr<CharType&, CharType>>
{
    return detail::string_repr<CharType&, CharType>(
            c, delim, escape, detail::repr_type::literal);
}

template<typename CharType>
inline auto literal(
    const CharType& c,
    CharType delim = CharType('\''), CharType escape = CharType('\\')
    ) -> std::enable_if_t<
        traits::is_char_variant<CharType>::value,
        detail::string_repr<const CharType&, CharType>>
{
    return detail::string_repr<const CharType&, CharType>(
            c, delim, escape, detail::repr_type::literal);
}

template<typename CharType>
inline auto literal(
    CharType* string,
    CharType delim = CharType('"'), CharType escape = CharType('\\')
    ) -> detail::string_repr<CharType*, CharType>
{
    return detail::string_repr<CharType*, CharType>(
            string, delim, escape, detail::repr_type::literal);
}

template<typename CharType>
inline auto literal(
    const CharType* string,
    CharType delim = CharType('"'), CharType escape = CharType('\\')
    ) -> detail::string_repr<const CharType*, CharType>
{
    return detail::string_repr<const CharType*, CharType>(
            string, delim, escape, detail::repr_type::literal);
}

template<typename CharType, typename TraitsType, typename AllocType>
inline auto literal(
    std::basic_string<CharType, TraitsType, AllocType>& string,
    CharType delim = CharType('"'), CharType escape = CharType('\\')
    ) -> detail::string_repr<
        std::basic_string<CharType, TraitsType, AllocType>&, CharType>
{
    return detail::string_repr<
	std::basic_string<CharType, TraitsType, AllocType>&, CharType>(
	    string, delim, escape, detail::repr_type::literal);
}

// note: using decltype(string) for string_repr<StringType, > fails to assign const reference
template<typename CharType, typename TraitsType, typename AllocType>
inline auto literal(
    const std::basic_string<CharType, TraitsType, AllocType>& string,
    CharType delim = CharType('"'), CharType escape = CharType('\\')
    ) -> detail::string_repr<
        const std::basic_string<CharType, TraitsType, AllocType>&, CharType>
{
    return detail::string_repr<
	const std::basic_string<CharType, TraitsType, AllocType>&, CharType>(
	    string, delim, escape, detail::repr_type::literal);
}

#if __cplusplus >= 201703L
template<typename CharType, typename TraitsType>
inline auto literal(
    std::basic_string_view<CharType, TraitsType>& string_view,
    CharType delim = CharType('"'), CharType escape = CharType('\\')
    ) -> detail::string_repr<
        std::basic_string_view<CharType, TraitsType>&, CharType>
{
    return detail::string_repr<
        std::basic_string_view<CharType, TraitsType>&, CharType>(
	    string_view, delim, escape, detail::repr_type::literal);
}

template<typename CharType, typename TraitsType>
inline auto literal(
    const std::basic_string_view<CharType, TraitsType>& string_view,
    CharType delim = CharType('"'), CharType escape = CharType('\\')
    ) -> detail::string_repr<
        const std::basic_string_view<CharType, TraitsType>&, CharType>
{
    return detail::string_repr<
        const std::basic_string_view<CharType, TraitsType>&, CharType>(
	    string_view, delim, escape, detail::repr_type::literal);
}
#endif  // C++17

}  // namespace strings

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

using namespace strings::compile_time;  // char_literal, string_literal

/**
 * @brief Default for any container type that isn't even more specialized.
 */
template <typename ContainerType, typename CharacterType>
struct delimiters
{
    static constexpr delim_wrapper<CharacterType> values {
        STRING_LITERAL(CharacterType, "["),
        STRING_LITERAL(CharacterType, ","),
        STRING_LITERAL(CharacterType, " "),
        STRING_LITERAL(CharacterType, "]") };
};

/**
 * @brief Specialization for std::set<...> instances.
 */
template <typename DataType, typename ComparatorType, typename AllocatorType,
          typename CharacterType>
struct delimiters<std::set<DataType, ComparatorType, AllocatorType>, CharacterType>
{
    static constexpr delim_wrapper<CharacterType> values {
        STRING_LITERAL(CharacterType, "{"),
        STRING_LITERAL(CharacterType, ","),
        STRING_LITERAL(CharacterType, " "),
        STRING_LITERAL(CharacterType, "}") };
};

/**
 * @brief Specialization for std::multiset<...> instances.
 */
template <typename DataType, typename ComparatorType, typename AllocatorType,
          typename CharacterType>
struct delimiters<std::multiset<DataType, ComparatorType, AllocatorType>, CharacterType>
{
    static constexpr delim_wrapper<CharacterType> values {
        STRING_LITERAL(CharacterType, "{"),
        STRING_LITERAL(CharacterType, ","),
        STRING_LITERAL(CharacterType, " "),
        STRING_LITERAL(CharacterType, "}") };
};

/**
 * @brief Specialization for std::pair<...> instances.
 */
template <typename FirstType, typename SecondType,
          typename CharacterType>
struct delimiters<std::pair<FirstType, SecondType>, CharacterType>
{
    static constexpr delim_wrapper<CharacterType> values {
        STRING_LITERAL(CharacterType, "("),
        STRING_LITERAL(CharacterType, ","),
        STRING_LITERAL(CharacterType, " "),
        STRING_LITERAL(CharacterType, ")") };
};

/**
 * @brief Specialization for std::tuple<...> instances.
 */
template <typename... DataType,
          typename CharacterType>
struct delimiters<std::tuple<DataType...>, CharacterType>
{
    static constexpr delim_wrapper<CharacterType> values {
        STRING_LITERAL(CharacterType, "<"),
        STRING_LITERAL(CharacterType, ","),
        STRING_LITERAL(CharacterType, " "),
        STRING_LITERAL(CharacterType, ">") };
};

} // namespace decorator

namespace input {

template <typename ContainerType, typename StreamType>
struct default_formatter
{
    using repr_type = strings::detail::repr_type;
    using stream_char_type = typename StreamType::char_type;

    static constexpr auto decorators = container_stream_io::decorator::delimiters<
        ContainerType, stream_char_type>::values;

    // attempts extraction of exact token
    static void extract_token(
        StreamType& istream, const stream_char_type* token)
    {
        if (token == nullptr)
        {
            istream.setstate(std::ios_base::failbit);
            return;
        }
        auto token_s {
#if (__cplusplus >= 201703L)
            std::basic_string_view<stream_char_type> { token }
#else
            std::basic_string<stream_char_type> { token }
#endif
        };
        istream >> std::ws;
        auto it_1 { token_s.begin() };
        while (istream.good() && it_1 != token_s.end() &&
               stream_char_type(istream.peek()) == *it_1)
        {
            istream.get();
            ++it_1;
        }
        if (it_1 != token_s.end())
            istream.setstate(std::ios_base::failbit);
    }

    static void parse_prefix(StreamType& istream) noexcept
    {
        extract_token(istream, decorators.prefix);
    }

    template<typename ElementType>
    static auto parse_element(StreamType& istream, ElementType& element
        ) noexcept -> std::enable_if_t<
            !traits::is_char_variant<ElementType>::value,
            void>
    {
        istream >> std::ws >> element;
    }

    template<typename ElementType>
    static auto parse_element(StreamType& istream, ElementType& element
        ) noexcept -> std::enable_if_t<
            traits::is_char_variant<ElementType>::value,
            void>
    {
        if (static_cast<repr_type>(
                istream.iword(strings::detail::get_manip_i())) == repr_type::quoted)
            istream >> std::ws >> strings::quoted(element);
        else
            istream >> std::ws >> strings::literal(element);
    }

    template<typename CharType>
    static void parse_element(StreamType& istream,
                              std::basic_string<CharType>& element)
    {
        if (static_cast<repr_type>(
                istream.iword(strings::detail::get_manip_i())) == repr_type::quoted)
            istream >> std::ws >> strings::quoted(element);
        else
            istream >> std::ws >> strings::literal(element);
    }

    // TBD std::cin >> charT[n] works, but with stack smashing on buffer overrun,
    //   std::cin >> charT* works, but with segfault on nullptr or buffer overrun -
    //   should this overload and one for charT* be allowed? If so, support for
    //   charT[n] and char* should be moved into quoted()/literal(), to make for
    //   consistent formatting inside and outside supported containers.
    template <typename CharType, std::size_t ArraySize>
    static auto parse_element(
        StreamType& istream, CharType (&element)[ArraySize]
        ) noexcept -> std::enable_if_t<
            traits::is_char_variant<CharType>::value,
            void>
    {
        std::basic_string<CharType> s;
        if (static_cast<repr_type>(
                istream.iword(strings::detail::get_manip_i())) == repr_type::quoted)
            istream >> std::ws >> strings::quoted(s);
        else
            istream >> std::ws >> strings::literal(s);
        if (s.size() < ArraySize)
        {
            auto it {std::copy(s.begin(), s.end(), std::begin(element))};
            std::fill(it, std::end(element), CharType('\0'));
        }
        else
        {
            istream.setstate(std::ios_base::failbit);
        }
    }

    // !!! should not stream into a string_view, as it could be equivalent to
    //   a CharType* of unknown allocation

    static void parse_separator(StreamType& istream) noexcept
    {
        extract_token(istream, decorators.separator);
    }

    static void parse_suffix(StreamType& istream) noexcept
    {
        extract_token(istream, decorators.suffix);
    }
};

// TBD is_move_assignable may be redundant here if is_parseable_as_container tests most STL
//   containers for is_move_constructible
// std::vector, std::deque, std::list
template<typename ContainerType, typename ElementType>
static auto emplace_element(
    ContainerType& container, const ElementType& element) noexcept -> std::enable_if_t<
        container_stream_io::traits::has_emplace_back<ContainerType>::value &&
        std::is_move_assignable<ElementType>::value, void>
{
    container.emplace_back(element);
}

// need this overload due to const keys making set/map pairs non-move-assignable
// std::(unordered_)set (redundant keys ignored)
// std::(unordered_)map (redundant keys will have value of first appearance in serialization)
// std::(unordered_)multiset, std::(unordered_)multimap
template <typename ContainerType, typename KeyType, typename ValueType>
static auto emplace_element(
    ContainerType& container,
    const std::pair<const KeyType, ValueType>& element) noexcept -> std::enable_if_t<
        container_stream_io::traits::has_iterless_emplace<ContainerType>::value, void>
{
    container.emplace(element.first, element.second);
}

// !!? now this is the intended generic
template <typename ContainerType, typename ElementType>
static auto emplace_element(
    ContainerType& container, const ElementType& element) noexcept -> std::enable_if_t<
        container_stream_io::traits::has_iterless_emplace<ContainerType>::value &&
        !container_stream_io::traits::has_emplace_back<ContainerType>::value &&
        std::is_move_assignable<ElementType>::value, void>
{
    container.emplace(element);
}

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
    static void parse(
        StreamType& istream, TupleType& tuple, const FormatterType& formatter)
    {
        if (istream.good())
            formatter.parse_element(istream, std::get<Index>(tuple));
        if (istream.good())
            formatter.parse_separator(istream);
        if (istream.good())
            tuple_handler<TupleType, Index + 1, Last>::parse(istream, tuple, formatter);
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
    static void parse(
        StreamType& istream, TupleType& tuple, const FormatterType& formatter) noexcept
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

// std::pairs, including set/map members
template <typename FirstType, typename SecondType,
          typename StreamType, typename FormatterType>
static StreamType& from_stream(
    StreamType& istream, std::pair<FirstType, SecondType>& container,
    const FormatterType& formatter)
{
    formatter.parse_prefix(istream);
    if (!istream.good())
        return istream;

    // pairs are commonly encounted as key-value members of (unordered_)(multi)maps,
    //   in which case keys are const regardless of key type named in map instantiation
    using BaseFirstType = typename std::remove_const<FirstType>::type;
    BaseFirstType first;
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

    safer_assign(first, const_cast<BaseFirstType&>(container.first));
    safer_assign(second, container.second);

    return istream;
}

// std::forward_list requires unique overload due to only having emplace_after,
//   and no easy way to get iterator to last element (end(), but no --it,)
//   plus its iterators not being affeced by new emplacements
template <typename StreamType, typename ElementType, typename FormatterType>
static StreamType& from_stream(
    StreamType& istream, std::forward_list<ElementType>& container,
    const FormatterType& formatter)
{
    formatter.parse_prefix(istream);
    if (!istream.good())
        return istream;

    std::forward_list<ElementType> new_container;
    ElementType temp_elem;

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

    auto nc_it { new_container.before_begin() };
    formatter.parse_element(istream, temp_elem);
    if (!istream.good())
        return istream;
    new_container.emplace_after(nc_it, temp_elem);
    ++nc_it;

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
        new_container.emplace_after(nc_it, temp_elem);
        ++nc_it;
    }

    if (!istream.fail() && !istream.bad())
        container = std::move(new_container);
    return istream;
}

// TBD use of clear could be avoided with container = ContainerType{}
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
    emplace_element(new_container, temp_elem);

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
        emplace_element(new_container, temp_elem);
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

    using repr_type = strings::detail::repr_type;

    static void print_prefix(StreamType& ostream) noexcept
    {
        ostream << decorators.prefix;
    }

    template <typename ElementType>
    static auto print_element(StreamType& ostream, const ElementType& element
        ) noexcept -> std::enable_if_t<
            !traits::is_char_variant<ElementType>::value &&
            !traits::is_string_variant<ElementType>::value,
            void>
    {
        ostream << element;
    }

    template<typename ElementType>
    static auto print_element(StreamType& ostream, const ElementType element
        ) noexcept -> std::enable_if_t<
            traits::is_char_variant<ElementType>::value ||
            traits::is_string_variant<ElementType>::value,
            void>
    {
        if (static_cast<repr_type>(
                ostream.iword(strings::detail::get_manip_i())) == repr_type::quoted)
            ostream << strings::quoted(element);
        else
            ostream << strings::literal(element);
    }

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
    static void print(
        StreamType& ostream, const TupleType& tuple, const FormatterType& formatter)
    {
        formatter.print_element(ostream, std::get<Index>(tuple));
        formatter.print_separator(ostream);
        tuple_handler<TupleType, Index + 1, Last>::print(ostream, tuple, formatter);
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
    static void print(
        StreamType& ostream, const TupleType& tuple, const FormatterType& formatter) noexcept
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
    StreamType& ostream, const std::tuple<TupleArgs...>& tuple,
    const FormatterType& formatter)
{
    using TupleType = std::decay_t<decltype(tuple)>;

    formatter.print_prefix(ostream);
    container_stream_io::output::tuple_handler<
        TupleType, 0, sizeof...(TupleArgs) - 1>::print(ostream, tuple, formatter);
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
