#pragma once

#include <cstddef>
#include <iostream>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace Traits
{
   /**
   * @brief P.F.M
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
      typename TraitsType,
      typename AllocatorType
   >
   struct is_printable_as_container<
      std::basic_string<
      CharacterType,
         TraitsType,
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

namespace Printer
{
   /**
   * @brief Struct to neatly wrap up all the additional characters we'll need in order to
   * print out the containers.
   */
   template<typename DelimiterType>
   struct delimiter_values
   {
      using type = DelimiterType;

      const type* prefix;
      const type* separator;
      const type* suffix;
   };

   /**
   * @brief Additional wrapper around the `delimiter_values` struct for added convenience.
   */
   template<
      typename /*ContainerType*/,
      typename DelimiterType
   >
   struct delimiters
   {
      using type = delimiter_values<DelimiterType>;

      static const type values;
   };

   /**
   * @brief Default narrow delimiters for any container type that isn't specialized.
   */
   template<typename ContainerType>
   struct delimiters<ContainerType, char>
   {
      using type = delimiter_values<char>;

      static constexpr type values = { "[", ", ", "]" };
   };

   /**
   * @brief Default wide delimiters for any container type that isn't specialized.
   */
   template<typename ContainerType>
   struct delimiters<ContainerType, wchar_t>
   {
      using type = delimiter_values<wchar_t>;

      static constexpr type values = { L"[", L", ", L"]" };
   };

   /**
   * @brief Narrow character specialization for std::set<...>.
   */
   template<
      typename Type,
      typename ComparatorType,
      typename AllocatorType
   >
   struct delimiters<std::set<Type, ComparatorType, AllocatorType>, char>
   {
      static constexpr delimiter_values<char> values = { "{", ", ", "}" };
   };

   /**
   * @brief Wide character specialization for std::set<...>.
   */
   template<
      typename Type,
      typename ComparatorType,
      typename AllocatorType
   >
   struct delimiters<std::set<Type, ComparatorType, AllocatorType>, wchar_t>
   {
      static constexpr delimiter_values<wchar_t> values = { L"{", L", ", L"}" };
   };

   /**
   * @brief Narrow character specialization for std::set<...>.
   */
   template<
      typename Type,
      typename ComparatorType,
      typename AllocatorType
   >
      struct delimiters<std::multiset<Type, ComparatorType, AllocatorType>, char>
   {
      static constexpr delimiter_values<char> values = { "{", ", ", "}" };
   };

   /**
   * @brief Wide character specialization for std::set<...>.
   */
   template<
      typename Type,
      typename ComparatorType,
      typename AllocatorType
   >
      struct delimiters<std::multiset<Type, ComparatorType, AllocatorType>, wchar_t>
   {
      static constexpr delimiter_values<wchar_t> values = { L"{", L", ", L"}" };
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
      static constexpr delimiter_values<char> values = { "(", ", ", ")" };
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
      static constexpr delimiter_values<wchar_t> values = { L"(", L", ", L")" };
   };

   /**
   * @brief Narrow character specialization for std::tuple<...>.
   */
   template<typename... Types>
   struct delimiters<std::tuple<Types...>, char>
   {
      static constexpr delimiter_values<char> values = { "<", ", ", ">" };
   };

   /**
   * @brief Wide character specialization for std::tuple<...>.
   */
   template<typename... Types>
   struct delimiters<std::tuple<Types...>, wchar_t>
   {
      static constexpr delimiter_values<wchar_t> values = { L"<", L", ", L">" };
   };

   /**
   * @brief Recursive tuple helper template to print std::tuple<...> objects.
   */
   template<
      typename TupleType,
      std::size_t N
   >
   struct TuplePrinter
   {
      template<
         typename CharacterType,
         typename TraitsType,
         typename DelimiterValues
      >
      inline static void Print(
         std::basic_ostream<CharacterType, TraitsType>& stream,
         const TupleType& container,
         const DelimiterValues& delimiters)
      {
         TuplePrinter<TupleType, N - 1>::Print(stream, container, delimiters);

         stream
            << delimiters.separator
            << std::get<N - 1>(container);
      }
   };

   /**
   * @brief Specialization of tuple helper to handle empty std::tuple<...> objects.
   */
   template<typename TupleType>
   struct TuplePrinter<TupleType, 0>
   {
      template<
         typename CharacterType,
         typename TraitsType,
         typename DelimiterValues
      >
         inline static void Print(
            std::basic_ostream<CharacterType, TraitsType>& /*stream*/,
            const TupleType& /*tuple*/,
            const DelimiterValues& /*delimiters*/)
      {
      };
   };

   /**
   * @brief Specialization of tuple helper for the first index of a std::tuple<...>
   */
   template<typename TupleType>
   struct TuplePrinter<TupleType, 1>
   {
      template<
         typename CharacterType,
         typename TraitsType,
         typename DelimiterValues
      >
      inline static void Print(
         std::basic_ostream<CharacterType, TraitsType>& stream,
         const TupleType& tuple,
         const DelimiterValues& /*delimiters*/)
      {
         stream << std::get<0>(tuple);
      }
   };

   /**
   * @brief Specialization for non-zero indices of a std::tuple<...>.
   */
   template<
      typename CharacterType,
      typename TraitsType,
      typename... TupleArgs
   >
   inline static void PrintingHelper(
      std::basic_ostream<CharacterType, TraitsType>& stream,
      const std::tuple<TupleArgs...>& container)
   {
      static constexpr auto delimiters =
         Printer::delimiters<
            std::decay_t<decltype(container)>,
            CharacterType
         >::values;

      stream << delimiters.prefix;
      TuplePrinter<decltype(container), sizeof...(TupleArgs)>::Print(stream, container, delimiters);
      stream << delimiters.suffix;
   }

   /**
   * @brief Helper function to determine if a container is empty.
   */
   template<typename ContainerType>
   inline bool IsEmpty(const ContainerType& container)
   {
      return container.empty();
   };

   /**
   * @brief Helper function to be selected for non-empty arrays.
   */
   template<
      typename ArrayType,
      std::size_t ArraySize
   >
   constexpr bool IsEmpty(const ArrayType(&)[ArraySize])
   {
      return false;
   }

   /**
   * @brief Helper function to be selected for empty arrays.
   */
   template<
      typename ArrayType,
      std::size_t /*ArraySize*/
   >
   constexpr bool IsEmpty(const ArrayType(&)[0])
   {
      return true;
   }

   /**
   * @brief Printing specialization suitable for most container types.
   */
   template<
      typename ContainerType,
      typename CharacterType,
      typename TraitsType
   >
   inline static void PrintingHelper(
      std::basic_ostream<CharacterType, TraitsType>& stream,
      const ContainerType& container)
   {
      static constexpr auto delimiters = Printer::delimiters<ContainerType, CharacterType>::values;

      if (IsEmpty(container))
      {
         stream
            << delimiters.prefix
            << delimiters.suffix;

         return;
      }

      auto begin = std::begin(container);
      stream << delimiters.prefix << *begin;
      std::advance(begin, 1);

      std::for_each(begin, std::end(container),
         [&stream] (const auto& value)
      {
         stream << delimiters.separator << value;
      });

      stream << delimiters.suffix;
   }

   /**
   * @brief Printing specialization for std::pair<...>.
   */
   template<
      typename FirstType,
      typename SecondType,
      typename CharacterType,
      typename TraitsType
   >
   inline static void PrintingHelper(
      std::basic_ostream<CharacterType, TraitsType>& stream,
      const std::pair<FirstType, SecondType>& container)
   {
      static constexpr auto delimiters =
         Printer::delimiters<
            std::decay_t<decltype(container)>,
            CharacterType
         >::values;

      stream
         << delimiters.prefix
         << container.first
         << delimiters.separator
         << container.second
         << delimiters.suffix;
   }

   /**
   * @brief Main class that will print out the container with the help of various templated
   * helper functions.
   */
   template<
      typename ContainerType,
      typename CharacterType,
      typename TraitsType
   >
   class ContainerPrinter
   {
   public:
      ContainerPrinter(const ContainerType& container)
         : m_container{ container }
      {
      }

      inline void PrintTo(std::basic_ostream<CharacterType, TraitsType>& stream) const
      {
         PrintingHelper(stream, m_container);
      }

   private:
      const ContainerType& m_container;
   };
}

/**
* @brief Overload of the stream output operator for compatible containers.
*/
template<
   typename ContainerType,
   typename CharacterType,
   typename TraitsType,
   typename = std::enable_if_t<Traits::is_printable_as_container_v<ContainerType>>
>
auto& operator<<(
   std::basic_ostream<CharacterType, TraitsType>& stream,
   const ContainerType& container)
{
   Printer::ContainerPrinter<ContainerType, CharacterType, TraitsType>(container).PrintTo(stream);

   return stream;
}
