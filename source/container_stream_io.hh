#pragma once

#include <cstdint>      // (u)int_XX_t
#include <algorithm>    // copy find_if for_each (limits:numeric_limits)
#include <cstddef>      // size_t
#include <iostream>
#include <sstream>      // basic_ostringstream
#include <set>
#include <map>
#include <string>
#include <tuple>
#include <forward_list>
#include <utility>
#include <iomanip>      // setfill, setw
#include <iterator>     // begin, end
#include <type_traits>  // true_type, false_type

#if (__cplusplus < 201103L)
#error "container_stream_io only supports C++11 and above"
#endif  // pre-C++11

/**
 * @brief manually adding to std STL elements from later standards when needed
 */
namespace std {

#if (__cplusplus < 201402L)

// no feature test macro found for decay_t
template <class T>
using decay_t = typename decay<T>::type;

// no feature test macro found for enable_if_t
template <bool B, class T = void>
using enable_if_t = typename enable_if<B,T>::type;

// no feature test macro found for remove_const_t
template <class T>
using remove_const_t = typename remove_const<T>::type;

// Use of generic lambda in generic overload of to_stream ellided by
//   testing for feature test macro __cpp_generic_lambdas

#endif  // pre-C++14

#ifndef __cpp_lib_void_t  // from C++17

template <typename...>
using void_t = void;

#endif  // void_t not implemented

}  // namespace std

/**
 * @brief contains supporting logic for implementation of istream and ostream
 *   operators for compatible containers
 */
namespace container_stream_io {

/**
 * @brief contains type traits tests to use in template resolution
 */
namespace traits {

/**
 * @brief tests for character types
 */
template <typename Type>
struct is_char_type : public std::false_type
{};

template <>
struct is_char_type<char> : public std::true_type
{};

template <>
struct is_char_type<wchar_t> : public std::true_type
{};

#if (__cplusplus > 201703L)
template <>
struct is_char_type<char8_t> : public std::true_type
{};

#endif
template <>
struct is_char_type<char16_t> : public std::true_type
{};

template <>
struct is_char_type<char32_t> : public std::true_type
{};

/*
 * @brief tests for (const) character type pointers or C arrays
 */
template <typename Type>
struct is_c_string_type : public std::false_type
{};

template <typename CharType>
struct is_c_string_type<CharType*> :
    public std::integral_constant<bool,
                                  is_char_type<std::remove_const_t<CharType>>::value>
{};

template <typename CharType, std::size_t ArraySize>
struct is_c_string_type<CharType[ArraySize]> :
    public std::integral_constant<bool,
                                  is_char_type<std::remove_const_t<CharType>>::value>
{};

/**
 * @brief tests for STL string types
 */
template <typename Type>
struct is_stl_string_type : public std::false_type
{};

template <typename CharType>
struct is_stl_string_type<std::basic_string<CharType>> :
    public std::integral_constant<bool, is_char_type<CharType>::value>
{};


#if (__cplusplus >= 201703L)
template <typename CharType>
struct is_stl_string_type<std::basic_string_view<CharType>> :
    public std::integral_constant<bool, is_char_type<CharType>::value>
{};

#endif  // C++17 and above

/**
 * @brief tests for character type pointers and C arrays, plus STL string types
 */
template <typename StringType>
struct is_string_type :
    public std::integral_constant<bool,
                                  is_c_string_type<StringType>::value ||
                                  is_stl_string_type<StringType>::value>
{};

/**
 * @brief tests for member function emplace(const_iterator, args...), eg as
 *   found in std::vector, std::list, std::deque
 * @notes
 *   - detection idiom:                   https://stackoverflow.com/a/41936999
 *   - detection idiom mod for variadics: https://stackoverflow.com/a/35669421
 */
template <typename Type, typename = void>
struct has_emplace : public std::false_type
{};

template <typename Type>
struct has_emplace<
    Type, std::void_t<decltype(
    std::declval<Type>().emplace(std::declval<typename Type::const_iterator>()))>>
    : public std::true_type
{};

/**
 * @brief tests for member function emplace(args...) (no iterator required), eg
 *   as found in std::(unordered_)(multi)set, std::(unordered_)(multi)map
 */
template <typename Type, typename = void>
struct has_iterless_emplace : public std::false_type
{};

template <typename Type>
struct has_iterless_emplace<Type, std::void_t<decltype(std::declval<Type>().emplace())>>
    : public std::true_type
{};

/**
 * @brief tests for member function emplace_back(args...) (no iterator required),
 *   eg as found in std::vector, std::list, std::deque
 */
template <typename Type, typename = void>
struct has_emplace_back : public std::false_type
{};

template <typename Type>
struct has_emplace_back<Type, std::void_t<decltype(std::declval<Type>().emplace_back())>>
    : public std::true_type
{};

/**
 * @brief tests for member function emplace_after(const iterator, args...),
 *   eg as found in std::forward_list
 */
template <typename Type, typename = void>
struct has_emplace_after : public std::false_type
{};

template <typename Type>
struct has_emplace_after<
    Type, std::void_t<decltype(
    std::declval<Type>().emplace_after(std::declval<typename Type::const_iterator>()))>>
    : public std::true_type
{};

/**
 * @brief tests for presence of some emplacement member function that can be
 *   used during container extraction from istreams
 */
template <typename Type>
struct supports_element_emplacement : public std::integral_constant<
    bool,
    has_emplace<Type>::value || has_iterless_emplace<Type>::value ||
    has_emplace_back<Type>::value || has_emplace_after<Type>::value>
{};

/**
 * @brief tests for class compatibility with container istreaming
 * @notes overloads should behave as follows:
 *   - base case: all incompatible types excluded
 *   - default: intended for inclusion of most STL container types, includes
 *       classes with members value_type (which is a move-constructible type,)
 *       clear(), and a supported version of emplacement:
 *         std::vector, std::deque, std::forward_list, std::list,
 *         std::(unordered_)(multi)set, std::(unordered_)(multi)map
 *       but not:
 *         std::stack, std::queue, std::priority_queue (lacking clear())
 *         std::basic_string, std::basic_string_view (lacking emplacement)
 *         std::array (lacking both clear() and emplacement)
 *   - std::pair: exeception to default
 *   - std::tuple: exeception to default
 *   - std::array: exeception to default
 *   - C array of non-char type: exeception to default
 *   - C array of char type: explicitly excluded to differentiate from non-char arrays
 */
template <typename Type, typename = void>
struct is_parseable_as_container : public std::false_type
{};

template <typename Type>
struct is_parseable_as_container<
    Type, std::void_t<typename Type::value_type,
                      decltype(std::declval<Type>().clear())>>
    : public std::integral_constant<bool,
                                    supports_element_emplacement<Type>::value &&
                                    std::is_move_constructible<typename Type::value_type>::value>
{};

template <typename FirstType, typename SecondType>
struct is_parseable_as_container<std::pair<FirstType, SecondType>> : public std::true_type
{};

template <typename... Args>
struct is_parseable_as_container<std::tuple<Args...>> : public std::true_type
{};

template <typename ArrayType, std::size_t ArraySize>
struct is_parseable_as_container<std::array<ArrayType, ArraySize>> : public std::true_type
{};

template <typename ArrayType, std::size_t ArraySize>
struct is_parseable_as_container<ArrayType[ArraySize],
                                 std::enable_if_t<!is_char_type<ArrayType>::value, void>>
    : public std::true_type
{};

template <typename CharType, std::size_t ArraySize>
struct is_parseable_as_container<CharType[ArraySize],
                                 std::enable_if_t<is_char_type<CharType>::value, void>>
    : public std::false_type
{};

#ifdef __cpp_variable_templates  // C++14 and above
/**
 * @brief variable template for is_parseable_as_container
 */
template <typename Type>
constexpr bool is_parseable_as_container_v = is_parseable_as_container<Type>::value;

#endif
/**
 * @brief tests for class compatibility with container ostreaming
 * @notes overloads should behave as follows:
 *   - base case: all incompatible types excluded
 *   - default: intended for inclusion of most STL container types, includes
 *       classes that are "iterable," with members iterator, begin(), end(),
 *       and empty():
 *         std::array, std::vector, std::deque, std::forward_list, std::list,
 *         std::(unordered_)(multi)set, std::(unordered_)(multi)map
 *       but not:
 *         std::stack, std::queue, std::priority_queue (lacking iterator, begin(), end())
 *   - std::pair: exeception to default
 *   - std::tuple: exeception to default
 *   - C array of non-char type: exeception to default
 *   - C array of char type: explicitly excluded to differentiate from non-char arrays
 *   - std::basic_string: exclusion from default
 *   - std::basic_string_view: exclusion from default
 */
template <typename Type, typename = void>
struct is_printable_as_container : public std::false_type
{};

template <typename Type>
struct is_printable_as_container<
    Type, std::void_t<typename Type::iterator,
                      decltype(std::declval<Type&>().begin()),
                      decltype(std::declval<Type&>().end()),
                      decltype(std::declval<Type&>().empty())>>
    : public std::true_type
{};

template <typename FirstType, typename SecondType>
struct is_printable_as_container<std::pair<FirstType, SecondType>> : public std::true_type
{};

template <typename... Args>
struct is_printable_as_container<std::tuple<Args...>> : public std::true_type
{};

template <typename ArrayType, std::size_t ArraySize>
struct is_printable_as_container<ArrayType[ArraySize],
                                 std::enable_if_t<!is_char_type<ArrayType>::value, void>>
    : public std::true_type
{};

template <typename ArrayType, std::size_t ArraySize>
struct is_printable_as_container<ArrayType[ArraySize],
                                 std::enable_if_t<is_char_type<ArrayType>::value, void>>
    : public std::false_type
{};

template <typename CharType, typename TraitsType, typename AllocType>
struct is_printable_as_container<std::basic_string<CharType, TraitsType, AllocType>>
    : public std::false_type
{};

#if (__cplusplus >= 201703L)
template <typename CharType>
struct is_printable_as_container<std::basic_string_view<CharType>>
    : public std::false_type
{};
#endif

#ifdef __cpp_variable_templates  // C++14 and above
/**
 * @brief variable template for is_printable_as_container
 */
template <typename Type>
constexpr bool is_printable_as_container_v = is_printable_as_container<Type>::value;

#endif
/**
 * @brief helper function to determine if a container is empty
 */
template <typename ContainerType>
bool is_empty(const ContainerType& container) noexcept
{
    return container.empty();
}

/**
 * @brief helper function to test C arrays for emptiness
 */
template <typename ArrayType, std::size_t ArraySize>
constexpr bool is_empty(const ArrayType (&)[ArraySize]) noexcept
{
    return ArraySize == 0;
}

}  // namespace traits

/**
 * @brief contains resources for string encoding/decoding
 */
namespace strings {

/**
 * @brief contains implementation of macros which generate compile-time string
 *   or character literals templated by character type
 * @notes adapted from: https://stackoverflow.com/a/60770220
 */
namespace compile_time {

#if (__cplusplus >= 201703L)

/**
 * @brief C++17 implementation of CHAR_LITERAL
 */
template <typename CharType>
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

/**
 * @brief C++17 implementation of STRING_LITERAL
 */
template <typename CharType,
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

/**
 * @brief C++11 implementation of CHAR_LITERAL
 * @notes constepxr functions implicitly inline per C++11 standard:
 *   https://stackoverflow.com/a/14391320
 */
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

/**
 * @brief C++11 implementation of STRING_LITERAL
 * @notes partial function template specialization not allowed by standard, see:
 *   https://stackoverflow.com/a/21218271, so instead using overloads each
 *   templated by literal sizes and enabled by return char type
 */
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

#  define CHAR_LITERAL(CHAR_T, LITERAL) \
    char_literal<CHAR_T>( LITERAL, L ## LITERAL, u8 ## LITERAL, \
                          u ## LITERAL, U ## LITERAL )

#  define STRING_LITERAL(CHAR_T, LITERAL) \
    string_literal<CHAR_T>( LITERAL, L ## LITERAL, u8 ## LITERAL, \
                            u ## LITERAL, U ## LITERAL )

#else  // __cplusplus <= 201703L

#  define CHAR_LITERAL(CHAR_T, LITERAL) \
    char_literal<CHAR_T>( LITERAL, L ## LITERAL, u ## LITERAL, U ## LITERAL )

#  define STRING_LITERAL(CHAR_T, LITERAL) \
    string_literal<CHAR_T>( LITERAL, L ## LITERAL, u ## LITERAL, U ## LITERAL )

#endif

}  // namespace compile_time

/**
 * @brief implementation details for quoted/literal
 * @notes quoted/literal implementation (string_repr, operator<</>>(string_repr),
 *   quoted(), literal()) is based on GNU ISO C++14 std::quoted (as seen in
 *   headers `iomanip`, `bits/quoted_string.h`) and reworked here to add features
 *   and allow use in C++11
 */
namespace detail {

/**
 * @brief labels for string representation type flag values
 */
enum class repr_type { literal, quoted };

/**
 * @brief stream index getter for use with iword/pword to set literalrepr/quotedrepr
 */
static inline int get_manip_i()
{
    static int i {std::ios_base::xalloc()};
    return i;
}

/**
 * @brief string representation, contains data necessary to istream/ostream a
 *   a quoted/literal string encoding
 */
template <typename StringType, typename CharType>
struct string_repr
{
private:
    /**
     * @brief contains standard ascii escape sequences, mapped both by value and
     *   by escape symbol
     * @notes
     *   - by_value and by_symbol need to be wrapped in a struct to avoid
     *       variable template use (for C++11 compliance)
     *   - given Unicode code points below 0x7f map to 7-bit ASCII, escapes
     *       treated the same for all char types
     *   - legacy C trigraphs and "\?" -> '?' escaping ignored for now, see:
     *       https://en.cppreference.com/w/cpp/language/escape
     *       https://en.cppreference.com/w/c/language/operator_alternative
     *   - non-literal (std::map has non-trivial dtor) static members must be
     *       initialized outside class definition (per gcc)
     */
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

/**
 * linker needs declaration of static constexpr members outside class for
 *   standards below C++17, see:
 *   - https://en.cppreference.com/w/cpp/language/static
 *   - https://stackoverflow.com/a/28846608
 */
template <typename StringType, typename CharType>
constexpr typename string_repr<StringType, CharType>::ascii_escapes string_repr<StringType, CharType>::escapes;

#endif  // pre-C++17

template <typename StringType, typename CharType>
std::map<CharType, CharType> string_repr<StringType, CharType>::ascii_escapes::by_value
{
    {
        {'\a', 'a'}, {'\b', 'b'}, {'\f', 'f'}, {'\n', 'n'},
        {'\r', 'r'}, {'\t', 't'}, {'\v', 'v'}, {'\0', '0'}
    }
};

template <typename StringType, typename CharType>
std::map<CharType, CharType> string_repr<StringType, CharType>::ascii_escapes::by_symbol
{
    {
        {'a', '\a'}, {'b', '\b'}, {'f', '\f'}, {'n', '\n'},
        {'r', '\r'}, {'t', '\t'}, {'v', '\v'}, {'0', '\0'}
    }
};

/**
 * @brief helper to operator<<(string_repr), ostreams literal prefix
 */
template <typename StreamCharType, typename StringCharType>
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

/**
 * @brief helper to operator<<(string_repr), ostreams one character from a
 *   string representation
 */
template <typename StreamCharType, typename StringType, typename StringCharType>
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
/**
 * @brief ostream operator for string representations
 * @notes overloads as follows:
 *   - default: handles (const) basic_string& and (const) basic_string_view&
 *      represenations (potentially also (const) (CharT&)[] if not decayed?)
 *   - single-char: handles (const) CharT& representations
 *   - C string: handles (const) CharT* representations
 */
template<typename StreamCharType, typename StringType, typename StringCharType>
auto operator<<(
    std::basic_ostream<StreamCharType>& ostream,
    const string_repr<StringType&, StringCharType>& repr
    ) -> std::enable_if_t<
    traits::is_stl_string_type<std::remove_const_t<StringType>>::value,
    std::basic_ostream<StreamCharType>&>
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

template <typename StreamCharType, typename CharType, typename StringCharType>
auto operator<<(
    std::basic_ostream<StreamCharType>& ostream,
    const string_repr<CharType&, StringCharType>& repr
    ) -> std::enable_if_t<
    std::is_same<StringCharType, std::remove_const_t<CharType>>::value,
    std::basic_ostream<StreamCharType>&>
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

template <typename StreamCharType, typename CharType, typename StringCharType>
auto operator<<(
    std::basic_ostream<StreamCharType>& ostream,
    const string_repr<CharType*, StringCharType>& repr
    ) -> std::enable_if_t<
    std::is_same<StringCharType, std::remove_const_t<CharType>>::value,
    std::basic_ostream<StreamCharType>&>
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

/**
 * @brief helper to extract_string_repr, decodes/validates a literal prefix
 *   matching the target char type
 */
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

/**
 * @brief helper to extract_string_repr, decodes a hex escaped value and
 *   validates that it matches the width of the target char type
 */
template<typename StreamCharType, typename StringCharType>
static int64_t extract_fixed_width_hex_value(
    std::basic_istream<StreamCharType>& istream)
{
    static constexpr uint32_t hex_length { sizeof(StringCharType) * 2 };
    // strtol expects char* (not using wcstol due to variable size of wchar_t)
    char buff[hex_length + 1] {};
    char *p { buff };
    // malformed hex strings could have values larger than StreamCharType max,
    //   with unpredictable overflows, so we need to pre-screen one by one
    //   rather than just call `get(buff, hex_length + 1)`
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

/**
 * @brief helper to extract_string_repr, encapsulates main quoted representation
 *   decoding loop
 */
template<typename StreamCharType, typename StringCharType>
static void extract_quoted_repr(
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

/**
 * @brief helper to extract_string_repr, encapsulates main literal
 *   representation decoding loop
 */
template<typename StreamCharType, typename StringCharType>
static void extract_literal_repr(
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

/**
 * @brief helper to operator>>(string_repr), differentiates between quoted and
 *   literal decoding
 */
template<typename StreamCharType, typename StringCharType>
static void extract_string_repr(
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

/**
 * @brief istream operator for string representations
 * @notes overloads as follows:
 *   - CharT&
 *   - basic_string&
 */
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

/**
 * @brief iomanip to set encoding/decoding of strings/chars in containers to
 *   literal
 */
template<typename CharType, typename TraitsType>
std::basic_ios<CharType, TraitsType>& literalrepr(
    std::basic_ios<CharType, TraitsType>& stream)
{
    stream.iword(detail::get_manip_i()) =
        static_cast<int>(detail::repr_type::literal);
    return stream;
}

/**
 * @brief iomanip to set encoding/decoding of strings/chars in containers to
 *   quoted
 */
template<typename CharType, typename TraitsType>
std::basic_ios<CharType, TraitsType>& quotedrepr(
    std::basic_ios<CharType, TraitsType>& stream)
{
    stream.iword(detail::get_manip_i()) =
        static_cast<int>(detail::repr_type::quoted);
    return stream;
}

/**
 * @brief generates quoted string represenation intended for use with stream
 *   operators
 * @notes
 *   - overloads need to be supplied for each string type of param `string`,
 *       so that it can both be templated for any char type, and allow the
 *       deduction of the `delim` and `escape` types when they are not given as
 *       params
 *   - overloads should support the following string types: (const) CharT&,
 *       (const) CharT*, (const) basic_string&, (const) basic_string_view&
 */
template<typename CharType>
inline auto quoted(
    CharType& string,
    CharType delim = CharType('\''), CharType escape = CharType('\\')
    ) -> std::enable_if_t<
        traits::is_char_type<CharType>::value,
        detail::string_repr<CharType&, CharType>>
{
    return detail::string_repr<CharType&, CharType>(
            string, delim, escape, detail::repr_type::quoted);
}

template<typename CharType>
inline auto quoted(
    const CharType& string,
    CharType delim = CharType('\''), CharType escape = CharType('\\')
    ) -> std::enable_if_t<
        traits::is_char_type<CharType>::value,
        detail::string_repr<const CharType&, CharType>>
{
    return detail::string_repr<const CharType&, CharType>(
            string, delim, escape, detail::repr_type::quoted);
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

/**
 * @brief generates literal string represenation intended for use with stream
 *   operators
 * @notes
 *   - overloads need to be supplied for each string type of param `string`,
 *       so that it can both be templated for any char type, and allow the
 *       deduction of the `delim` and `escape` types when they are not given as
 *       params
 *   - overloads should support the following string types: (const) CharT&,
 *       (const) CharT*, (const) basic_string&, (const) basic_string_view&
 */
template<typename CharType>
inline auto literal(
    CharType& string,
    CharType delim = CharType('\''), CharType escape = CharType('\\')
    ) -> std::enable_if_t<
        traits::is_char_type<CharType>::value,
        detail::string_repr<CharType&, CharType>>
{
    return detail::string_repr<CharType&, CharType>(
            string, delim, escape, detail::repr_type::literal);
}

template<typename CharType>
inline auto literal(
    const CharType& string,
    CharType delim = CharType('\''), CharType escape = CharType('\\')
    ) -> std::enable_if_t<
        traits::is_char_type<CharType>::value,
        detail::string_repr<const CharType&, CharType>>
{
    return detail::string_repr<const CharType&, CharType>(
            string, delim, escape, detail::repr_type::literal);
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

/**
 * @brief contains structures which provide tokens used around and between
 *   elements in conatiner serializations
 */
namespace decorator {

/**
 * @brief wraps tokens used around and between elements in serialization of a
 *   given set of container types
 */
template <typename CharType>
struct delim_wrapper
{
    const CharType* prefix;
    const CharType* separator;
    const CharType* whitespace;
    const CharType* suffix;
};

using namespace strings::compile_time;  // char_literal, string_literal

// TBD consider making map/multimap curly braced, as they are essentially sets of pairs
/**
 * @brief wraps delim_wrapper, allowing templating by both char and container
 *   types
 * @notes overloads as follows:
 *   - default (eg std::array, C array, vector, list, forward_list, queue,
 *       unordered(multi)(map|set))
 *   - set/multiset
 *   - pair
 *   - tuple
 */
template <typename ContainerType,
          typename CharType>
struct delimiters
{
    static constexpr delim_wrapper<CharType> values {
        STRING_LITERAL(CharType, "["),
        STRING_LITERAL(CharType, ","),
        STRING_LITERAL(CharType, " "),
        STRING_LITERAL(CharType, "]") };
};

template <typename DataType, typename CompareType, typename AllocType,
          typename CharType>
struct delimiters<std::set<DataType, CompareType, AllocType>, CharType>
{
    static constexpr delim_wrapper<CharType> values {
        STRING_LITERAL(CharType, "{"),
        STRING_LITERAL(CharType, ","),
        STRING_LITERAL(CharType, " "),
        STRING_LITERAL(CharType, "}") };
};

template <typename DataType, typename CompareType, typename AllocType,
          typename CharType>
struct delimiters<std::multiset<DataType, CompareType, AllocType>, CharType>
{
    static constexpr delim_wrapper<CharType> values {
        STRING_LITERAL(CharType, "{"),
        STRING_LITERAL(CharType, ","),
        STRING_LITERAL(CharType, " "),
        STRING_LITERAL(CharType, "}") };
};

template <typename FirstType, typename SecondType,
          typename CharType>
struct delimiters<std::pair<FirstType, SecondType>, CharType>
{
    static constexpr delim_wrapper<CharType> values {
        STRING_LITERAL(CharType, "("),
        STRING_LITERAL(CharType, ","),
        STRING_LITERAL(CharType, " "),
        STRING_LITERAL(CharType, ")") };
};

template <typename... DataType,
          typename CharType>
struct delimiters<std::tuple<DataType...>, CharType>
{
    static constexpr delim_wrapper<CharType> values {
        STRING_LITERAL(CharType, "<"),
        STRING_LITERAL(CharType, ","),
        STRING_LITERAL(CharType, " "),
        STRING_LITERAL(CharType, ">") };
};

}  // namespace decorator

/**
 * @brief contains functions to govern input streaming/extraction of compatible
 *   containers
 */
namespace input {

/**
 * @brief default formatter for the parsing of decorators and elements in a
 *   container serialization
 */
template <typename ContainerType, typename StreamType>
struct default_formatter
{
    using repr_type = strings::detail::repr_type;
    using stream_char_type = typename StreamType::char_type;

    static constexpr auto decorators {
        decorator::delimiters<ContainerType, stream_char_type>::values };

    /**
     * @brief attempts stream extraction of an exact token
     */
    static void extract_token(StreamType& istream, const stream_char_type* token)
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

    /**
     * @brief extracts prefix decorator from stream
     */
    static void parse_prefix(StreamType& istream) noexcept
    {
        extract_token(istream, decorators.prefix);
    }

    /**
     * @brief extracts element from stream
     * @notes overloads as follows:
     *   - default
     *   - CharT&
     *   - (CharT&)[] (invoked in case of nested C arrays, eg CharT[][])
     *   - basic_string&
     */
    template<typename ElementType>
    static auto parse_element(StreamType& istream, ElementType& element
        ) noexcept -> std::enable_if_t<
            !traits::is_char_type<ElementType>::value,
            void>
    {
        istream >> std::ws >> element;
    }

    template<typename ElementType>
    static auto parse_element(StreamType& istream, ElementType& element
        ) noexcept -> std::enable_if_t<
            traits::is_char_type<ElementType>::value,
            void>
    {
        if (static_cast<repr_type>(
                istream.iword(strings::detail::get_manip_i())) == repr_type::quoted)
            istream >> std::ws >> strings::quoted(element);
        else
            istream >> std::ws >> strings::literal(element);
    }

    template <typename CharType, std::size_t ArraySize>
    static auto parse_element(
        StreamType& istream, CharType (&element)[ArraySize]
        ) noexcept -> std::enable_if_t<
            traits::is_char_type<CharType>::value,
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

    /**
     * @brief extracts separator decorator from stream
     */
    static void parse_separator(StreamType& istream) noexcept
    {
        extract_token(istream, decorators.separator);
    }

    /**
     * @brief extracts suffix decorator from stream
     */
    static void parse_suffix(StreamType& istream) noexcept
    {
        extract_token(istream, decorators.suffix);
    }
};

/**
 * @brief helper to array_from_stream and from_stream overloads, used to move
 *   elements which themselves may be nested containers with C arrays at some
 *   level of nesting
 */
template<typename ContainerType>
static auto c_array_compatible_move_assignment(ContainerType& source,
                                               ContainerType& target
    ) -> std::enable_if_t<
        std::is_move_assignable<ContainerType>::value,
        void>
{
    target = std::move(source);
}

// TBD can this be improved with std::make_move_iterator?
template<typename ElementType, std::size_t ArraySize>
static void c_array_compatible_move_assignment(ElementType (&source)[ArraySize],
                                               ElementType (&target)[ArraySize])
{
    auto t_end {std::end(target)};
    for (auto s_it {std::begin(source)}, t_it {std::begin(target)};
         t_it != t_end; ++s_it, ++t_it)
    {
        c_array_compatible_move_assignment(*s_it, *t_it);
    }
}

// TBD can the relevant from_stream overloads be combined instead with a SFINAE
//   struct is_array, while not letting CharT[] types decay to CharT*?
/**
 * @brief wraps logic for C array and std::array overloads of from_stream
 */
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
    if (istream.good())
        c_array_compatible_move_assignment(temp_container, container);
    return istream;
}

/**
 * @brief helper to from_stream(tuple), recursive struct meant to unpack and
 *   parse std::tuple elements
 * @notes overloads as follows:
 *   - default
 *   - last element in tuple
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
 * @brief helper to default from_stream overload, uses appropriate emplacement
 *   method based on container type
 * @notes overloads as follows:
 *   - emplace_back (preferred over other emplace methods)
 *   - no emplace_back, but emplace (no const iterator needed) available
 *   - emplace (no const iterator needed) available, elements are std::pair with
 *       const .first (used for std::(unordered_)(multi)(set|map), where const
 *       pair.first makes elements non-move-assignable)
 */
template<typename ContainerType, typename ElementType>
static auto emplace_element(ContainerType& container, const ElementType& element
    ) noexcept -> std::enable_if_t<
        traits::has_emplace_back<ContainerType>::value,
        void>
{
    container.emplace_back(element);
}

template <typename ContainerType, typename ElementType>
static auto emplace_element(ContainerType& container, const ElementType& element
    ) noexcept -> std::enable_if_t<
        traits::has_iterless_emplace<ContainerType>::value &&
        !traits::has_emplace_back<ContainerType>::value,
        void>
{
    container.emplace(element);
}

template <typename ContainerType, typename KeyType, typename ValueType>
static auto emplace_element(ContainerType& container,
                            const std::pair<const KeyType, ValueType>& element
    ) noexcept -> std::enable_if_t<
        traits::has_iterless_emplace<ContainerType>::value,
        void>
{
    container.emplace(element.first, element.second);
}

/**
 * @brief stream extraction of compatible container type
 * @notes overloads as follows:
 *   - C array
 *   - std::array
 *   - std::tuple<T...>
 *   - std::tuple<>
 *   - std::pair
 *   - std::forward_list: unique overload required due to forward_list not
 *       having emplace(_back) or an easy way to get an iterator to the last
 *       element (end(), but no --it)
 *   - default: intended for "iterable" STL containers (see
 *       traits::is_parseable_as_container)
 */
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

template <typename StreamType, typename FormatterType, typename... TupleArgs>
static StreamType& from_stream(
    StreamType& istream, std::tuple<TupleArgs...>& container,
    const FormatterType& formatter)
{
    using ContainerType = std::decay_t<decltype(container)>;

    ContainerType temp;
    formatter.parse_prefix(istream);
    tuple_handler<ContainerType, 0, sizeof...(TupleArgs) - 1
                  >::parse(istream, temp, formatter);
    formatter.parse_suffix(istream);
    // C arrays not allowed as STL container members due to non-move-
    //   constructiblity, so no need for c_array_compatible_move_assignment
    if (istream.good())
        container = std::move(temp);
    return istream;
}

template <typename StreamType, typename FormatterType>
static StreamType& from_stream(
    StreamType& istream, std::tuple<>& /*container*/,
    const FormatterType& formatter)
{
    // no contents to parse, only checks if prefix or suffix properly encoded
    formatter.parse_prefix(istream);
    formatter.parse_suffix(istream);
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

    // pairs are commonly encountered as elements of std::(unordered_)(multi)(sets|map)s,
    //   in which case keys are const regardless of key type passed to container template
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

    // C arrays not allowed as STL container members due to non-move-
    //   constructiblity, so no need for c_array_compatible_move_assignment
    const_cast<BaseFirstType&>(container.first) = std::move(first);
    container.second = std::move(second);

    return istream;
}

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
    // forward_list iterators are not affected by new emplacements, therefore
    //   nc_it can continue to be used as indicating position before last element
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

    // C arrays not allowed as STL container members due to non-move-
    //   constructiblity, so no need for c_array_compatible_move_assignment
    if (istream.good())
        container = std::move(new_container);
    return istream;
}

// TBD use of clear could be avoided with container = ContainerType{}
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

    // C arrays not allowed as STL container members due to non-move-
    //   constructiblity, so no need for c_array_compatible_move_assignment
    if (istream.good())
        container = std::move(new_container);
    return istream;
}

}  // namespace input

/**
 * @brief contains functions to govern output streaming/insertion of compatible
 *   containers
 */
namespace output {

/**
 * @brief default formatter for the printing of decorators and elements in a
 *   container serialization
 */
template <typename ContainerType, typename StreamType>
struct default_formatter
{
    static constexpr auto decorators {
        decorator::delimiters<ContainerType, typename StreamType::char_type>::values };

    using repr_type = strings::detail::repr_type;

    /**
     * @brief inserts prefix decorator in stream
     */
    static void print_prefix(StreamType& ostream) noexcept
    {
        ostream << decorators.prefix;
    }

    /**
     * @brief inserts element in stream
     * @notes overloads as follows:
     *   - default
     *   - char or string types (C or STL)
     */
    template <typename ElementType>
    static auto print_element(StreamType& ostream, const ElementType& element
        ) noexcept -> std::enable_if_t<
            !traits::is_char_type<ElementType>::value &&
            !traits::is_string_type<ElementType>::value,
            void>
    {
        ostream << element;
    }

    template<typename ElementType>
    static auto print_element(StreamType& ostream, const ElementType& element
        ) noexcept -> std::enable_if_t<
            traits::is_char_type<ElementType>::value ||
            traits::is_string_type<ElementType>::value,
            void>
    {
        if (static_cast<repr_type>(
                ostream.iword(strings::detail::get_manip_i())) == repr_type::quoted)
            ostream << strings::quoted(element);
        else
            ostream << strings::literal(element);
    }

    /**
     * @brief inserts separator and whitespace decorators in stream
     */
    static void print_separator(StreamType& ostream) noexcept
    {
        ostream << decorators.separator << decorators.whitespace;
    }

    /**
     * @brief inserts suffix decorator in stream
     */
    static void print_suffix(StreamType& ostream) noexcept
    {
        ostream << decorators.suffix;
    }
};

/**
 * @brief helper to to_stream(tuple), recursive struct meant to unpack and
 *   parse std::tuple elements
 * @notes overloads as follows:
 *   - default
 *   - last element in tuple
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
 * @brief stream insertion of compatible container type
 * @notes overloads as follows:
 *   - std::tuple<T...>
 *   - std::tuple<>
 *   - std::pair
 *   - default: intended for "iterable" STL containers (see
 *       traits::is_printable_as_container)
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

template <typename StreamType, typename FormatterType, typename... TupleArgs>
static StreamType& to_stream(
    StreamType& ostream, const std::tuple<>& /*tuple*/,
    const FormatterType& formatter)
{
    formatter.print_prefix(ostream);
    formatter.print_suffix(ostream);
    return ostream;
}

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

}  // namespace output

}  // namespace container_stream_io

/**
 * @brief istream operator overload for compatible containers
 */
template <typename ContainerType, typename StreamType>
auto operator>>(StreamType& istream, ContainerType& container
    ) -> std::enable_if_t<
    container_stream_io::traits::is_parseable_as_container<ContainerType>::value,
    StreamType&>
{
    using formatter_type =
        container_stream_io::input::default_formatter<ContainerType, StreamType>;
    container_stream_io::input::from_stream(istream, container, formatter_type{});

    return istream;
}

/**
 * @brief ostream operator overload for compatible containers
 */
template <typename ContainerType, typename StreamType>
auto operator<<(StreamType& ostream, const ContainerType& container
    ) -> std::enable_if_t<
    container_stream_io::traits::is_printable_as_container<ContainerType>::value,
    StreamType&>
{
    using formatter_type =
        container_stream_io::output::default_formatter<ContainerType, StreamType>;
    container_stream_io::output::to_stream(ostream, container, formatter_type{});

    return ostream;
}
