#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include "container_stream_io.hh"

#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <set>
#include <vector>

#if (__cplusplus < 201103L)
#error "csio_unit_tests.cpp only supports C++11 and above"
#endif  // pre-C++11

namespace
{

/**
 * @brief An RAII wrapper that will execute an action when the wrapper falls out of scope, or is
 * otherwise destroyed.
 */
template <typename LambdaType>
class scope_exit
{
public:
    scope_exit(LambdaType&& lambda) noexcept : m_lambda{ std::move(lambda) }
    {
        static_assert(
#if __cplusplus >= 201703L
            std::is_nothrow_invocable<LambdaType>::value,
#else
            noexcept(lambda),
#endif
            "Since the callable type is invoked from the destructor, exceptions are not allowed.");
    }

    ~scope_exit() noexcept
    {
        m_lambda();
    }

    scope_exit(const scope_exit&) = delete;
    scope_exit& operator=(const scope_exit&) = delete;

    scope_exit(scope_exit&&) = default;
    scope_exit& operator=(scope_exit&&) = default;

private:
    LambdaType m_lambda;
};

template <typename Type>
class vector_wrapper : public std::vector<Type>
{
};

/**
 * @brief Custom formatting struct.
 */
struct custom_formatter
{
    template <typename StreamType> void print_prefix(StreamType& stream) const noexcept
    {
        stream << L"$$ ";
    }

    template <typename StreamType, typename ElementType>
    void print_element(StreamType& stream, const ElementType& element) const noexcept
    {
        stream << element;
    }

    template <typename StreamType> void print_separator(StreamType& stream) const noexcept
    {
        stream << L" | ";
    }

    template <typename StreamType> void print_suffix(StreamType& stream) const noexcept
    {
        stream << L" $$";
    }
};

// C++ idiomatic comparison of C strings (can't overload operator== for fundamental types)
template <typename CharacterType, std::size_t ArraySize>
bool c_strings_match(const CharacterType* const& s1,
                     const CharacterType (&s2)[ArraySize])
{
#if __cplusplus >= 201703L
    return s1 == std::basic_string_view<CharacterType>(s2);
#else
    return s1 == std::basic_string<CharacterType>(s2);
#endif
}

} // namespace

TEST_CASE("Traits")
{
    SECTION("Detect std::vector<...> as being an iterable container type.")
    {
#if __cplusplus >= 201703L
        REQUIRE(container_stream_io::traits::is_printable_as_container_v<std::vector<int>> == true);
#else
        REQUIRE(container_stream_io::traits::is_printable_as_container<std::vector<int>>::value == true);
#endif
    }

    SECTION("Detect std::list<...> as being an iterable container type.")
    {
#if __cplusplus >= 201703L
        REQUIRE(container_stream_io::traits::is_printable_as_container_v<std::list<int>> == true);
#else
        REQUIRE(container_stream_io::traits::is_printable_as_container<std::list<int>>::value == true);
#endif
    }

    SECTION("Detect std::set<...> as being an iterable container type.")
    {
#if __cplusplus >= 201703L
        REQUIRE(container_stream_io::traits::is_printable_as_container_v<std::set<int>> == true);
#else
        REQUIRE(container_stream_io::traits::is_printable_as_container<std::set<int>>::value == true);
#endif
    }

    SECTION("Detect array as being an iterable container type.")
    {
#if __cplusplus >= 201703L
        REQUIRE(container_stream_io::traits::is_printable_as_container_v<int[10]> == true);
#else
        REQUIRE(container_stream_io::traits::is_printable_as_container<int[10]>::value == true);
#endif
    }

    SECTION("Detect std::string as a type that shouldn't be iterated over.")
    {
#if __cplusplus >= 201703L
        REQUIRE(container_stream_io::traits::is_printable_as_container_v<std::string> == false);
#else
        REQUIRE(container_stream_io::traits::is_printable_as_container<std::string>::value == false);
#endif
    }

    SECTION("Detect std::wstring as a type that shouldn't be iterated over.")
    {
#if __cplusplus >= 201703L
        REQUIRE(container_stream_io::traits::is_printable_as_container_v<std::wstring> == false);
#else
        REQUIRE(container_stream_io::traits::is_printable_as_container<std::wstring>::value == false);
#endif
    }

    SECTION("Detect inherited iterable container type.")
    {
#if __cplusplus >= 201703L
        REQUIRE(
            container_stream_io::traits::is_printable_as_container_v<vector_wrapper<int>> == true);
#else
        REQUIRE(
            container_stream_io::traits::is_printable_as_container<vector_wrapper<int>>::value == true);
#endif
    }
}

TEST_CASE("Delimiter Validation")
{
    SECTION("Verify narrow character delimiters for a non-specialized container type.")
    {
        constexpr auto delimiters = container_stream_io::decorator::delimiters<char[], char>::values;

        REQUIRE(c_strings_match(delimiters.prefix,     "[" ));
        REQUIRE(c_strings_match(delimiters.separator,  "," ));
        REQUIRE(c_strings_match(delimiters.whitespace, " " ));
        REQUIRE(c_strings_match(delimiters.suffix,     "]" ));
    }

    SECTION("Verify wide character delimiters for a non-specialized container type.")
    {
        constexpr auto delimiters =
            container_stream_io::decorator::delimiters<wchar_t[], wchar_t>::values;

        REQUIRE(c_strings_match(delimiters.prefix,     L"[" ));
        REQUIRE(c_strings_match(delimiters.separator,  L"," ));
        REQUIRE(c_strings_match(delimiters.whitespace, L" " ));
        REQUIRE(c_strings_match(delimiters.suffix,     L"]" ));
    }

    SECTION("Verify narrow character delimiters for a std::set<...>.")
    {
        constexpr auto delimiters =
            container_stream_io::decorator::delimiters<std::set<int>, char>::values;

        REQUIRE(c_strings_match(delimiters.prefix,     "{" ));
        REQUIRE(c_strings_match(delimiters.separator,  "," ));
        REQUIRE(c_strings_match(delimiters.whitespace, " " ));
        REQUIRE(c_strings_match(delimiters.suffix,     "}" ));
    }

    SECTION("Verify wide character delimiters for a std::set<...>.")
    {
        constexpr auto delimiters =
            container_stream_io::decorator::delimiters<std::set<int>, wchar_t>::values;

        REQUIRE(c_strings_match(delimiters.prefix,     L"{" ));
        REQUIRE(c_strings_match(delimiters.separator,  L"," ));
        REQUIRE(c_strings_match(delimiters.whitespace, L" " ));
        REQUIRE(c_strings_match(delimiters.suffix,     L"}" ));
    }

    SECTION("Verify narrow character delimiters for a std::pair<...>.")
    {
        constexpr auto delimiters =
            container_stream_io::decorator::delimiters<std::pair<int, int>, char>::values;

        REQUIRE(c_strings_match(delimiters.prefix,     "(" ));
        REQUIRE(c_strings_match(delimiters.separator,  "," ));
        REQUIRE(c_strings_match(delimiters.whitespace, " " ));
        REQUIRE(c_strings_match(delimiters.suffix,     ")" ));
    }

    SECTION("Verify wide character delimiters for a std::pair<...>.")
    {
        constexpr auto delimiters =
            container_stream_io::decorator::delimiters<std::pair<int, int>, wchar_t>::values;

        REQUIRE(c_strings_match(delimiters.prefix,     L"(" ));
        REQUIRE(c_strings_match(delimiters.separator,  L"," ));
        REQUIRE(c_strings_match(delimiters.whitespace, L" " ));
        REQUIRE(c_strings_match(delimiters.suffix,     L")" ));
    }

    SECTION("Verify narrow character delimiters for a std::tuple<...>.")
    {
        constexpr auto delimiters =
            container_stream_io::decorator::delimiters<std::tuple<int, int>, char>::values;

        REQUIRE(c_strings_match(delimiters.prefix,     "<" ));
        REQUIRE(c_strings_match(delimiters.separator,  "," ));
        REQUIRE(c_strings_match(delimiters.whitespace, " " ));
        REQUIRE(c_strings_match(delimiters.suffix,     ">" ));
    }

    SECTION("Verify wide character delimiters for a std::tuple<...>.")
    {
        constexpr auto delimiters =
            container_stream_io::decorator::delimiters<std::tuple<int, int>, wchar_t>::values;

        REQUIRE(c_strings_match(delimiters.prefix,     L"<" ));
        REQUIRE(c_strings_match(delimiters.separator,  L"," ));
        REQUIRE(c_strings_match(delimiters.whitespace, L" " ));
        REQUIRE(c_strings_match(delimiters.suffix,     L">" ));
    }
}

TEST_CASE("Printing of Raw Arrays")
{
    std::stringstream narrow_buffer;
    auto* const old_narrow_buffer { std::cout.rdbuf(narrow_buffer.rdbuf()) };
#if __cplusplus >= 201703L
    const scope_exit reset_narrow_buffer { [&]() noexcept { std::cout.rdbuf(old_narrow_buffer); } };
#else
    const auto narrow_lambda { [&]() noexcept { std::cout.rdbuf(old_narrow_buffer); } };
    const scope_exit<decltype(narrow_lambda)> reset_narrow_buffer { std::move(narrow_lambda) };
#endif

    std::wstringstream wide_buffer;
    auto* const old_wide_buffer { std::wcout.rdbuf(wide_buffer.rdbuf()) };
#if __cplusplus >= 201703L
    const scope_exit reset_wide_buffer { [&]() noexcept { std::wcout.rdbuf(old_wide_buffer); } };
#else
    const auto wide_lambda { [&]() noexcept { std::wcout.rdbuf(old_wide_buffer); } };
    const scope_exit<decltype(wide_lambda)> reset_wide_buffer { std::move(wide_lambda) };
#endif

    SECTION("Printing a narrow character array.")
    {
        const auto& array { "Hello" };
        std::cout << array << std::flush;

        REQUIRE(narrow_buffer.str() == "Hello");
    }

    SECTION("Printing a wide character array.")
    {
        const auto& array { L"Hello" };
        std::wcout << array << std::flush;

        REQUIRE(wide_buffer.str() == L"Hello");
    }

    SECTION("Printing an array of ints to a narrow stream.")
    {
        const int array[5] = { 1, 2, 3, 4, 5 };
        std::cout << array << std::flush;

        REQUIRE(narrow_buffer.str() == "[1, 2, 3, 4, 5]");
    }

    SECTION("Printing an array of ints to a wide stream.")
    {
        const int array[5] = { 1, 2, 3, 4, 5 };
        std::wcout << array << std::flush;

        REQUIRE(wide_buffer.str() == L"[1, 2, 3, 4, 5]");
    }
}


TEST_CASE("Printing of Standard Library Containers")
{
    std::stringstream narrow_buffer;
    auto* const old_narrow_buffer { std::cout.rdbuf(narrow_buffer.rdbuf()) };
#if __cplusplus >= 201703L
    const scope_exit reset_narrow_buffer { [&]() noexcept { std::cout.rdbuf(old_narrow_buffer); } };
#else
    const auto narrow_lambda { [&]() noexcept { std::cout.rdbuf(old_narrow_buffer); } };
    const scope_exit<decltype(narrow_lambda)> reset_narrow_buffer { std::move(narrow_lambda) };
#endif

    std::wstringstream wide_buffer;
    auto* const old_wide_buffer { std::wcout.rdbuf(wide_buffer.rdbuf()) };
#if __cplusplus >= 201703L
    const scope_exit reset_wide_buffer { [&]() noexcept { std::wcout.rdbuf(old_wide_buffer); } };
#else
    const auto wide_lambda { [&]() noexcept { std::wcout.rdbuf(old_wide_buffer); } };
    const scope_exit<decltype(wide_lambda)> reset_wide_buffer { std::move(wide_lambda) };
#endif

    SECTION("Printing a std::pair<...> to a narrow stream.")
    {
        const auto pair { std::make_pair(10, 100) };
        std::cout << pair << std::flush;

        REQUIRE(narrow_buffer.str() == "(10, 100)");
    }

    SECTION("Printing a std::pair<...> to a wide stream.")
    {
        const auto pair { std::make_pair(10, 100) };
        std::wcout << pair << std::flush;

        REQUIRE(wide_buffer.str() == L"(10, 100)");
    }

    SECTION("Printing an empty std::vector<...> to a wide stream.")
    {
        const std::vector<int> vector{};
        std::wcout << vector << std::flush;

        REQUIRE(wide_buffer.str() == L"[]");
    }

    SECTION("Printing a populated std::vector<...> to a narrow stream.")
    {
        const std::vector<int> vector { 1, 2, 3, 4 };
        std::cout << vector << std::flush;

        REQUIRE(narrow_buffer.str() == "[1, 2, 3, 4]");
    }

    SECTION("Printing a populated std::vector<...> to a wide stream.")
    {
        const std::vector<int> vector { 1, 2, 3, 4 };
        std::wcout << vector << std::flush;

        REQUIRE(wide_buffer.str() == L"[1, 2, 3, 4]");
    }

    SECTION("Printing an empty std::set<...> to a narrow stream.")
    {
        const std::set<int> set {};
        std::cout << set << std::flush;

        REQUIRE(narrow_buffer.str() == "{}");
    }

    SECTION("Printing an empty std::set<...> to a wide stream.")
    {
        const std::set<int> set {};
        std::wcout << set << std::flush;

        REQUIRE(wide_buffer.str() == L"{}");
    }

    SECTION("Printing a populated std::set<...> to a narrow stream.")
    {
        const std::set<int> set { 1, 2, 3, 4 };
        std::cout << set << std::flush;

        REQUIRE(narrow_buffer.str() == "{1, 2, 3, 4}");
    }

    SECTION("Printing a populated std::set<...> to a wide stream.")
    {
        const std::set<int> set { 1, 2, 3, 4 };
        std::wcout << set << std::flush;

        REQUIRE(wide_buffer.str() == L"{1, 2, 3, 4}");
    }

    SECTION("Printing a populated std::multiset<...> to a narrow stream.")
    {
        const std::multiset<int> multiset { 1, 2, 3, 4 };
        std::cout << multiset << std::flush;

        REQUIRE(narrow_buffer.str() == "{1, 2, 3, 4}");
    }

    SECTION("Printing a populated std::multiset<...> to a wide stream.")
    {
        const std::multiset<int> multiset { 1, 2, 3, 4 };
        std::wcout << multiset << std::flush;

        REQUIRE(wide_buffer.str() == L"{1, 2, 3, 4}");
    }

    SECTION("Printing an empty std::tuple<...> to a narrow stream.")
    {
        const auto tuple { std::make_tuple() };
        std::cout << tuple << std::flush;

        REQUIRE(narrow_buffer.str() == "<>");
    }

    SECTION("Printing an empty std::tuple<...> to a wide stream.")
    {
        const auto tuple { std::make_tuple() };
        std::wcout << tuple << std::flush;

        REQUIRE(wide_buffer.str() == L"<>");
    }

    SECTION("Printing a populated std::tuple<...> to a narrow stream.")
    {
        const auto tuple { std::make_tuple(1, 2, 3, 4, 5) };
        std::cout << tuple << std::flush;

        REQUIRE(narrow_buffer.str() == "<1, 2, 3, 4, 5>");
    }

    SECTION("Printing a populated std::tuple<...> to a wide stream.")
    {
        const auto tuple { std::make_tuple(1, 2, 3, 4, 5) };
        std::wcout << tuple << std::flush;

        REQUIRE(wide_buffer.str() == L"<1, 2, 3, 4, 5>");
    }
}

TEST_CASE("Printing of Nested Containers")
{
    std::stringstream narrow_buffer;
    auto* const old_narrow_buffer { std::cout.rdbuf(narrow_buffer.rdbuf()) };
#if __cplusplus >= 201703L
    const scope_exit reset_narrow_buffer { [&]() noexcept { std::cout.rdbuf(old_narrow_buffer); } };
#else
    const auto narrow_lambda { [&]() noexcept { std::cout.rdbuf(old_narrow_buffer); } };
    const scope_exit<decltype(narrow_lambda)> reset_narrow_buffer { std::move(narrow_lambda) };
#endif

    std::wstringstream wide_buffer;
    auto* const old_wide_buffer { std::wcout.rdbuf(wide_buffer.rdbuf()) };
#if __cplusplus >= 201703L
    const scope_exit reset_wide_buffer { [&]() noexcept { std::wcout.rdbuf(old_wide_buffer); } };
#else
    const auto wide_lambda { [&]() noexcept { std::wcout.rdbuf(old_wide_buffer); } };
    const scope_exit<decltype(wide_lambda)> reset_wide_buffer { std::move(wide_lambda) };
#endif

    SECTION("Printing a populated std::map<...> to a narrow stream.")
    {
        const auto map { std::map<int, std::string> {
                { 1, "Template" }, { 2, "Meta" }, { 3, "Programming" } } };

        std::cout << map << std::flush;

        REQUIRE(narrow_buffer.str() == "[(1, \"Template\"), (2, \"Meta\"), (3, \"Programming\")]");
    }

    SECTION("Printing a populated std::map<...> to a wide stream.")
    {
        const auto map { std::map<int, std::wstring>{
                { 1, L"Template" }, { 2, L"Meta" }, { 3, L"Programming" } } };

        std::wcout << map << std::flush;

        REQUIRE(wide_buffer.str() == L"[(1, L\"Template\"), (2, L\"Meta\"), (3, L\"Programming\")]");
    }

    SECTION("Printing a populated std::vector<std::tuple<...>> to a narrow stream.")
    {
        const auto vector { std::vector<std::tuple<int, double, std::string>>{
                std::make_tuple(1, 0.1, "Hello"), std::make_tuple(2, 0.2, "World") } };

        std::cout << vector << std::flush;

        REQUIRE(narrow_buffer.str() == "[<1, 0.1, \"Hello\">, <2, 0.2, \"World\">]");
    }

    SECTION("Printing a populated std::vector<std::tuple<...>> to a wide stream.")
    {
        const auto vector { std::vector<std::tuple<int, double, std::wstring>>{
                std::make_tuple(1, 0.1, L"Hello"), std::make_tuple(2, 0.2, L"World") } };

        std::wcout << vector << std::flush;

        REQUIRE(wide_buffer.str() == L"[<1, 0.1, L\"Hello\">, <2, 0.2, L\"World\">]");
    }

    SECTION("Printing a populated std::pair<int, std::vector<std::pair<std::string, std::string>>>")
    {
        const auto pair { std::make_pair(
                10, std::vector<std::pair<std::string, std::string>> {
                    std::make_pair("Why", "Not?"), std::make_pair("Someone", "Might!") }) };

        std::cout << pair << std::flush;

        REQUIRE(narrow_buffer.str() == "(10, [(\"Why\", \"Not?\"), (\"Someone\", \"Might!\")])");
    }
}

TEST_CASE("Printing with Custom Formatters")
{
    std::stringstream narrow_buffer;
    auto* const old_narrow_buffer { std::cout.rdbuf(narrow_buffer.rdbuf()) };
#if __cplusplus >= 201703L
    const scope_exit reset_narrow_buffer { [&]() noexcept { std::cout.rdbuf(old_narrow_buffer); } };
#else
    const auto narrow_lambda { [&]() noexcept { std::cout.rdbuf(old_narrow_buffer); } };
    const scope_exit<decltype(narrow_lambda)> reset_narrow_buffer { std::move(narrow_lambda) };
#endif

    std::wstringstream wide_buffer;
    auto* const old_wide_buffer { std::wcout.rdbuf(wide_buffer.rdbuf()) };
#if __cplusplus >= 201703L
    const scope_exit reset_wide_buffer { [&]() noexcept { std::wcout.rdbuf(old_wide_buffer); } };
#else
    const auto wide_lambda { [&]() noexcept { std::wcout.rdbuf(old_wide_buffer); } };
    const scope_exit<decltype(wide_lambda)> reset_wide_buffer { std::move(wide_lambda) };
#endif

    SECTION("Printing a populated std::vector<...> to a wide stream.")
    {
        const auto container { std::vector<int>{ 1, 2, 3, 4 } };
        container_stream_io::output::to_stream(std::wcout, container, custom_formatter{}) << std::flush;

        REQUIRE(wide_buffer.str() == L"$$ 1 | 2 | 3 | 4 $$");
    }

    SECTION("Printing a populated std::tuple<...> to a wide stream.")
    {
        const auto container { std::make_tuple(1, 2, 3, 4) };
        container_stream_io::output::to_stream(std::wcout, container, custom_formatter{}) << std::flush;

        REQUIRE(wide_buffer.str() == L"$$ 1 | 2 | 3 | 4 $$");
    }

    SECTION("Printing a populated std::pair<...> to a wide stream.")
    {
        const auto container { std::make_pair(1, 2) };
        container_stream_io::output::to_stream(std::wcout, container, custom_formatter{}) << std::flush;

        REQUIRE(wide_buffer.str() == L"$$ 1 | 2 $$");
    }
}
