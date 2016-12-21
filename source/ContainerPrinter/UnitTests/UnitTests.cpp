#pragma once

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "Catch.hpp"

#include "ScopeExit.hpp"

#include "../ContainerPrinter/ContainerPrinter.hpp"

#include <algorithm>
#include <list>
#include <set>
#include <unordered_map>
#include <vector>

namespace
{
   template<typename Type>
   class VectorWrapper : public std::vector<Type> { };
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

TEST_CASE("Delimiters")
{
   SECTION("Verify narrow character delimiters for a non-specialized container type.")
   {
      constexpr auto delimiters = ContainerPrinter::delimiters<char[], char>::values;
      REQUIRE(delimiters.prefix == "[");
      REQUIRE(delimiters.separator == ", ");
      REQUIRE(delimiters.suffix == "]");
   }

   SECTION("Verify wide character delimiters for a non-specialized container type.")
   {
      constexpr auto delimiters = ContainerPrinter::delimiters<wchar_t[], wchar_t>::values;
      REQUIRE(delimiters.prefix == L"[");
      REQUIRE(delimiters.separator == L", ");
      REQUIRE(delimiters.suffix == L"]");
   }

   SECTION("Verify narrow character delimiters for a std::set<...>.")
   {
      constexpr auto delimiters = ContainerPrinter::delimiters<std::set<int>, char>::values;
      REQUIRE(delimiters.prefix == "{");
      REQUIRE(delimiters.separator == ", ");
      REQUIRE(delimiters.suffix == "}");
   }

   SECTION("Verify wide character delimiters for a std::set<...>.")
   {
      constexpr auto delimiters = ContainerPrinter::delimiters<std::set<int>, wchar_t>::values;
      REQUIRE(delimiters.prefix == L"{");
      REQUIRE(delimiters.separator == L", ");
      REQUIRE(delimiters.suffix == L"}");
   }

   SECTION("Verify narrow character delimiters for a std::pair<...>.")
   {
      constexpr auto delimiters = ContainerPrinter::delimiters<std::pair<int, int>, char>::values;
      REQUIRE(delimiters.prefix == "(");
      REQUIRE(delimiters.separator == ", ");
      REQUIRE(delimiters.suffix == ")");
   }

   SECTION("Verify wide character delimiters for a std::pair<...>.")
   {
      constexpr auto delimiters = ContainerPrinter::delimiters<std::pair<int, int>, wchar_t>::values;
      REQUIRE(delimiters.prefix == L"(");
      REQUIRE(delimiters.separator == L", ");
      REQUIRE(delimiters.suffix == L")");
   }

   SECTION("Verify narrow character delimiters for a std::tuple<...>.")
   {
      constexpr auto delimiters = ContainerPrinter::delimiters<std::tuple<int, int>, char>::values;
      REQUIRE(delimiters.prefix == "<");
      REQUIRE(delimiters.separator == ", ");
      REQUIRE(delimiters.suffix == ">");
   }

   SECTION("Verify wide character delimiters for a std::tuple<...>.")
   {
      constexpr auto delimiters = ContainerPrinter::delimiters<std::tuple<int, int>, wchar_t>::values;
      REQUIRE(delimiters.prefix == L"<");
      REQUIRE(delimiters.separator == L", ");
      REQUIRE(delimiters.suffix == L">");
   }
}

TEST_CASE("Container Printing")
{
   std::stringstream narrowBuffer;
   std::streambuf* oldNarrowBuffer = std::cout.rdbuf(narrowBuffer.rdbuf());

   ON_SCOPE_EXIT{ std::cout.rdbuf(oldNarrowBuffer); };

   std::wstringstream wideBuffer;
   std::wstreambuf* oldWideBuffer = std::wcout.rdbuf(wideBuffer.rdbuf());

   ON_SCOPE_EXIT{ std::wcout.rdbuf(oldWideBuffer); };

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

   SECTION("Printing an empty std::vector<...> to a narrow stream.")
   {
      const std::vector<int> vector{ };
      std::cout << vector << std::flush;

      REQUIRE(narrowBuffer.str() == std::string{ "[]" });
   }

   SECTION("Printing an empty std::vector<...> to a wide stream.")
   {
      const std::vector<int> vector{};
      std::wcout << vector << std::flush;

      REQUIRE(wideBuffer.str() == std::wstring{ L"[]" });
   }

   SECTION("Printing a populated std::vector<...> to a narrow stream.")
   {
      const std::vector<int> vector { 1, 2, 3, 4 };
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
      const std::set<int> set{ };
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
      const std::set<int> set { 1, 2, 3, 4 };
      std::cout << set << std::flush;

      REQUIRE(narrowBuffer.str() == std::string{ "{1, 2, 3, 4}" });
   }

   SECTION("Printing a populated std::set<...> to a wide stream.")
   {
      const std::set<int> set{ 1, 2, 3, 4 };
      std::wcout << set << std::flush;

      REQUIRE(wideBuffer.str() == std::wstring{ L"{1, 2, 3, 4}" });
   }

   SECTION("Printing a populated std::set<...> to a narrow stream.")
   {
      const std::multiset<int> multiset{ 1, 2, 3, 4 };
      std::cout << multiset << std::flush;

      REQUIRE(narrowBuffer.str() == std::string{ "{1, 2, 3, 4}" });
   }

   SECTION("Printing a populated std::set<...> to a wide stream.")
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
      const auto tuple = std::make_tuple(1, 2, 3);
      std::cout << tuple << std::flush;

      REQUIRE(narrowBuffer.str() == std::string{ "<1, 2, 3>" });
   }

   SECTION("Printing a populated std::tuple<...> to a wide stream.")
   {
      const auto tuple = std::make_tuple(1, 2, 3);
      std::wcout << tuple << std::flush;

      REQUIRE(wideBuffer.str() == std::wstring{ L"<1, 2, 3>" });
   }
}

TEST_CASE("Nested Containers")
{
   std::stringstream narrowBuffer;
   std::streambuf* oldNarrowBuffer = std::cout.rdbuf(narrowBuffer.rdbuf());

   ON_SCOPE_EXIT{ std::cout.rdbuf(oldNarrowBuffer); };

   std::wstringstream wideBuffer;
   std::wstreambuf* oldWideBuffer = std::wcout.rdbuf(wideBuffer.rdbuf());

   ON_SCOPE_EXIT{ std::wcout.rdbuf(oldWideBuffer); };

   SECTION("Printing a populated std::unordered_map<...> to a narrow stream.")
   {
      const auto map = std::unordered_map<int, std::string>
      {
         { 1, "Template" },
         { 2, "Meta" },
         { 3, "Programming" }
      };

      std::cout << map << std::flush;

      REQUIRE(narrowBuffer.str() == std::string{ "[(1, Template), (2, Meta), (3, Programming)]" });
   }

   SECTION("Printing a populated std::unordered_map<...> to a wide stream.")
   {
      const auto map = std::unordered_map<int, std::wstring>
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
