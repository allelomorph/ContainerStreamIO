#pragma once

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <set>
#include <string>
#include <tuple>
#include <utility>

namespace ContainerPrinter
{
   namespace Traits
   {
      /**
      * @brief A neat SFINAE trick.
      *
      * @see N3911
      */
      template<class...>
      using void_t = void;

      /**
      * @brief Base case for the testing of STD compatible container types.
      */
      template<
         typename /*Type*/,
         typename = void
      >
      struct is_printable_as_container : public std::false_type
      {
      };

      /**
      * @brief Specialization to ensure that Standard Library compatible containers that have
      * `begin()`, `end()`, and `empty()` member functions are treated as printable containers.
      */
      template<typename Type>
      struct is_printable_as_container<
         Type,
         void_t<
            decltype(std::declval<Type&>().begin()),
            decltype(std::declval<Type&>().end()),
            decltype(std::declval<Type&>().empty())
         >
      > : public std::true_type
      {
      };

      /**
      * @brief Specialization to treat std::pair<...> as a printable container type.
      */
      template<
         typename FirstType,
         typename SecondType
      >
      struct is_printable_as_container<std::pair<FirstType, SecondType>> : public std::true_type
      {
      };

      /**
      * @brief Specialization to treat std::tuple<...> as a printable container type.
      */
      template<typename... Args>
      struct is_printable_as_container<std::tuple<Args...>> : public std::true_type
      {
      };

      /**
      * @brief Specialization to treat arrays as printable container types.
      */
      template<
         typename ArrayType,
         std::size_t ArraySize
      >
      struct is_printable_as_container<ArrayType[ArraySize]> : public std::true_type
      {
      };

      /**
      * @brief Narrow character array specialization in order to ensure that we print character
      * arrays as string and not as delimiter containers.
      */
      template<std::size_t ArraySize>
      struct is_printable_as_container<char[ArraySize]> : public std::false_type
      {
      };

      /**
      * @brief Wide character array specialization in order to ensure that we print character arrays
      * as string and not as delimiter containers.
      */
      template<std::size_t ArraySize>
      struct is_printable_as_container<wchar_t[ArraySize]> : public std::false_type
      {
      };

      /**
      * @brief String specialization in order to ensure that we treat strings as nothing more than
      * strings.
      */
      template<
         typename CharacterType,
         typename CharacterTraitsType,
         typename AllocatorType
      >
      struct is_printable_as_container<
         std::basic_string<
            CharacterType,
            CharacterTraitsType,
            AllocatorType
         >
      > : public std::false_type
      {
      };

      /**
      * @brief Helper variable template.
      */
      template<typename Type>
      constexpr bool is_printable_as_container_v = is_printable_as_container<Type>::value;
   }

   namespace Decorator
   {
      /**
      * @brief Struct to neatly wrap up all the additional characters we'll need in order to
      * print out the containers.
      */
      template<typename CharacterType>
      struct wrapper
      {
         using type = CharacterType;

         const type* prefix;
         const type* separator;
         const type* suffix;
      };

      /**
      * @brief Additional wrapper around the `delimiter_values` struct for added convenience.
      */
      template<
         typename /*ContainerType*/,
         typename CharacterType
      >
      struct delimiters
      {
         using type = wrapper<CharacterType>;

         static const type values;
      };

      /**
      * @brief Default narrow delimiters for any container type that isn't specialized.
      */
      template<typename ContainerType>
      struct delimiters<ContainerType, char>
      {
         using type = wrapper<char>;

         static constexpr type values = { "[", ", ", "]" };
      };

      /**
      * @brief Default wide delimiters for any container type that isn't specialized.
      */
      template<typename ContainerType>
      struct delimiters<ContainerType, wchar_t>
      {
         using type = wrapper<wchar_t>;

         static constexpr type values = { L"[", L", ", L"]" };
      };

      /**
      * @brief Narrow character specialization for std::set<...>.
      */
      template<
         typename DataType,
         typename ComparatorType,
         typename AllocatorType
      >
      struct delimiters<std::set<DataType, ComparatorType, AllocatorType>, char>
      {
         static constexpr wrapper<char> values = { "{", ", ", "}" };
      };

      /**
      * @brief Wide character specialization for std::set<...>.
      */
      template<
         typename DataType,
         typename ComparatorType,
         typename AllocatorType
      >
      struct delimiters<std::set<DataType, ComparatorType, AllocatorType>, wchar_t>
      {
         static constexpr wrapper<wchar_t> values = { L"{", L", ", L"}" };
      };

      /**
      * @brief Narrow character specialization for std::multiset<...>.
      */
      template<
         typename DataType,
         typename ComparatorType,
         typename AllocatorType
      >
      struct delimiters<std::multiset<DataType, ComparatorType, AllocatorType>, char>
      {
         static constexpr wrapper<char> values = { "{", ", ", "}" };
      };

      /**
      * @brief Wide character specialization for std::multiset<...>.
      */
      template<
         typename DataType,
         typename ComparatorType,
         typename AllocatorType
      >
      struct delimiters<std::multiset<DataType, ComparatorType, AllocatorType>, wchar_t>
      {
         static constexpr wrapper<wchar_t> values = { L"{", L", ", L"}" };
      };

      /**
      * @brief Narrow character specialization for std::pair<...>.
      */
      template<
         typename FirstType,
         typename SecondType
      >
      struct delimiters<std::pair<FirstType, SecondType>, char>
      {
         static constexpr wrapper<char> values = { "(", ", ", ")" };
      };

      /**
      * @brief Wide character specialization for std::pair<...>.
      */
      template<
         typename FirstType,
         typename SecondType
      >
      struct delimiters<std::pair<FirstType, SecondType>, wchar_t>
      {
         static constexpr wrapper<wchar_t> values = { L"(", L", ", L")" };
      };

      /**
      * @brief Narrow character specialization for std::tuple<...>.
      */
      template<typename... DataType>
      struct delimiters<std::tuple<DataType...>, char>
      {
         static constexpr wrapper<char> values = { "<", ", ", ">" };
      };

      /**
      * @brief Wide character specialization for std::tuple<...>.
      */
      template<typename... DataType>
      struct delimiters<std::tuple<DataType...>, wchar_t>
      {
         static constexpr wrapper<wchar_t> values = { L"<", L", ", L">" };
      };
   }

   /**
   * @brief Default container formatter that will be used to print prefix, element, separator, and
   * suffix strings to an output stream.
   */
   template<
      typename ContainerType,
      typename StreamType
   >
   struct default_formatter
   {
      static constexpr auto decorators =
         ContainerPrinter::Decorator::delimiters<ContainerType, StreamType::char_type>::values;

      static void print_prefix(StreamType& stream) noexcept
      {
         stream << decorators.prefix;
      }

      template<typename ElementType>
      static void print_element(
         StreamType& stream,
         const ElementType& element) noexcept
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
   template<typename ContainerType>
   inline bool is_empty(const ContainerType& container) noexcept
   {
      return container.empty();
   };

   /**
   * @brief Helper function to test arrays for emptiness.
   */
   template<
      typename ArrayType,
      std::size_t ArraySize
   >
   constexpr bool is_empty(const ArrayType(&)[ArraySize]) noexcept
   {
      return !static_cast<bool>(ArraySize);
   }

   /**
   * @brief Recursive tuple handler struct meant to unpack and print std::tuple<...> elements.
   */
   template<
      typename TupleType,
      std::size_t Index,
      std::size_t Last
   >
   struct tuple_handler
   {
      template<
         typename StreamType,
         typename FormatterType
      >
      inline static void print(
         StreamType& stream,
         const TupleType& container,
         FormatterType& formatter)
      {
         stream << std::get<Index>(container);
         formatter.print_delimiter(stream);
         tuple_handler<TupleType, Index + 1, Last>::print(stream, container, formatter);
      }
   };

   /**
   * @brief Specialization of tuple handler to handle empty std::tuple<...> objects.
   *
   * @note The use of "-1" as the ending index here is a bit of a hack. Since std::size_t is, by
   * definition, unsigned, this will evaluate to std::numeric_limits<std::size_t>::max().
   */
   template<typename TupleType>
   struct tuple_handler<TupleType, 0, -1>
   {
      template<
         typename StreamType,
         typename FormatterType
      >
      inline static void print(
         StreamType& /*stream*/,
         const TupleType& /*tuple*/,
         FormatterType& /*formatter*/) noexcept
      {
      };
   };

   /**
   * @brief Specialization of tuple handler for handle the last element in the std::tuple<...>
   * object.
   */
   template<
      typename TupleType,
      std::size_t Index
   >
   struct tuple_handler<TupleType, Index, Index>
   {
      template<
         typename StreamType,
         typename FormatterType
      >
      inline static void print(
         StreamType& stream,
         const TupleType& tuple,
         FormatterType& /*formatter*/) noexcept
      {
         stream << std::get<Index>(tuple);
      }
   };

   /**
   * @brief Overload meant to handle std::tuple<...> objects.
   *
   * @todo Look into using fold expressions once C++17 arrives.
   */
   template<
      typename StreamType,
      typename FormatterType,
      typename... TupleArgs
   >
   static StreamType& ToStream(
      StreamType& stream,
      const std::tuple<TupleArgs...>& container,
      FormatterType& formatter)
   {
      using ContainerType = std::decay_t<decltype(container)>;

      formatter.print_prefix(stream);
      tuple_handler<ContainerType, 0, sizeof...(TupleArgs) - 1>::print(stream, container, formatter);
      formatter.print_suffix(stream);

      return stream;
   }

   /**
   * @brief Overload meant to handle std::pair<...> objects.
   */
   template<
      typename FirstType,
      typename SecondType,
      typename StreamType,
      typename FormatterType
   >
   static StreamType& ToStream(
      StreamType& stream,
      const std::pair<FirstType, SecondType>& container,
      FormatterType& formatter)
   {
      formatter.print_prefix(stream);
      formatter.print_element(stream, container.first);
      formatter.print_delimiter(stream);
      formatter.print_element(stream, container.second);
      formatter.print_suffix(stream);

      return stream;
   }

   /**
   * @brief Overload meant to handle containers that support the notion of "emptiness".
   */
   template<
      typename ContainerType,
      typename StreamType,
      typename FormatterType
   >
   static StreamType& ToStream(
      StreamType& stream,
      const ContainerType& container,
      FormatterType& formatter)
   {
      formatter.print_prefix(stream);

      if (is_empty(container))
      {
         formatter.print_suffix(stream);

         return stream;
      }

      auto begin = std::begin(container);
      formatter.print_element(stream, *begin);

      std::advance(begin, 1);

      std::for_each(begin, std::end(container),
         [&stream, &formatter](const auto& element)
      {
         formatter.print_delimiter(stream);
         formatter.print_element(stream, element);
      });

      formatter.print_suffix(stream);

      return stream;
   }
}

/**
* @brief Overload of the stream output operator for compatible containers.
*/
template<
   typename ContainerType,
   typename StreamType,
   typename = std::enable_if_t<ContainerPrinter::Traits::is_printable_as_container_v<ContainerType>>
>
auto& operator<<(
   StreamType& stream,
   const ContainerType& container)
{
   using FormatterType = ContainerPrinter::default_formatter<ContainerType, StreamType>;
   ContainerPrinter::ToStream(stream, container, FormatterType{ });

   return stream;
}
