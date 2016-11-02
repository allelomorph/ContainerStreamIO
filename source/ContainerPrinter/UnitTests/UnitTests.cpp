#pragma once

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "Catch.hpp"

#include "ScopeExit.hpp"

#include "../ContainerPrinter/ContainerPrinter.hpp"

#include <algorithm>
#include <list>
#include <set>
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
      constexpr auto isAContainer = Traits::is_printable_as_container_v<std::vector<int>>;
      REQUIRE(isAContainer == true);
   }

   SECTION("Detect std::list<...> as being an iterable container type.")
   {
      constexpr auto isAContainer = Traits::is_printable_as_container_v<std::list<int>>;
      REQUIRE(isAContainer == true);
   }

   SECTION("Detect std::set<...> as being an iterable container type.")
   {
      constexpr auto isAContainer = Traits::is_printable_as_container_v<std::set<int>>;
      REQUIRE(isAContainer == true);
   }

   SECTION("Detect array as being an iterable container type.")
   {
      constexpr auto isAContainer = Traits::is_printable_as_container_v<int[10]>;
      REQUIRE(isAContainer == true);
   }

   SECTION("Detect std::string as a type that shouldn't be iterated over.")
   {
      constexpr auto isAContainer = Traits::is_printable_as_container_v<std::string>;
      REQUIRE(isAContainer == false);
   }

   SECTION("Detect std::wstring as a type that shouldn't be iterated over.")
   {
      constexpr auto isAContainer = Traits::is_printable_as_container_v<std::wstring>;
      REQUIRE(isAContainer == false);
   }

   SECTION("Detect inherited iterable container type.")
   {
      constexpr auto isAContainer = Traits::is_printable_as_container_v<VectorWrapper<int>>;
      REQUIRE(isAContainer == true);
   }
}

TEST_CASE("Delimiters")
{
   SECTION("Verify narrow character delimiters for a non-specialized container type.")
   {
      constexpr auto delimiters = Printer::delimiters<char[], char>::values;
      REQUIRE(delimiters.prefix == "[");
      REQUIRE(delimiters.delimiter == ", ");
      REQUIRE(delimiters.postfix == "]");
   }

   SECTION("Verify wide character delimiters for a non-specialized container type.")
   {
      constexpr auto delimiters = Printer::delimiters<wchar_t[], wchar_t>::values;
      REQUIRE(delimiters.prefix == L"[");
      REQUIRE(delimiters.delimiter == L", ");
      REQUIRE(delimiters.postfix == L"]");
   }

   SECTION("Verify narrow character delimiters for a std::set<...>.")
   {
      constexpr auto delimiters = Printer::delimiters<std::set<int>, char>::values;
      REQUIRE(delimiters.prefix == "{");
      REQUIRE(delimiters.delimiter == ", ");
      REQUIRE(delimiters.postfix == "}");
   }

   SECTION("Verify wide character delimiters for a std::set<...>.")
   {
      constexpr auto delimiters = Printer::delimiters<std::set<int>, wchar_t>::values;
      REQUIRE(delimiters.prefix == L"{");
      REQUIRE(delimiters.delimiter == L", ");
      REQUIRE(delimiters.postfix == L"}");
   }

   SECTION("Verify narrow character delimiters for a std::pair<...>.")
   {
      constexpr auto delimiters = Printer::delimiters<std::pair<int, int>, char>::values;
      REQUIRE(delimiters.prefix == "(");
      REQUIRE(delimiters.delimiter == ", ");
      REQUIRE(delimiters.postfix == ")");
   }

   SECTION("Verify wide character delimiters for a std::pair<...>.")
   {
      constexpr auto delimiters = Printer::delimiters<std::pair<int, int>, wchar_t>::values;
      REQUIRE(delimiters.prefix == L"(");
      REQUIRE(delimiters.delimiter == L", ");
      REQUIRE(delimiters.postfix == L")");
   }

   SECTION("Verify narrow character delimiters for a std::tuple<...>.")
   {
      constexpr auto delimiters = Printer::delimiters<std::tuple<int, int>, char>::values;
      REQUIRE(delimiters.prefix == "<");
      REQUIRE(delimiters.delimiter == ", ");
      REQUIRE(delimiters.postfix == ">");
   }

   SECTION("Verify wide character delimiters for a std::tuple<...>.")
   {
      constexpr auto delimiters = Printer::delimiters<std::tuple<int, int>, wchar_t>::values;
      REQUIRE(delimiters.prefix == L"<");
      REQUIRE(delimiters.delimiter == L", ");
      REQUIRE(delimiters.postfix == L">");
   }
}

TEST_CASE("Container Printing")
{
   std::stringstream buffer;
   std::streambuf* oldBuffer = std::cout.rdbuf(buffer.rdbuf());

   ON_SCOPE_EXIT{ std::cout.rdbuf(oldBuffer); };

   SECTION("Printing a character array")
   {
      const auto& array = "Hello";
      std::cout << array << std::flush;

      REQUIRE(buffer.str() == std::string{ "Hello" });
   }

   SECTION("Printing an array of ints")
   {
      const int array[5] = { 1, 2, 3, 4, 5 };
      std::cout << array << std::flush;

      REQUIRE(buffer.str() == std::string{ "[1, 2, 3, 4, 5]" });
   }

   SECTION("Printing a std::pair<...>.")
   {
      const auto pair = std::make_pair(10, 100);
      std::cout << pair << std::flush;

      REQUIRE(buffer.str() == std::string{ "(10, 100)" });
   }

   SECTION("Printing an empty std::vector<...>.")
   {
      const std::vector<int> vector{ };
      std::cout << vector << std::flush;

      REQUIRE(buffer.str() == std::string{ "[]" });
   }

   SECTION("Printing a populated std::vector<...>.")
   {
      const std::vector<int> vector { 1, 2, 3, 4 };
      std::cout << vector << std::flush;

      REQUIRE(buffer.str() == std::string{ "[1, 2, 3, 4]" });
   }

   SECTION("Printing an empty std::set<...>.")
   {
      const std::set<int> set{ };
      std::cout << set << std::flush;

      REQUIRE(buffer.str() == std::string{ "{}" });
   }

   SECTION("Printing a populated std::set<...>.")
   {
      const std::set<int> set { 1, 2, 3, 4 };
      std::cout << set << std::flush;

      REQUIRE(buffer.str() == std::string{ "{1, 2, 3, 4}" });
   }

   SECTION("Printing an empty std::tuple<...>.")
   {
      const auto tuple = std::make_tuple();
      std::cout << tuple << std::flush;

      REQUIRE(buffer.str() == std::string{ "<>" });
   }

   SECTION("Printing a populated std::tuple<...>.")
   {
      const auto tuple = std::make_tuple(1, 2, 3);
      std::cout << tuple << std::flush;

      REQUIRE(buffer.str() == std::string{ "<1, 2, 3>" });
   }
}
