#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>

#include "container_printer.h"

#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <set>
#include <vector>

namespace
{
/**
 * @brief An RAII wrapper that will execute an action when the wrapper falls out of scope, or is
 * otherwise destroyed.
 */
template <typename LambdaType> class scope_exit
{
  public:
    scope_exit(LambdaType&& lambda) noexcept : m_lambda{ std::move(lambda) }
    {
        static_assert(
            std::is_nothrow_invocable<LambdaType>::value,
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

template <typename Type> class vector_wrapper : public std::vector<Type>
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

    template <typename StreamType> void print_delimiter(StreamType& stream) const noexcept
    {
        stream << L" | ";
    }

    template <typename StreamType> void print_suffix(StreamType& stream) const noexcept
    {
        stream << L" $$";
    }
};
} // namespace

TEST_CASE("Traits")
{
    SECTION("Detect std::vector<...> as being an iterable container type.")
    {
        REQUIRE(container_printer::traits::is_printable_as_container_v<std::vector<int>> == true);
    }

    SECTION("Detect std::list<...> as being an iterable container type.")
    {
        REQUIRE(container_printer::traits::is_printable_as_container_v<std::list<int>> == true);
    }

    SECTION("Detect std::set<...> as being an iterable container type.")
    {
        REQUIRE(container_printer::traits::is_printable_as_container_v<std::set<int>> == true);
    }

    SECTION("Detect array as being an iterable container type.")
    {
        REQUIRE(container_printer::traits::is_printable_as_container_v<int[10]> == true);
    }

    SECTION("Detect std::string as a type that shouldn't be iterated over.")
    {
        REQUIRE(container_printer::traits::is_printable_as_container_v<std::string> == false);
    }

    SECTION("Detect std::wstring as a type that shouldn't be iterated over.")
    {
        REQUIRE(container_printer::traits::is_printable_as_container_v<std::wstring> == false);
    }

    SECTION("Detect inherited iterable container type.")
    {
        REQUIRE(
            container_printer::traits::is_printable_as_container_v<vector_wrapper<int>> == true);
    }
}

TEST_CASE("Delimiter Validation")
{
    SECTION("Verify narrow character delimiters for a non-specialized container type.")
    {
        constexpr auto delimiters = container_printer::decorator::delimiters<char[], char>::values;

        REQUIRE(delimiters.prefix == std::string_view{ "[" });
        REQUIRE(delimiters.separator == std::string_view{ ", " });
        REQUIRE(delimiters.suffix == std::string_view{ "]" });
    }

    SECTION("Verify wide character delimiters for a non-specialized container type.")
    {
        constexpr auto delimiters =
            container_printer::decorator::delimiters<wchar_t[], wchar_t>::values;

        REQUIRE(delimiters.prefix == std::wstring_view{ L"[" });
        REQUIRE(delimiters.separator == std::wstring_view{ L", " });
        REQUIRE(delimiters.suffix == std::wstring_view{ L"]" });
    }

    SECTION("Verify narrow character delimiters for a std::set<...>.")
    {
        constexpr auto delimiters =
            container_printer::decorator::delimiters<std::set<int>, char>::values;

        REQUIRE(delimiters.prefix == std::string_view{ "{" });
        REQUIRE(delimiters.separator == std::string_view{ ", " });
        REQUIRE(delimiters.suffix == std::string_view{ "}" });
    }

    SECTION("Verify wide character delimiters for a std::set<...>.")
    {
        constexpr auto delimiters =
            container_printer::decorator::delimiters<std::set<int>, wchar_t>::values;

        REQUIRE(delimiters.prefix == std::wstring_view{ L"{" });
        REQUIRE(delimiters.separator == std::wstring_view{ L", " });
        REQUIRE(delimiters.suffix == std::wstring_view{ L"}" });
    }

    SECTION("Verify narrow character delimiters for a std::pair<...>.")
    {
        constexpr auto delimiters =
            container_printer::decorator::delimiters<std::pair<int, int>, char>::values;

        REQUIRE(delimiters.prefix == std::string_view{ "(" });
        REQUIRE(delimiters.separator == std::string_view{ ", " });
        REQUIRE(delimiters.suffix == std::string_view{ ")" });
    }

    SECTION("Verify wide character delimiters for a std::pair<...>.")
    {
        constexpr auto delimiters =
            container_printer::decorator::delimiters<std::pair<int, int>, wchar_t>::values;

        REQUIRE(delimiters.prefix == std::wstring_view{ L"(" });
        REQUIRE(delimiters.separator == std::wstring_view{ L", " });
        REQUIRE(delimiters.suffix == std::wstring_view{ L")" });
    }

    SECTION("Verify narrow character delimiters for a std::tuple<...>.")
    {
        constexpr auto delimiters =
            container_printer::decorator::delimiters<std::tuple<int, int>, char>::values;

        REQUIRE(delimiters.prefix == std::string_view{ "<" });
        REQUIRE(delimiters.separator == std::string_view{ ", " });
        REQUIRE(delimiters.suffix == std::string_view{ ">" });
    }

    SECTION("Verify wide character delimiters for a std::tuple<...>.")
    {
        constexpr auto delimiters =
            container_printer::decorator::delimiters<std::tuple<int, int>, wchar_t>::values;
            
        REQUIRE(delimiters.prefix == std::wstring_view{ L"<" });
        REQUIRE(delimiters.separator == std::wstring_view{ L", " });
        REQUIRE(delimiters.suffix == std::wstring_view{ L">" });
    }
}

TEST_CASE("Printing of Raw Arrays")
{
    std::stringstream narrow_buffer;
    auto* const old_narrow_buffer = std::cout.rdbuf(narrow_buffer.rdbuf());
    const scope_exit reset_narrow_buffer = [&]() noexcept { std::cout.rdbuf(old_narrow_buffer); };

    std::wstringstream wide_buffer;
    auto* const old_wide_buffer = std::wcout.rdbuf(wide_buffer.rdbuf());
    const scope_exit reset_wide_buffer = [&]() noexcept { std::wcout.rdbuf(old_wide_buffer); };

    SECTION("Printing a narrow character array.")
    {
        const auto& array = "Hello";
        std::cout << array << std::flush;

        REQUIRE(narrow_buffer.str() == std::string{ "Hello" });
    }

    SECTION("Printing a wide character array.")
    {
        const auto& array = L"Hello";
        std::wcout << array << std::flush;

        REQUIRE(wide_buffer.str() == std::wstring{ L"Hello" });
    }

    SECTION("Printing an array of ints to a narrow stream.")
    {
        const int array[5] = { 1, 2, 3, 4, 5 };
        std::cout << array << std::flush;

        REQUIRE(narrow_buffer.str() == std::string{ "[1, 2, 3, 4, 5]" });
    }

    SECTION("Printing an array of ints to a wide stream.")
    {
        const int array[5] = { 1, 2, 3, 4, 5 };
        std::wcout << array << std::flush;

        REQUIRE(wide_buffer.str() == std::wstring{ L"[1, 2, 3, 4, 5]" });
    }
}

TEST_CASE("Printing of Standard Library Containers")
{
    std::stringstream narrow_buffer;
    auto* const old_narrow_buffer = std::cout.rdbuf(narrow_buffer.rdbuf());
    const scope_exit reset_narrow_buffer = [&]() noexcept { std::cout.rdbuf(old_narrow_buffer); };

    std::wstringstream wide_buffer;
    auto* const old_wide_buffer = std::wcout.rdbuf(wide_buffer.rdbuf());
    const scope_exit reset_wide_buffer = [&]() noexcept { std::wcout.rdbuf(old_wide_buffer); };

    SECTION("Printing a std::pair<...> to a narrow stream.")
    {
        const auto pair = std::make_pair(10, 100);
        std::cout << pair << std::flush;

        REQUIRE(narrow_buffer.str() == std::string{ "(10, 100)" });
    }

    SECTION("Printing a std::pair<...> to a wide stream.")
    {
        const auto pair = std::make_pair(10, 100);
        std::wcout << pair << std::flush;

        REQUIRE(wide_buffer.str() == std::wstring{ L"(10, 100)" });
    }

    SECTION("Printing an empty std::vector<...> to a wide stream.")
    {
        const std::vector<int> vector{};
        std::wcout << vector << std::flush;

        REQUIRE(wide_buffer.str() == std::wstring{ L"[]" });
    }

    SECTION("Printing a populated std::vector<...> to a narrow stream.")
    {
        const std::vector<int> vector{ 1, 2, 3, 4 };
        std::cout << vector << std::flush;

        REQUIRE(narrow_buffer.str() == std::string{ "[1, 2, 3, 4]" });
    }

    SECTION("Printing a populated std::vector<...> to a wide stream.")
    {
        const std::vector<int> vector{ 1, 2, 3, 4 };
        std::wcout << vector << std::flush;

        REQUIRE(wide_buffer.str() == std::wstring{ L"[1, 2, 3, 4]" });
    }

    SECTION("Printing an empty std::set<...> to a narrow stream.")
    {
        const std::set<int> set{};
        std::cout << set << std::flush;

        REQUIRE(narrow_buffer.str() == std::string{ "{}" });
    }

    SECTION("Printing an empty std::set<...> to a wide stream.")
    {
        const std::set<int> set{};
        std::wcout << set << std::flush;

        REQUIRE(wide_buffer.str() == std::wstring{ L"{}" });
    }

    SECTION("Printing a populated std::set<...> to a narrow stream.")
    {
        const std::set<int> set{ 1, 2, 3, 4 };
        std::cout << set << std::flush;

        REQUIRE(narrow_buffer.str() == std::string{ "{1, 2, 3, 4}" });
    }

    SECTION("Printing a populated std::set<...> to a wide stream.")
    {
        const std::set<int> set{ 1, 2, 3, 4 };
        std::wcout << set << std::flush;

        REQUIRE(wide_buffer.str() == std::wstring{ L"{1, 2, 3, 4}" });
    }

    SECTION("Printing a populated std::multiset<...> to a narrow stream.")
    {
        const std::multiset<int> multiset{ 1, 2, 3, 4 };
        std::cout << multiset << std::flush;

        REQUIRE(narrow_buffer.str() == std::string{ "{1, 2, 3, 4}" });
    }

    SECTION("Printing a populated std::multiset<...> to a wide stream.")
    {
        const std::multiset<int> multiset{ 1, 2, 3, 4 };
        std::wcout << multiset << std::flush;

        REQUIRE(wide_buffer.str() == std::wstring{ L"{1, 2, 3, 4}" });
    }

    SECTION("Printing an empty std::tuple<...> to a narrow stream.")
    {
        const auto tuple = std::make_tuple();
        std::cout << tuple << std::flush;

        REQUIRE(narrow_buffer.str() == std::string{ "<>" });
    }

    SECTION("Printing an empty std::tuple<...> to a wide stream.")
    {
        const auto tuple = std::make_tuple();
        std::wcout << tuple << std::flush;

        REQUIRE(wide_buffer.str() == std::wstring{ L"<>" });
    }

    SECTION("Printing a populated std::tuple<...> to a narrow stream.")
    {
        const auto tuple = std::make_tuple(1, 2, 3, 4, 5);
        std::cout << tuple << std::flush;

        REQUIRE(narrow_buffer.str() == std::string{ "<1, 2, 3, 4, 5>" });
    }

    SECTION("Printing a populated std::tuple<...> to a wide stream.")
    {
        const auto tuple = std::make_tuple(1, 2, 3, 4, 5);
        std::wcout << tuple << std::flush;

        REQUIRE(wide_buffer.str() == std::wstring{ L"<1, 2, 3, 4, 5>" });
    }
}

TEST_CASE("Printing of Nested Containers")
{
    std::stringstream narrow_buffer;
    auto* const old_narrow_buffer = std::cout.rdbuf(narrow_buffer.rdbuf());
    const scope_exit reset_narrow_buffer = [&]() noexcept { std::cout.rdbuf(old_narrow_buffer); };

    std::wstringstream wide_buffer;
    auto* const old_wide_buffer = std::wcout.rdbuf(wide_buffer.rdbuf());
    const scope_exit reset_wide_buffer = [&]() noexcept { std::wcout.rdbuf(old_wide_buffer); };

    SECTION("Printing a populated std::map<...> to a narrow stream.")
    {
        const auto map =
            std::map<int, std::string>{ { 1, "Template" }, { 2, "Meta" }, { 3, "Programming" } };

        std::cout << map << std::flush;

        REQUIRE(
            narrow_buffer.str() == std::string{ "[(1, Template), (2, Meta), (3, Programming)]" });
    }

    SECTION("Printing a populated std::map<...> to a wide stream.")
    {
        const auto map = std::map<int, std::wstring>{ { 1, L"Template" },
                                                      { 2, L"Meta" },
                                                      { 3, L"Programming" } };

        std::wcout << map << std::flush;

        REQUIRE(
            wide_buffer.str() == std::wstring{ L"[(1, Template), (2, Meta), (3, Programming)]" });
    }

    SECTION("Printing a populated std::vector<std::tuple<...>> to a narrow stream.")
    {
        const auto vector =
            std::vector<std::tuple<int, double, std::string>>{ std::make_tuple(1, 0.1, "Hello"),
                                                               std::make_tuple(2, 0.2, "World") };

        std::cout << vector << std::flush;

        REQUIRE(narrow_buffer.str() == std::string{ "[<1, 0.1, Hello>, <2, 0.2, World>]" });
    }

    SECTION("Printing a populated std::vector<std::tuple<...>> to a narrow stream.")
    {
        const auto vector =
            std::vector<std::tuple<int, double, std::wstring>>{ std::make_tuple(1, 0.1, L"Hello"),
                                                                std::make_tuple(2, 0.2, L"World") };

        std::wcout << vector << std::flush;

        REQUIRE(wide_buffer.str() == std::wstring{ L"[<1, 0.1, Hello>, <2, 0.2, World>]" });
    }

    SECTION("Printing a populated std::pair<int, std::vector<std::pair<std::string, std::string>>>")
    {
        const auto pair = std::make_pair(
            10, std::vector<std::pair<std::string, std::string>>{
                    std::make_pair("Why", "Not?"), std::make_pair("Someone", "Might!") });

        std::cout << pair << std::flush;

        REQUIRE(narrow_buffer.str() == std::string{ "(10, [(Why, Not?), (Someone, Might!)])" });
    }
}

TEST_CASE("Printing with Custom Formatters")
{
    std::stringstream narrow_buffer;
    auto* const old_narrow_buffer = std::cout.rdbuf(narrow_buffer.rdbuf());
    const scope_exit reset_narrow_buffer = [&]() noexcept { std::cout.rdbuf(old_narrow_buffer); };

    std::wstringstream wide_buffer;
    auto* const old_wide_buffer = std::wcout.rdbuf(wide_buffer.rdbuf());
    const scope_exit reset_wide_buffer = [&]() noexcept { std::wcout.rdbuf(old_wide_buffer); };

    SECTION("Printing a populated std::vector<...> to a wide stream.")
    {
        const auto container = std::vector<int>{ 1, 2, 3, 4 };

        container_printer::to_stream(std::wcout, container, custom_formatter{}) << std::flush;

        REQUIRE(wide_buffer.str() == std::wstring{ L"$$ 1 | 2 | 3 | 4 $$" });
    }

    SECTION("Printing a populated std::tuple<...> to a wide stream.")
    {
        const auto container = std::make_tuple(1, 2, 3, 4);

        container_printer::to_stream(std::wcout, container, custom_formatter{}) << std::flush;

        REQUIRE(wide_buffer.str() == std::wstring{ L"$$ 1 | 2 | 3 | 4 $$" });
    }

    SECTION("Printing a populated std::pair<...> to a wide stream.")
    {
        const auto container = std::make_pair(1, 2);

        container_printer::to_stream(std::wcout, container, custom_formatter{}) << std::flush;

        REQUIRE(wide_buffer.str() == std::wstring{ L"$$ 1 | 2 $$" });
    }
}
