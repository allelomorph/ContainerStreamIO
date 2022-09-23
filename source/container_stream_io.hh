#pragma once

#include <algorithm>  // copy
#include <cstddef>
#include <iostream>
#include <set>
#include <string>
#include <tuple>
#include <utility>

// Temporary includes for from_stream overloads
//#include <array>
#include <vector>
//#include <deque>
//#include <forward_list>
//#include <list>
//#include <map>
//#include <unordered_set>
//#include <unordered_map>
//#include <stack>
//#include <queue>

#include <iomanip>  // quoted
#include <iterator>  // begin, end

namespace std {

#if (__cplusplus < 201703L)

#  ifndef __cpp_lib_void_t

#define __cpp_lib_void_t 201411L
template<typename...>
using void_t = void;

#  endif

#endif

#if (__cplusplus < 201402L)

// No feature test macro found for decay_t.
template<class T>
using decay_t = typename decay<T>::type;

// No feature test macro found for enable_if_t.
template< bool B, class T = void >
using enable_if_t = typename enable_if<B,T>::type;

// Use of variable template is_printable_as_container_v below ellided by
//   testing for feature test macro __cpp_variable_templates.

// Use of generic lambda in to_stream(not tuple or pair) below ellided by
//   testing for feature test macro __cpp_generic_lambdas.

#endif

}  // namespace std


namespace container_stream_io {

namespace traits {

/**
 * @brief Base case for the testing of STL compatible container types.
 */
template <typename Type, typename = void>
struct is_parseable_as_container : public std::false_type
{};

/**
 * @brief Specialization to ensure that Standard Library compatible containers that have
 * `begin()`, `end()`, and `empty()` member functions are treated as parseable containers.
 */
template <typename Type>
struct is_parseable_as_container<
    Type, std::void_t<
              typename Type::iterator, decltype(std::declval<Type&>().begin()),
              decltype(std::declval<Type&>().end()), decltype(std::declval<Type&>().empty())>>
    : public std::true_type
{};

/**
 * @brief Specialization to treat std::pair<...> as a parseable container type.
 */
/*
template <typename FirstType, typename SecondType>
struct is_parseable_as_container<std::pair<FirstType, SecondType>> : public std::true_type
{};
*/

/**
 * @brief Specialization to treat std::tuple<...> as a parseable container type.
 */
/*
template <typename... Args>
struct is_parseable_as_container<std::tuple<Args...>> : public std::true_type
{};
*/

/**
 * @brief Specialization to treat arrays as parseable container types.
 */
/*
template <typename ArrayType, std::size_t ArraySize>
struct is_parseable_as_container<ArrayType[ArraySize]> : public std::true_type
{};
*/

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

/**
 * @brief Helper variable template.
 */
#ifdef __cpp_variable_templates
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

/**
 * @brief Helper variable template.
 */
#ifdef __cpp_variable_templates
template <typename Type>
constexpr bool is_printable_as_container_v = is_printable_as_container<Type>::value;
#endif

} // namespace traits

namespace decorator {

/**
 * @brief Struct to neatly wrap up all the additional characters we'll need in order to
 * print out the containers.
 */
template <typename CharacterType>
struct wrapper
{
    using type = CharacterType;

    const type* prefix;
    const type* delimiter;
    const type* whitespace;
    const type* suffix;
};

/**
 * @brief Base definition for the delimiters. This probably won't ever get invoked.
 */
template <typename /*ContainerType*/, typename CharacterType>
struct delimiters
{
    using type = wrapper<CharacterType>;

    static const type values;
};

/**
 * @brief Default narrow character specialization for any container type that isn't even
 * more specialized.
 */
template <typename ContainerType>
struct delimiters<ContainerType, char>
{
    using type = wrapper<char>;

    static constexpr type values = { "[", ",", " ", "]" };
};

/**
 * @brief Default wide character specialization for any container type that isn't even
 * more specialized.
 */
template <typename ContainerType>
struct delimiters<ContainerType, wchar_t>
{
    using type = wrapper<wchar_t>;

    static constexpr type values = { L"[", L",", L" ", L"]" };
};

/**
 * @brief Narrow character specialization for std::set<...> instances.
 */
template <typename DataType, typename ComparatorType, typename AllocatorType>
struct delimiters<std::set<DataType, ComparatorType, AllocatorType>, char>
{
    static constexpr wrapper<char> values = { "{", ",", " ", "}" };
};

/**
 * @brief Wide character specialization for std::set<...> instances.
 */
template <typename DataType, typename ComparatorType, typename AllocatorType>
struct delimiters<std::set<DataType, ComparatorType, AllocatorType>, wchar_t>
{
    static constexpr wrapper<wchar_t> values = { L"{", L",", L" ", L"}" };
};

/**
 * @brief Narrow character specialization for std::multiset<...> instances.
 */
template <typename DataType, typename ComparatorType, typename AllocatorType>
struct delimiters<std::multiset<DataType, ComparatorType, AllocatorType>, char>
{
    static constexpr wrapper<char> values = { "{", ",", " ", "}" };
};

/**
 * @brief Wide character specialization for std::multiset<...> instances.
 */
template <typename DataType, typename ComparatorType, typename AllocatorType>
struct delimiters<std::multiset<DataType, ComparatorType, AllocatorType>, wchar_t>
{
    static constexpr wrapper<wchar_t> values = { L"{", L",", L" ", L"}" };
};

/**
 * @brief Narrow character specialization for std::pair<...> instances.
 */
template <typename FirstType, typename SecondType>
struct delimiters<std::pair<FirstType, SecondType>, char>
{
    static constexpr wrapper<char> values = { "(", ",", " ", ")" };
};

/**
 * @brief Wide character specialization for std::pair<...> instances.
 */
template <typename FirstType, typename SecondType>
struct delimiters<std::pair<FirstType, SecondType>, wchar_t>
{
    static constexpr wrapper<wchar_t> values = { L"(", L",", L" ", L")" };
};

/**
 * @brief Narrow character specialization for std::tuple<...> instances.
 */
template <typename... DataType>
struct delimiters<std::tuple<DataType...>, char>
{
    static constexpr wrapper<char> values = { "<", ",", " ", ">" };
};

/**
 * @brief Wide character specialization for std::tuple<...> instances.
 */
template <typename... DataType>
struct delimiters<std::tuple<DataType...>, wchar_t>
{
    static constexpr wrapper<wchar_t> values = { L"<", L",", L" ", L">" };
};

} // namespace decorator

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

    static void print_prefix(StreamType& stream) noexcept
    {
        stream << decorators.prefix;
    }

    template <typename ElementType>
    static void print_element(StreamType& stream, const ElementType& element) noexcept
    {
        stream << element;
    }

    template <std::size_t ArraySize>
    static void print_element(StreamType& stream, const char element[ArraySize]) noexcept
    {
        stream << std::quoted(element);
    }

    template <std::size_t ArraySize>
    static void print_element(StreamType& stream, const wchar_t element[ArraySize]) noexcept
    {
        stream << std::quoted(element);
    }

    template<typename CharacterType>
    static void print_element(StreamType& stream,
                              const std::basic_string<CharacterType>& element) noexcept
    {
        stream << std::quoted(element);
    }

#if (__cplusplus >= 201703L)
    template<typename CharacterType>
    static void print_element(StreamType& stream,
                              const std::basic_string_view<CharacterType>& element) noexcept
    {
        stream << std::quoted(element);
    }

#endif
    static void print_delimiter(StreamType& stream) noexcept
    {
        stream << decorators.delimiter << decorators.whitespace;
    }

    static void print_suffix(StreamType& stream) noexcept
    {
        stream << decorators.suffix;
    }
};

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

/**
 * @brief Recursive tuple handler struct meant to unpack and print std::tuple<...> elements.
 */
template <typename TupleType, std::size_t Index, std::size_t Last>
struct tuple_handler
{
    template <typename StreamType, typename FormatterType>
    static void
    print(StreamType& stream, const TupleType& container, const FormatterType& formatter)
    {
        stream << std::get<Index>(container);
        formatter.print_delimiter(stream);
        tuple_handler<TupleType, Index + 1, Last>::print(stream, container, formatter);
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
        StreamType& /*stream*/, const TupleType& /*tuple*/,
        const FormatterType& /*formatter*/) noexcept
    {}
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
    print(StreamType& stream, const TupleType& tuple, const FormatterType& formatter) noexcept
    {
        formatter.print_element(stream, std::get<Index>(tuple));
    }
};

/**
 * @brief Overload to deal with std::tuple<...> objects.
 */
template <typename StreamType, typename FormatterType, typename... TupleArgs>
static StreamType& to_stream(
    StreamType& stream, const std::tuple<TupleArgs...>& container,
    const FormatterType& formatter)
{
    using ContainerType = std::decay_t<decltype(container)>;

    formatter.print_prefix(stream);
    tuple_handler<ContainerType, 0, sizeof...(TupleArgs) - 1>::print(stream, container, formatter);
    formatter.print_suffix(stream);

    return stream;
}

/**
 * @brief Overload to handle std::pair<...> objects.
 */
template <typename FirstType, typename SecondType, typename StreamType, typename FormatterType>
static StreamType& to_stream(
    StreamType& stream, const std::pair<FirstType, SecondType>& container,
    const FormatterType& formatter)
{
    formatter.print_prefix(stream);
    formatter.print_element(stream, container.first);
    formatter.print_delimiter(stream);
    formatter.print_element(stream, container.second);
    formatter.print_suffix(stream);

    return stream;
}

/**
 * @brief Overload to handle containers that support the notion of "emptiness,"
 * and forward-iterability.
 */
template <typename ContainerType, typename StreamType, typename FormatterType>
static StreamType& to_stream(
    StreamType& stream, const ContainerType& container,
    const FormatterType& formatter)
{
    formatter.print_prefix(stream);

    if (is_empty(container)) {
        formatter.print_suffix(stream);

        return stream;
    }

    auto begin = std::begin(container);
    formatter.print_element(stream, *begin);

    std::advance(begin, 1);

    std::for_each(begin, std::end(container),
#ifdef __cpp_generic_lambdas
                  [&stream, &formatter](const auto& element) {
#else
                  [&stream, &formatter](const decltype(*begin)& element) {
#endif
        formatter.print_delimiter(stream);
        formatter.print_element(stream, element);
    });

    formatter.print_suffix(stream);

    return stream;
}

} // namespace output

namespace input {

// Will first be adding specific overloads of from_stream() for each container
//   supported, which will result initially in much redundant code. Then will
//   try to mimic Severeijns' metaprogramming generalizations to consolidate
//   those overloads around the different container element insertion methods:
//
// pair:
//   p.first = elem1;
//   p.second = elem2;
//     or
//   std::make_pair(elem1, elem2);
//
// tuple:
//   (not sure on this one yet - use recursive template like output::tuple_handler,
//     with variadic function that returns std::make_tuple(args)?)
//
// array:
//   a[i] = elem;
//
// forward_list:
//   fl.emplace_after(fl.end(), elem);
//
// vector:
// deque:
// list:
//   (in commmon: have emplace_back)
//   v/d/l.emplace_back(elem);
//
// set:
// multiset:
// unordered_set:
// unordered_multiset:
//   (in common: have key_type, but no operator[])
//   s/ms/us/ums.emplace(elem); (in non-multisets, redundant keys in serialization are ignored)
//
// map:
// multimap:
// unordered_map:
// unordered_multimap:
//   (in common: have key_type and operator[])
//   parse pair, then
//   m/mm/um/umm.emplace(elem1, elem2); (in non-multimaps, redundant keys in
//       serialization will have first value)
//     or
//   m/mm/um/umm.emplace(std::make_pair(elem1, elem2)); (same behavior for emplace(elem1, elem2))
//     or
//   m/mm/um/umm.emplace(parsed pair); (same behavior for emplace(elem1, elem2))
//     or
//   m/mm/um/umm[pair.first] = pair.second; (in non-multimaps, redundant keys in
//       serialization will have last value)
//
//   (maybe set failbit if redundant keys deserialized for non-multimaps?)

template <typename CharacterType>
static void extract_token(
    std::basic_istream<CharacterType>& istream, const CharacterType *token) {
    if (token == nullptr) {
        istream.setstate(std::ios_base::failbit);
        return;
    }
    auto token_s {
#if (__cplusplus >= 201703L)
        std::string_view{token}
#else
        std::string{token}
#endif
    };
    // no check for size of 0 needed, as then begin() == end()
    istream >> std::ws;
    auto it_1 {token_s.begin()};
    while (!istream.eof() && it_1 != token_s.end() && istream.peek() == *it_1) {
        istream.get();
        //std::cout << "\textract_token: get(): '" << static_cast<CharacterType>(istream.get()) <<  "'\n";
        ++it_1;
    }
    if (it_1 != token_s.end()) {
        // only partial delim match, return chars to stream
        for (auto it_2 {token_s.begin()}; it_2 != it_1; ++it_2) {
            // std::cout << "\textract_token: putback(): '" << *it_2 <<  "'\n";
            istream.putback(*it_2);
        }
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

    template <std::size_t ArraySize>
    static void parse_element(StreamType& istream, char (&element)[ArraySize]) noexcept
    {
        // can only use quoted with operator>> when target is string, not C-array
        std::string s;
        istream >> std::ws >> std::quoted(s);
        if (s.size() != ArraySize) {
            istream.setstate(std::ios_base::failbit);
            return;
        }
        std::copy(s.begin(), s.end(), std::begin(element));
    }

    template <std::size_t ArraySize>
    static void parse_element(StreamType& istream, wchar_t (&element)[ArraySize]) noexcept
    {
        // can only use quoted with operator>> when target is string, not C-array
        std::wstring s;
        istream >> std::ws >> std::quoted(s);
        if (s.size() != ArraySize) {
            istream.setstate(std::ios_base::failbit);
            return;
        }
        std::copy(s.begin(), s.end(), std::begin(element));
    }

    template<typename CharacterType>
    static void parse_element(StreamType& istream,
                              std::basic_string<CharacterType>& element) noexcept
    {
        istream >> std::ws >> std::quoted(element);
    }

#if (__cplusplus >= 201703L)
    template<typename CharacterType>
    static void parse_element(StreamType& istream,
                              std::basic_string_view<CharacterType>& element) noexcept
    {
        // can only use quoted with operator>> when target is string, not string_view
        std::basic_string<CharacterType> s;
        istream >> std::ws >> std::quoted(s);
        element = std::basic_string_view<CharacterType> {s.c_str()};
    }

#endif

    // TBD: try to consolidate all the insert_element overloads with metaprogramming
    /*
    static void insert_element(std::forward_list<ElementType>& container, const ElementType& element) noexcept
    {
        container.emplace_after(container.end(), element);
    }
    */
    template <typename ElementType>
    static void insert_element(std::vector<ElementType>& container, const ElementType& element) noexcept
    {
        container.emplace_back(element);
    }
/*
    template <typename ElementType>
    static void insert_element(std::deque<ElementType>& container, const ElementType& element) noexcept
    {
        container.emplace_back(element);
    }

    template <typename ElementType>
    static void insert_element(std::list<ElementType>& container, const ElementType& element) noexcept
    {
        container.emplace_back(element);
    }
*/
/*
    static void insert_element(std::set<ElementType>& container, const ElementType& element) noexcept
    {
        container.emplace(element);
    }

    static void insert_element(std::multiset<ElementType>& container, const ElementType& element) noexcept
    {
        container.emplace(element);
    }

    static void insert_element(std::unordered_set<ElementType>& container, const ElementType& element) noexcept
    {
        container.emplace(element);
    }
    static void insert_element(std::unordered_multiset<ElementType>& container, const ElementType& element) noexcept
    {
        container.emplace(element);
    }
*/
/*
    template<typename KeyType, typename ValueType>
    static void insert_element(std::map<KeyType, ValueType>& container,
                               const std::pair<KeyType, ValueType>& element) noexcept
    {
        container.emplace(element);
    }

    static void insert_element(std::multimap<KeyType, ValueType>& container,
                               const std::pair<KeyType, ValueType>& element) noexcept
    {
        container.emplace(element);
    }

    static void insert_element(std::unordered_map<KeyType, ValueType>& container,
                               const std::pair<KeyType, ValueType>& element) noexcept
    {
        container.emplace(element);
    }

    static void insert_element(std::unordered_multimap<KeyType, ValueType>& container,
                               const std::pair<KeyType, ValueType>& element) noexcept
    {
        container.emplace(element);
    }
*/
    static void parse_delimiter(StreamType& istream) noexcept
    {
        extract_token(istream, decorators.delimiter);
    }

    static void parse_suffix(StreamType& istream) noexcept
    {
        extract_token(istream, decorators.suffix);
    }
};


template <typename ContainerType, typename StreamType, typename FormatterType>
static StreamType& from_stream(
    StreamType& istream, ContainerType& container,
    const FormatterType& formatter)
{
    formatter.parse_prefix(istream);
    if (!istream.good()) {
        //std::cout << "parse_prefix fail\n";
        return istream;
    }

    ContainerType new_container;
    // value_type should work for most non-C_array, tuple, pair containers
    // (value_type for maps is pair)
    typename ContainerType::value_type temp_elem;

    // parse suffix to check for empty container
    formatter.parse_suffix(istream);
    if (!istream.bad()) {
        if (!istream.fail()) {
            //std::cout << "first parse_suffix succeed\n";
            container.clear();
            return istream;
        } else {
            //std::cout << "first parse_suffix fail\n";
            // will reattempt with element
            istream.clear();
        }
    }

    // parse first element
    formatter.parse_element(istream, temp_elem);
    if (!istream.good()) {
        //std::cout << "first parse_element fail\n";
        return istream;
    }
    formatter.insert_element(new_container, temp_elem);

    // parse remaining elements
    while (!istream.eof()) {
        // parse suffix first to detect end of serialization
        formatter.parse_suffix(istream);
        if (!istream.bad()) {
            if (!istream.fail()) {
                //std::cout << "loop parse_suffix succeed\n";
                break;
            } else {
                //std::cout << "loop parse_suffix fail\n";
                // will reattempt with delimiter
                istream.clear();
            }
        }

        formatter.parse_delimiter(istream);
        if (!istream.good()) {
            //std::cout << "parse_delimiter fail\n";
            return istream;
        }

        formatter.parse_element(istream, temp_elem);
        if (!istream.good()) {
            //std::cout << "loop parse_element fail\n";
            return istream;
        }
        formatter.insert_element(new_container, temp_elem);
    }

    if (!istream.fail() && !istream.bad())
        container = std::move(new_container);
    return istream;
}

} // namespace input

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
