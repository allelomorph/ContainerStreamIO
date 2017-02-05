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
         struct is_printable_as_container : std::false_type
      {
      };

      /**
      * @brief Specialization to ensure that Standard Library compatible containers that have
      * `begin()`, `end()`, and `empty()` member functions, as well as the `value_type` and
      * `iterator` typedefs, are treated as printable containers.
      */
      template<typename Type>
      struct is_printable_as_container<
         Type,
         void_t<
            decltype(std::declval<Type&>().begin()),
            decltype(std::declval<Type&>().end()),
            decltype(std::declval<Type&>().empty()),
            typename Type::value_type,
            typename Type::iterator
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
      template<typename... DataTypes>
      struct is_printable_as_container<std::tuple<DataTypes...>> : public std::true_type
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
      * @brief Narrow character array specialization in order to ensure that we print character arrays
      * as string and not as delimiter containers.
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
      * @brief String specialization in order to ensure that we treat strings as nothing more than strings.
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

   namespace Formatter
   {
      /**
      * @brief
      */
      template<
         typename ContainerType,
         typename CharacterType,
         typename CharacterTraitsType
      >
      struct default_formatter
      {
         using StreamType = std::basic_ostream<CharacterType, CharacterTraitsType>;

         static constexpr auto decorators =
            ContainerPrinter::Decorator::delimiters<ContainerType, CharacterType>::values;

         static void print_prefix(StreamType& stream)
         {
            stream << decorators.prefix;
         }

         template<typename ElementType>
         static void print_element(
            StreamType& stream,
            const ElementType& element)
         {
            stream << element;
         }

         static void print_delimiter(StreamType& stream)
         {
            stream << decorators.separator;
         }

         static void print_suffix(StreamType& stream)
         {
            stream << decorators.suffix;
         }
      };
   }

   /**
   * @brief Helper function to determine if a container is empty.
   */
   template<typename ContainerType>
   inline bool is_empty(const ContainerType& container)
   {
      return container.empty();
   };

   /**
   * @brief Helper function to be selected for arrays.
   */
   template<
      typename ArrayType,
      std::size_t ArraySize
   >
      constexpr bool is_empty(const ArrayType(&)[ArraySize])
   {
      return static_cast<bool>(ArraySize);
   }

   /**
   * @brief Recursive tuple helper template to print std::tuple<...> objects.
   */
   template<
      typename TupleType,
      std::size_t Index
   >
   struct tuple_handler
   {
      template<
         typename CharacterType,
         typename CharacterTraitsType
      >
      inline static void print(
         std::basic_ostream<CharacterType, CharacterTraitsType>& stream,
         const TupleType& container)
      {
         tuple_handler<TupleType, Index - 1>::print(stream, container);

         static constexpr auto decorators =
            ContainerPrinter::Decorator::delimiters<TupleType, CharacterType>::values;

         stream
            << decorators.separator
            << std::get<Index - 1>(container);
      }
   };

   /**
   * @brief Specialization of tuple helper to handle empty std::tuple<...> objects.
   */
   template<typename TupleType>
   struct tuple_handler<TupleType, 0>
   {
      template<
         typename CharacterType,
         typename CharacterTraitsType
      >
      inline static void print(
         std::basic_ostream<CharacterType, CharacterTraitsType>& /*stream*/,
         const TupleType& /*tuple*/)
      {
      };
   };


   /**
   * @brief Specialization of tuple helper for the first index of a std::tuple<...>
   */
   template<typename TupleType>
   struct tuple_handler<TupleType, 1>
   {
      template<
         typename CharacterType,
         typename CharacterTraitsType
      >
         inline static void print(
            std::basic_ostream<CharacterType, CharacterTraitsType>& stream,
            const TupleType& tuple)
      {
         stream << std::get<0>(tuple);
      }
   };

   /**
   * @brief
   */
   template<
      typename CharacterType,
      typename CharacterTraitsType,
      typename... TupleArgs
   >
   static void Emit(
      std::basic_ostream<CharacterType, CharacterTraitsType>& stream,
      const std::tuple<TupleArgs...>& container)
   {
      using ContainerType = std::decay_t<decltype(container)>;
      using Formatter = Formatter::default_formatter<ContainerType, CharacterType, CharacterTraitsType>;

      Formatter::print_prefix(stream);
      tuple_handler<ContainerType, sizeof...(TupleArgs)>::print(stream, container);
      Formatter::print_suffix(stream);
   }

   /**
   * @brief
   */
   template<
      typename FirstType,
      typename SecondType,
      typename CharacterType,
      typename CharacterTraitsType,
      typename FormatterType = Formatter::default_formatter<
         std::pair<FirstType, SecondType>, 
         CharacterType, 
         CharacterTraitsType
      >
   >
   static void Emit(
      std::basic_ostream<CharacterType, CharacterTraitsType>& stream,
      const std::pair<FirstType, SecondType>& container)
   {
      using ContainerType = std::decay_t<decltype(container)>;
      using Formatter = Formatter::default_formatter<ContainerType, CharacterType, CharacterTraitsType>;

      Formatter::print_prefix(stream);
      Formatter::print_element(stream, container.first);
      Formatter::print_delimiter(stream);
      Formatter::print_element(stream, container.second);
      Formatter::print_suffix(stream);
   }

   /**
   * @brief
   */
   template<
      typename ContainerType,
      typename CharacterType,
      typename CharacterTraitsType,
      typename FormatterType = Formatter::default_formatter<ContainerType, CharacterType, CharacterTraitsType>
   >
   static void Emit(
      std::basic_ostream<CharacterType, CharacterTraitsType>& stream,
      const ContainerType& container)
   {
      using Formatter = FormatterType;

      Formatter::print_prefix(stream);
      
      if (is_empty(container))
      {
         Formatter::print_suffix(stream);
         return;
      }

      auto begin = std::begin(container);
      Formatter::print_element(stream, *begin);

      std::advance(begin, 1);

      std::for_each(begin, std::end(container),
         [&stream] (const auto& element)
      {
         Formatter::print_delimiter(stream);
         Formatter::print_element(stream, element);
      });

      Formatter::print_suffix(stream);
   }
}

/**
* @brief Overload of the stream output operator for compatible containers.
*/
template<
   typename ContainerType,
   typename CharacterType,
   typename CharacterTraitsType,
   typename = std::enable_if_t<ContainerPrinter::Traits::is_printable_as_container_v<ContainerType>>
>
auto& operator<<(
   std::basic_ostream<CharacterType, CharacterTraitsType>& stream,
   const ContainerType& container)
{
   ContainerPrinter::Emit(stream, container);

   return stream;
}
