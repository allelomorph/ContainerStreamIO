#pragma once

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <set>
#include <string>
#include <tuple>
#include <utility>

namespace container_printer
{
namespace traits
{
/**
 * @brief Base case for the testing of STL compatible container types.
 */
template <typename Type, typename = void> struct is_printable_as_container : public std::false_type
{
};

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
{
};

/**
 * @brief Specialization to treat std::pair<...> as a printable container type.
 */
template <typename FirstType, typename SecondType>
struct is_printable_as_container<std::pair<FirstType, SecondType>> : public std::true_type
{
};

/**
 * @brief Specialization to treat std::tuple<...> as a printable container type.
 */
template <typename... Args>
struct is_printable_as_container<std::tuple<Args...>> : public std::true_type
{
};

/**
 * @brief Specialization to treat arrays as printable container types.
 */
template <typename ArrayType, std::size_t ArraySize>
struct is_printable_as_container<ArrayType[ArraySize]> : public std::true_type
{
};

/**
 * @brief Narrow character array specialization meant to ensure that we print character arrays
 * as strings and not as delimiter containers of individual characters.
 */
template <std::size_t ArraySize>
struct is_printable_as_container<char[ArraySize]> : public std::false_type
{
};

/**
 * @brief Wide character array specialization meant to ensure that we print character arrays
 * as strings and not as delimiter containers of individual characters.
 */
template <std::size_t ArraySize>
struct is_printable_as_container<wchar_t[ArraySize]> : public std::false_type
{
};

/**
 * @brief String specialization meant to ensure that we treat strings as nothing more than
 * strings.
 */
template <typename CharacterType, typename CharacterTraitsType, typename AllocatorType>
struct is_printable_as_container<
    std::basic_string<CharacterType, CharacterTraitsType, AllocatorType>> : public std::false_type
{
};

/**
 * @brief Helper variable template.
 */
template <typename Type>
constexpr bool is_printable_as_container_v = is_printable_as_container<Type>::value;
} // namespace traits

namespace decorator
{
/**
 * @brief Struct to neatly wrap up all the additional characters we'll need in order to
 * print out the containers.
 */
template <typename CharacterType> struct wrapper
{
    using type = CharacterType;

    const type* prefix;
    const type* separator;
    const type* suffix;
};

/**
 * @brief Base definition for the delimiters. This probably won't ever get invoked.
 */
template <typename /*ContainerType*/, typename CharacterType> struct delimiters
{
    using type = wrapper<CharacterType>;

    static const type values;
};

/**
 * @brief Default narrow character specialization for any container type that isn't even
 * more specialized.
 */
template <typename ContainerType> struct delimiters<ContainerType, char>
{
    using type = wrapper<char>;

    static constexpr type values = { "[", ", ", "]" };
};

/**
 * @brief Default wide character specialization for any container type that isn't even
 * more specialized.
 */
template <typename ContainerType> struct delimiters<ContainerType, wchar_t>
{
    using type = wrapper<wchar_t>;

    static constexpr type values = { L"[", L", ", L"]" };
};

/**
 * @brief Narrow character specialization for std::set<...> instances.
 */
template <typename DataType, typename ComparatorType, typename AllocatorType>
struct delimiters<std::set<DataType, ComparatorType, AllocatorType>, char>
{
    static constexpr wrapper<char> values = { "{", ", ", "}" };
};

/**
 * @brief Wide character specialization for std::set<...> instances.
 */
template <typename DataType, typename ComparatorType, typename AllocatorType>
struct delimiters<std::set<DataType, ComparatorType, AllocatorType>, wchar_t>
{
    static constexpr wrapper<wchar_t> values = { L"{", L", ", L"}" };
};

/**
 * @brief Narrow character specialization for std::multiset<...> instances.
 */
template <typename DataType, typename ComparatorType, typename AllocatorType>
struct delimiters<std::multiset<DataType, ComparatorType, AllocatorType>, char>
{
    static constexpr wrapper<char> values = { "{", ", ", "}" };
};

/**
 * @brief Wide character specialization for std::multiset<...> instances.
 */
template <typename DataType, typename ComparatorType, typename AllocatorType>
struct delimiters<std::multiset<DataType, ComparatorType, AllocatorType>, wchar_t>
{
    static constexpr wrapper<wchar_t> values = { L"{", L", ", L"}" };
};

/**
 * @brief Narrow character specialization for std::pair<...> instances.
 */
template <typename FirstType, typename SecondType>
struct delimiters<std::pair<FirstType, SecondType>, char>
{
    static constexpr wrapper<char> values = { "(", ", ", ")" };
};

/**
 * @brief Wide character specialization for std::pair<...> instances.
 */
template <typename FirstType, typename SecondType>
struct delimiters<std::pair<FirstType, SecondType>, wchar_t>
{
    static constexpr wrapper<wchar_t> values = { L"(", L", ", L")" };
};

/**
 * @brief Narrow character specialization for std::tuple<...> instances.
 */
template <typename... DataType> struct delimiters<std::tuple<DataType...>, char>
{
    static constexpr wrapper<char> values = { "<", ", ", ">" };
};

/**
 * @brief Wide character specialization for std::tuple<...> instances.
 */
template <typename... DataType> struct delimiters<std::tuple<DataType...>, wchar_t>
{
    static constexpr wrapper<wchar_t> values = { L"<", L", ", L">" };
};
} // namespace decorator

/**
 * @brief Default container formatter that will be used to print prefix, element, separator, and
 * suffix strings to an output stream.
 */
template <typename ContainerType, typename StreamType> struct default_formatter
{
    static constexpr auto decorators = container_printer::decorator::delimiters<
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

    static void print_delimiter(StreamType& stream) noexcept
    {
        stream << decorators.separator;
    }

    static void print_suffix(StreamType& stream) noexcept
    {
        stream << decorators.suffix;
    }
};

/**
 * @brief Helper function to determine if a container is empty.
 */
template <typename ContainerType> bool is_empty(const ContainerType& container) noexcept
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
template <typename TupleType, std::size_t Index, std::size_t Last> struct tuple_handler
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
    {
    }
};

/**
 * @brief Specialization of tuple handler to deal with the last element in the std::tuple<...>
 * object.
 */
template <typename TupleType, std::size_t Index> struct tuple_handler<TupleType, Index, Index>
{
    template <typename StreamType, typename FormatterType>
    static void
    print(StreamType& stream, const TupleType& tuple, const FormatterType& /*formatter*/) noexcept
    {
        stream << std::get<Index>(tuple);
    }
};

/**
 * @brief Overload to deal with std::tuple<...> objects.
 */
template <typename StreamType, typename FormatterType, typename... TupleArgs>
static StreamType& to_stream(
    StreamType& stream, const std::tuple<TupleArgs...>& container, const FormatterType& formatter)
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
static StreamType&
to_stream(StreamType& stream, const ContainerType& container, const FormatterType& formatter)
{
    formatter.print_prefix(stream);

    if (is_empty(container)) {
        formatter.print_suffix(stream);

        return stream;
    }

    auto begin = std::begin(container);
    formatter.print_element(stream, *begin);

    std::advance(begin, 1);

    std::for_each(begin, std::end(container), [&stream, &formatter](const auto& element) {
        formatter.print_delimiter(stream);
        formatter.print_element(stream, element);
    });

    formatter.print_suffix(stream);

    return stream;
}
} // namespace container_printer

/**
 * @brief Overload of the stream output operator for compatible containers.
 */
template <typename ContainerType, typename StreamType>
auto operator<<(StreamType& stream, const ContainerType& container) -> std::enable_if_t<
    container_printer::traits::is_printable_as_container_v<ContainerType>, StreamType&>
{
    using formatter_type = container_printer::default_formatter<ContainerType, StreamType>;
    container_printer::to_stream(stream, container, formatter_type{});

    return stream;
}
