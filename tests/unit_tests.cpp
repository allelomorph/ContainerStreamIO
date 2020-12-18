#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <catch2/catch.hpp>

#include "container_printer.h"

#include <algorithm>
#include <list>
#include <functional>
#include <set>
#include <map>
#include <vector>

namespace
{
    /**
     * @brief An RAII wrapper that will execute an action when the wrapper falls out of scope, or is
     * otherwise destroyed.
     */
    template <typename LambdaType> class ScopeExit
    {
    public:
        ScopeExit(LambdaType&& lambda) noexcept : m_lambda{ std::move(lambda) }
        {
            static_assert(
                std::is_nothrow_invocable<LambdaType>::value,
                "Since the callable type is invoked from the destructor, exceptions are not allowed.");
        }

        ~ScopeExit() noexcept
        {
            m_lambda();
        }

        ScopeExit(const ScopeExit&) = delete;
        ScopeExit& operator=(const ScopeExit&) = delete;

        ScopeExit(ScopeExit&&) = default;
        ScopeExit& operator=(ScopeExit&&) = default;

    private:
        LambdaType m_lambda;
    };


   template<typename Type>
   class VectorWrapper : public std::vector<Type> 
   {
   };

   /**
   * @brief Custom formatting struct.
   */
   struct CustomFormatter
   {
      template<typename StreamType>
      void print_prefix(StreamType& stream) const noexcept
      {
         stream << L"$$ ";
      }

      template<
         typename StreamType,
         typename ElementType
      >
      void print_element(
         StreamType& stream,
         const ElementType& element) const noexcept
      {
         stream << element;
      }

      template<typename StreamType>
      void print_delimiter(StreamType& stream) const noexcept
      {
         stream << L" | ";
      }

      template<typename StreamType>
      void print_suffix(StreamType& stream) const noexcept
      {
         stream << L" $$";
      }
   };
}

TEST_CASE("Traits")
{
   SECTION("Detect std::vector<...> as being an iterable container type.")
   {
      constexpr auto isAContainer = ContainerPrinter::Traits::is_printable_as_container_v<std::vector<int>>;
      REQUIRE(isAContainer == true);
   }

   SECTION("Detect std::list<...> as being an iterable container type.")
   {
      constexpr auto isAContainer = ContainerPrinter::Traits::is_printable_as_container_v<std::list<int>>;
      REQUIRE(isAContainer == true);
   }

   SECTION("Detect std::set<...> as being an iterable container type.")
   {
      constexpr auto isAContainer = ContainerPrinter::Traits::is_printable_as_container_v<std::set<int>>;
      REQUIRE(isAContainer == true);
   }

   SECTION("Detect array as being an iterable container type.")
   {
      constexpr auto isAContainer = ContainerPrinter::Traits::is_printable_as_container_v<int[10]>;
      REQUIRE(isAContainer == true);
   }

   SECTION("Detect std::string as a type that shouldn't be iterated over.")
   {
      constexpr auto isAContainer = ContainerPrinter::Traits::is_printable_as_container_v<std::string>;
      REQUIRE(isAContainer == false);
   }

   SECTION("Detect std::wstring as a type that shouldn't be iterated over.")
   {
      constexpr auto isAContainer = ContainerPrinter::Traits::is_printable_as_container_v<std::wstring>;
      REQUIRE(isAContainer == false);
   }

   SECTION("Detect inherited iterable container type.")
   {
      constexpr auto isAContainer = ContainerPrinter::Traits::is_printable_as_container_v<VectorWrapper<int>>;
      REQUIRE(isAContainer == true);
   }
}

TEST_CASE("Delimiter Validation")
{
   SECTION("Verify narrow character delimiters for a non-specialized container type.")
   {
      constexpr auto delimiters = ContainerPrinter::Decorator::delimiters<char[], char>::values;
      REQUIRE(delimiters.prefix == std::string_view{ "[" });
      REQUIRE(delimiters.separator == std::string_view{ ", " });
      REQUIRE(delimiters.suffix == std::string_view{ "]" });
   }

   SECTION("Verify wide character delimiters for a non-specialized container type.")
   {
      constexpr auto delimiters = ContainerPrinter::Decorator::delimiters<wchar_t[], wchar_t>::values;
      REQUIRE(delimiters.prefix == std::wstring_view{ L"[" });
      REQUIRE(delimiters.separator == std::wstring_view{ L", " });
      REQUIRE(delimiters.suffix == std::wstring_view{ L"]" });
   }

   SECTION("Verify narrow character delimiters for a std::set<...>.")
   {
      constexpr auto delimiters = ContainerPrinter::Decorator::delimiters<std::set<int>, char>::values;
      REQUIRE(delimiters.prefix == std::string_view{ "{" });
      REQUIRE(delimiters.separator == std::string_view{ ", " });
      REQUIRE(delimiters.suffix == std::string_view{ "}" });
   }

   SECTION("Verify wide character delimiters for a std::set<...>.")
   {
      constexpr auto delimiters = ContainerPrinter::Decorator::delimiters<std::set<int>, wchar_t>::values;
      REQUIRE(delimiters.prefix == std::wstring_view{ L"{" });
      REQUIRE(delimiters.separator == std::wstring_view{ L", " });
      REQUIRE(delimiters.suffix == std::wstring_view{ L"}" });
   }

   SECTION("Verify narrow character delimiters for a std::pair<...>.")
   {
      constexpr auto delimiters = ContainerPrinter::Decorator::delimiters<std::pair<int, int>, char>::values;
      REQUIRE(delimiters.prefix == std::string_view{ "(" });
      REQUIRE(delimiters.separator == std::string_view{ ", " });
      REQUIRE(delimiters.suffix == std::string_view{ ")" });
   }

   SECTION("Verify wide character delimiters for a std::pair<...>.")
   {
      constexpr auto delimiters = ContainerPrinter::Decorator::delimiters<std::pair<int, int>, wchar_t>::values;
      REQUIRE(delimiters.prefix == std::wstring_view{ L"(" });
      REQUIRE(delimiters.separator == std::wstring_view{ L", " });
      REQUIRE(delimiters.suffix == std::wstring_view{ L")" });
   }

   SECTION("Verify narrow character delimiters for a std::tuple<...>.")
   {
      constexpr auto delimiters = ContainerPrinter::Decorator::delimiters<std::tuple<int, int>, char>::values;
      REQUIRE(delimiters.prefix == std::string_view{ "<" });
      REQUIRE(delimiters.separator == std::string_view{ ", " });
      REQUIRE(delimiters.suffix == std::string_view{ ">" });
   }

   SECTION("Verify wide character delimiters for a std::tuple<...>.")
   {
      constexpr auto delimiters = ContainerPrinter::Decorator::delimiters<std::tuple<int, int>, wchar_t>::values;
      REQUIRE(delimiters.prefix == std::wstring_view{ L"<" });
      REQUIRE(delimiters.separator == std::wstring_view{ L", " });
      REQUIRE(delimiters.suffix == std::wstring_view{ L">" });
   }
}

TEST_CASE("Printing of Raw Arrays")
{
   std::stringstream narrowBuffer;
   auto* const oldNarrowBuffer = std::cout.rdbuf(narrowBuffer.rdbuf());
   const ScopeExit resetNarrow = [&]() noexcept { std::cout.rdbuf(oldNarrowBuffer); };

   std::wstringstream wideBuffer;
   auto* const oldWideBuffer = std::wcout.rdbuf(wideBuffer.rdbuf());
   const ScopeExit resetWide = [&]() noexcept { std::wcout.rdbuf(oldWideBuffer); };

   SECTION("Printing a narrow character array.")
   {
      const auto& array = "Hello";
      std::cout << array << std::flush;

      REQUIRE(narrowBuffer.str() == std::string{ "Hello" });
   }

   SECTION("Printing a wide character array.")
   {
      const auto& array = L"Hello";
      std::wcout << array << std::flush;

      REQUIRE(wideBuffer.str() == std::wstring{ L"Hello" });
   }

   SECTION("Printing an array of ints to a narrow stream.")
   {
      const int array[5] = { 1, 2, 3, 4, 5 };
      std::cout << array << std::flush;

      REQUIRE(narrowBuffer.str() == std::string{ "[1, 2, 3, 4, 5]" });
   }

   SECTION("Printing an array of ints to a wide stream.")
   {
      const int array[5] = { 1, 2, 3, 4, 5 };
      std::wcout << array << std::flush;

      REQUIRE(wideBuffer.str() == std::wstring{ L"[1, 2, 3, 4, 5]" });
   }
}

TEST_CASE("Printing of Standard Library Containers")
{
   std::stringstream narrowBuffer;
   auto* const oldNarrowBuffer = std::cout.rdbuf(narrowBuffer.rdbuf());
   const ScopeExit resetNarrow = [&]() noexcept { std::cout.rdbuf(oldNarrowBuffer); };

   std::wstringstream wideBuffer;
   auto* const oldWideBuffer = std::wcout.rdbuf(wideBuffer.rdbuf());
   const ScopeExit resetWide = [&]() noexcept { std::wcout.rdbuf(oldWideBuffer); };

   SECTION("Printing a std::pair<...> to a narrow stream.")
   {
      const auto pair = std::make_pair(10, 100);
      std::cout << pair << std::flush;

      REQUIRE(narrowBuffer.str() == std::string{ "(10, 100)" });
   }

   SECTION("Printing a std::pair<...> to a wide stream.")
   {
      const auto pair = std::make_pair(10, 100);
      std::wcout << pair << std::flush;

      REQUIRE(wideBuffer.str() == std::wstring{ L"(10, 100)" });
   }

   SECTION("Printing an empty std::vector<...> to a wide stream.")
   {
      const std::vector<int> vector{};
      std::wcout << vector << std::flush;

      REQUIRE(wideBuffer.str() == std::wstring{ L"[]" });
   }

   SECTION("Printing a populated std::vector<...> to a narrow stream.")
   {
      const std::vector<int> vector{ 1, 2, 3, 4 };
      std::cout << vector << std::flush;

      REQUIRE(narrowBuffer.str() == std::string{ "[1, 2, 3, 4]" });
   }

   SECTION("Printing a populated std::vector<...> to a wide stream.")
   {
      const std::vector<int> vector{ 1, 2, 3, 4 };
      std::wcout << vector << std::flush;

      REQUIRE(wideBuffer.str() == std::wstring{ L"[1, 2, 3, 4]" });
   }

   SECTION("Printing an empty std::set<...> to a narrow stream.")
   {
      const std::set<int> set{};
      std::cout << set << std::flush;

      REQUIRE(narrowBuffer.str() == std::string{ "{}" });
   }

   SECTION("Printing an empty std::set<...> to a wide stream.")
   {
      const std::set<int> set{};
      std::wcout << set << std::flush;

      REQUIRE(wideBuffer.str() == std::wstring{ L"{}" });
   }

   SECTION("Printing a populated std::set<...> to a narrow stream.")
   {
      const std::set<int> set{ 1, 2, 3, 4 };
      std::cout << set << std::flush;

      REQUIRE(narrowBuffer.str() == std::string{ "{1, 2, 3, 4}" });
   }

   SECTION("Printing a populated std::set<...> to a wide stream.")
   {
      const std::set<int> set{ 1, 2, 3, 4 };
      std::wcout << set << std::flush;

      REQUIRE(wideBuffer.str() == std::wstring{ L"{1, 2, 3, 4}" });
   }

   SECTION("Printing a populated std::multiset<...> to a narrow stream.")
   {
      const std::multiset<int> multiset{ 1, 2, 3, 4 };
      std::cout << multiset << std::flush;

      REQUIRE(narrowBuffer.str() == std::string{ "{1, 2, 3, 4}" });
   }

   SECTION("Printing a populated std::multiset<...> to a wide stream.")
   {
      const std::multiset<int> multiset{ 1, 2, 3, 4 };
      std::wcout << multiset << std::flush;

      REQUIRE(wideBuffer.str() == std::wstring{ L"{1, 2, 3, 4}" });
   }

   SECTION("Printing an empty std::tuple<...> to a narrow stream.")
   {
      const auto tuple = std::make_tuple();
      std::cout << tuple << std::flush;

      REQUIRE(narrowBuffer.str() == std::string{ "<>" });
   }

   SECTION("Printing an empty std::tuple<...> to a wide stream.")
   {
      const auto tuple = std::make_tuple();
      std::wcout << tuple << std::flush;

      REQUIRE(wideBuffer.str() == std::wstring{ L"<>" });
   }

   SECTION("Printing a populated std::tuple<...> to a narrow stream.")
   {
      const auto tuple = std::make_tuple(1, 2, 3, 4, 5);
      std::cout << tuple << std::flush;

      REQUIRE(narrowBuffer.str() == std::string{ "<1, 2, 3, 4, 5>" });
   }

   SECTION("Printing a populated std::tuple<...> to a wide stream.")
   {
      const auto tuple = std::make_tuple(1, 2, 3, 4, 5);
      std::wcout << tuple << std::flush;

      REQUIRE(wideBuffer.str() == std::wstring{ L"<1, 2, 3, 4, 5>" });
   }
}

TEST_CASE("Printing of Nested Containers")
{
   std::stringstream narrowBuffer;
   auto* const oldNarrowBuffer = std::cout.rdbuf(narrowBuffer.rdbuf());
   const ScopeExit resetNarrow = [&]() noexcept { std::cout.rdbuf(oldNarrowBuffer); };

   std::wstringstream wideBuffer;
   auto* const oldWideBuffer = std::wcout.rdbuf(wideBuffer.rdbuf());
   const ScopeExit resetWide = [&]() noexcept { std::wcout.rdbuf(oldWideBuffer); };

   SECTION("Printing a populated std::map<...> to a narrow stream.")
   {
      const auto map = std::map<int, std::string>
      {
         { 1, "Template" },
         { 2, "Meta" },
         { 3, "Programming" }
      };

      std::cout << map << std::flush;

      REQUIRE(narrowBuffer.str() == std::string{ "[(1, Template), (2, Meta), (3, Programming)]" });
   }

   SECTION("Printing a populated std::map<...> to a wide stream.")
   {
      const auto map = std::map<int, std::wstring>
      {
         { 1, L"Template" },
         { 2, L"Meta" },
         { 3, L"Programming" }
      };

      std::wcout << map << std::flush;

      REQUIRE(wideBuffer.str() == std::wstring{ L"[(1, Template), (2, Meta), (3, Programming)]" });
   }

   SECTION("Printing a populated std::vector<std::tuple<...>> to a narrow stream.")
   {
      const auto vector = std::vector<std::tuple<int, double, std::string>>
      {
         std::make_tuple(1, 0.1, "Hello"),
         std::make_tuple(2, 0.2, "World")
      };

      std::cout << vector << std::flush;

      REQUIRE(narrowBuffer.str() == std::string{ "[<1, 0.1, Hello>, <2, 0.2, World>]" });
   }

   SECTION("Printing a populated std::vector<std::tuple<...>> to a narrow stream.")
   {
      const auto vector = std::vector<std::tuple<int, double, std::wstring>>
      {
         std::make_tuple(1, 0.1, L"Hello"),
         std::make_tuple(2, 0.2, L"World")
      };

      std::wcout << vector << std::flush;

      REQUIRE(wideBuffer.str() == std::wstring{ L"[<1, 0.1, Hello>, <2, 0.2, World>]" });
   }

   SECTION("Printing a populated std::pair<int, std::vector<std::pair<std::string, std::string>>>")
   {
      const auto pair = std::make_pair
      (
         10,
         std::vector<std::pair<std::string, std::string>>
         {
            std::make_pair("Why", "Not?"),
            std::make_pair("Someone", "Might!")
         }
      );

      std::cout << pair << std::flush;

      REQUIRE(narrowBuffer.str() == std::string{ "(10, [(Why, Not?), (Someone, Might!)])" });
   }
}

TEST_CASE("Printing with Custom Formatters")
{
   std::stringstream narrowBuffer;
   auto* const oldNarrowBuffer = std::cout.rdbuf(narrowBuffer.rdbuf());
   const ScopeExit resetNarrow = [&]() noexcept { std::cout.rdbuf(oldNarrowBuffer); };

   std::wstringstream wideBuffer;
   auto* const oldWideBuffer = std::wcout.rdbuf(wideBuffer.rdbuf());
   const ScopeExit resetWide = [&]() noexcept { std::wcout.rdbuf(oldWideBuffer); };

   SECTION("Printing a populated std::vector<...> to a wide stream.")
   {
      const auto container = std::vector<int>{ 1, 2, 3, 4 };

      ContainerPrinter::ToStream(std::wcout, container, CustomFormatter{ }) << std::flush;

      REQUIRE(wideBuffer.str() == std::wstring{ L"$$ 1 | 2 | 3 | 4 $$" });
   }

   SECTION("Printing a populated std::tuple<...> to a wide stream.")
   {
      const auto container = std::make_tuple( 1, 2, 3, 4 );

      ContainerPrinter::ToStream(std::wcout, container, CustomFormatter{ }) << std::flush;

      REQUIRE(wideBuffer.str() == std::wstring{ L"$$ 1 | 2 | 3 | 4 $$" });
   }

   SECTION("Printing a populated std::pair<...> to a wide stream.")
   {
      const auto container = std::make_pair(1, 2);

      ContainerPrinter::ToStream(std::wcout, container, CustomFormatter{ }) << std::flush;

      REQUIRE(wideBuffer.str() == std::wstring{ L"$$ 1 | 2 $$" });
   }
}
