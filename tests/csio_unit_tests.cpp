// requires CATCH_CONFIG_MAIN to be defined in separate translation unit that
//   also includes catch.hpp
#include "catch.hpp"

#include "container_stream_io.hh"

#include <algorithm>
#include <functional>

#include <array>
#include <vector>
#include <tuple>
#include <deque>
#include <forward_list>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <stack>
#include <queue>

#include <sstream>

#include "TypeName.hh"  // testing

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
{};

template <typename Type>
class stack_wrapper : public std::stack<Type>
{};

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

template <typename CStrTypeA, typename CStrTypeB>
auto idiomatic_strcmp(const CStrTypeA s1, const CStrTypeB s2
    ) -> std::enable_if_t<
        std::is_pointer<typename std::decay<CStrTypeA>::type>::value &&
        std::is_pointer<typename std::decay<CStrTypeB>::type>::value,
        bool>
{
    using char_type = typename std::remove_const<
        typename std::pointer_traits<
        typename std::decay<CStrTypeB>::type >::element_type >::type;
#if __cplusplus >= 201703L
    return s1 == std::basic_string_view<char_type>(s2);
#else
    return s1 == std::basic_string<char_type>(s2);
#endif
}

using namespace container_stream_io::strings::compile_time;  // string_literal

// TBD these do not affect iword/pword, and so not quoted/literal default
//   - could use `(o|i)ss.copyfmt(std::basic_(o|i)stringstream<CharType>{});`,
//   but at that point why not just `(o|i)ss = std::basic_(o|i)stringstream<CharType>{};`?
// TBD consider another reason to remove these altogether - the intended use of
//   Catch2 TEST_CASE and SECTION (which can be nested) is to share setup:
//   - https://github.com/catchorg/Catch2/blob/devel/docs/tutorial.md#test-cases-and-sections
//   (tradeoff is that every nested section must be named and bracketed, which
//   will make this file much longer and potentially less readable...)
template<typename CharType>
void reset_ostringstream(std::basic_ostringstream<CharType>& oss)
{
    static const auto default_flags {
        std::basic_ostringstream<CharType>{}.flags() };
    oss.str(STRING_LITERAL(CharType, ""));
    oss.clear();
    oss.flags(default_flags);
}

template<typename CharType>
void reset_istringstream(
    std::basic_istringstream<CharType>& iss,
    const CharType* s = STRING_LITERAL(CharType, ""))
{
    static const auto default_flags {
        std::basic_istringstream<CharType>{}.flags() };
    iss.str(s);
    iss.clear();
    iss.flags(default_flags);
}

template<typename CharType>
void reset_istringstream(
    std::basic_istringstream<CharType>& iss, const std::basic_string<CharType>& s)
{
    static const auto default_flags {
        std::basic_istringstream<CharType>{}.flags() };
    iss.str(s);
    iss.clear();
    iss.flags(default_flags);
}

} // namespace

using namespace container_stream_io;
/*
TEST_CASE("Traits: detect parseable (input stream extractable) container types",
          "[traits][input]")
{

    SECTION("Detect most STL containers as being parseable container types")
    {
        REQUIRE(traits::is_parseable_as_container<std::array<int, 5>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::vector<int>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::pair<int, double>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::tuple<int, double, float>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::deque<int>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::forward_list<int>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::list<int>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::set<int>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::multiset<int>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::map<int, float>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::multimap<int, float>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::unordered_set<int>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::unordered_multiset<int>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::unordered_map<int, float>>::value == true);
        REQUIRE(traits::is_parseable_as_container<std::unordered_multimap<int, float>>::value == true);
    }

    SECTION("Detect C-like array of non-char type as being a parseable container type")
    {
        REQUIRE(traits::is_parseable_as_container<int[5]>::value == true);
    }

    SECTION("Detect inherited parseable container type.")
    {
        REQUIRE(traits::is_parseable_as_container<vector_wrapper<int>>::value == true);
    }
}

TEST_CASE("Traits: detect types not parseable (input stream extractable) as containers",
          "[traits][input]")
{
    SECTION("Detect C-like array of char type as not being a parseable container type")
    {
        REQUIRE(traits::is_parseable_as_container<char[5]>::value == false);
    }

    SECTION("Detect STL strings as not being parseable container types")
    {
        REQUIRE(traits::is_parseable_as_container<std::basic_string<char>>::value == false);
#if __cplusplus >= 201703L
        REQUIRE(traits::is_parseable_as_container<std::basic_string_view<char>>::value == false);
#endif
    }

    SECTION("Detect STL stacks and single-ended queues as not being parseable container types")
    {
        REQUIRE(traits::is_parseable_as_container<std::stack<int>>::value == false);
        REQUIRE(traits::is_parseable_as_container<std::queue<int>>::value == false);
        REQUIRE(traits::is_parseable_as_container<std::priority_queue<int>>::value == false);
    }

    SECTION("Detect inherited unparseable container type.")
    {
        REQUIRE(traits::is_parseable_as_container<stack_wrapper<int>>::value == false);
    }
}

TEST_CASE("Traits: detect printable (output stream insertable) container types",
          "[traits][output]")
{
    SECTION("Detect most STL containers as being printable container types")
    {
        REQUIRE(traits::is_printable_as_container<std::array<int, 5>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::vector<int>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::pair<int, double>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::tuple<int, double, float>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::deque<int>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::forward_list<int>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::list<int>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::set<int>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::multiset<int>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::map<int, float>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::multimap<int, float>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::unordered_set<int>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::unordered_multiset<int>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::unordered_map<int, float>>::value == true);
        REQUIRE(traits::is_printable_as_container<std::unordered_multimap<int, float>>::value == true);
    }

    SECTION("Detect C-like array of non-char type as being a printable container type")
    {
        REQUIRE(traits::is_printable_as_container<int[5]>::value == true);
    }

    SECTION("Detect inherited printable container type.")
    {
        REQUIRE(traits::is_printable_as_container<vector_wrapper<int>>::value == true);
    }
}

TEST_CASE("Traits: detect types not printable (output stream insertable) as containers",
          "[traits][output]")
{
    SECTION("Detect C-like array of char type as not being a printable container type")
    {
        REQUIRE(traits::is_printable_as_container<char[5]>::value == false);
    }

    SECTION("Detect STL strings as not being printable container types")
    {
        REQUIRE(traits::is_printable_as_container<std::basic_string<char>>::value == false);
#if __cplusplus >= 201703L
        REQUIRE(traits::is_printable_as_container<std::basic_string_view<char>>::value == false);
#endif
    }

    SECTION("Detect STL stacks and single-ended queues as not being printable container types")
    {
        REQUIRE(traits::is_printable_as_container<std::stack<int>>::value == false);
        REQUIRE(traits::is_printable_as_container<std::queue<int>>::value == false);
        REQUIRE(traits::is_printable_as_container<std::priority_queue<int>>::value == false);
    }

    SECTION("Detect inherited unprintable container type")
    {
        REQUIRE(traits::is_printable_as_container<stack_wrapper<int>>::value == false);
    }
}

TEST_CASE("Traits: detect char types", "[traits]")
{
    REQUIRE(traits::is_char_variant<int>::value == false);
    REQUIRE(traits::is_char_variant<std::vector<int>>::value == false);

    REQUIRE(traits::is_char_variant<char>::value == true);
    REQUIRE(traits::is_char_variant<wchar_t>::value == true);
#if __cplusplus > 201703L
    REQUIRE(traits::is_char_variant<char8_t>::value == true);
#endif
    REQUIRE(traits::is_char_variant<char16_t>::value == true);
    REQUIRE(traits::is_char_variant<char32_t>::value == true);
}

TEST_CASE("Traits: detect string types", "[traits]")
{
    SECTION("Detect non-char types and C arrays as not being string types")
    {
        REQUIRE(traits::is_string_variant<int>::value == false);
        REQUIRE(traits::is_string_variant<int[5]>::value == false);
    }

    SECTION("Fail to detect char type C arrays as strings (expecting their decay to pointers)")
    {
        REQUIRE(traits::is_string_variant<char[5]>::value == false);
        REQUIRE(traits::is_string_variant<const char[5]>::value == false);
    }

    SECTION("Detect char pointers and STL strings as strings")
    {
        REQUIRE(traits::is_string_variant<char*>::value == true);
        REQUIRE(traits::is_string_variant<const char*>::value == true);
        REQUIRE(traits::is_string_variant<std::basic_string<char>>::value == true);
        REQUIRE(traits::is_string_variant<const std::basic_string<char>>::value == true);
#if __cplusplus >= 201703L
        REQUIRE(traits::is_string_variant<std::basic_string_view<char>>::value == true);
        REQUIRE(traits::is_string_variant<const std::basic_string_view<char>>::value == true);
#endif
    }
}

TEST_CASE("Traits: detect emplace methods", "[traits]")
{
    REQUIRE(traits::has_iterless_emplace<std::vector<int>>::value == false);
    REQUIRE(traits::has_emplace_back<std::vector<int>>::value == true);

    REQUIRE(traits::has_iterless_emplace<std::set<int>>::value == true);
    REQUIRE(traits::has_emplace_back<std::set<int>>::value == false);

    REQUIRE(traits::has_iterless_emplace<int>::value == false);
    REQUIRE(traits::has_emplace_back<int>::value == false);
}
*/

TEST_CASE("strings::literal() printing/output streaming escaped literals",
          "[literal][strings][output]")
{
    SECTION("supports parameter type",
            "(example parameter char type = char)")
    {
        std::ostringstream oss;

        SECTION("char&")
        {
            char c { 't' };
            oss << strings::literal(c);
            REQUIRE(oss.str() == "'t'");
        }

        SECTION("const char&")
        {
            oss << strings::literal('t');
            REQUIRE(oss.str() == "'t'");
        }

        SECTION("char* (char[])")
        {
            char s[5] { "test" };
            oss << strings::literal(s);
            REQUIRE(oss.str() == "\"test\"");
        }

        SECTION("const char* (const char[])")
        {
            oss << strings::literal("test");
            REQUIRE(oss.str() == "\"test\"");
        }

        SECTION("std::basic_string<char>&")
        {
            std::basic_string<char> s { "test" };
            oss << strings::literal(s);
            REQUIRE(oss.str() == "\"test\"");
        }

        SECTION("const std::basic_string<char>&")
        {
            const std::basic_string<char> s { "test" };
            oss << strings::literal(s);
            REQUIRE(oss.str() == "\"test\"");
        }

#if (__cplusplus >= 201703L)
        SECTION("std::basic_string_view<char>&")
        {
            std::basic_string_view<char> s { "test" };
            oss << strings::literal(s);
            REQUIRE(oss.str() == "\"test\"");
        }

        SECTION("const std::basic_string_view<char>&")
        {
            const std::basic_string_view<char> s { "test" };
            oss << strings::literal(s);
            REQUIRE(oss.str() == "\"test\"");
        }
#endif  // C++17 and above
    }

    SECTION("only uses printable 7-bit ASCII to encode")
    {
        std::ostringstream oss;

        SECTION("printable 7-bit ASCII values")
        {
            oss << strings::literal('t');
            REQUIRE(oss.str() == "'t'");
        }

        SECTION("delimiter character")
        {
            oss << strings::literal('\'');
            REQUIRE(oss.str() == "'\\\''");
        }

        SECTION("escape character")
        {
            oss << strings::literal('\\');
            REQUIRE(oss.str() == "'\\\\'");
        }

        SECTION("unprintable 7-bit ASCII values with standard escape")
        {
            oss << strings::literal('\t');
            REQUIRE(oss.str() == "'\\t'");
        }

        SECTION("unprintable 7-bit ASCII values without standard escape", "(hex escape)")
        {
            oss << strings::literal('\x01');
            REQUIRE(oss.str() == "'\\x01'");
        }

        SECTION("values oustide 7-bit ASCII range", "(hex escape)")
        {
            oss << strings::literal('\x80');
            REQUIRE(oss.str() == "'\\x80'");
        }

        SECTION("EOF value", "(therefore ensuring so setting of eofbit or failbit as a result)")
        {
            oss << strings::literal(char(std::ostringstream::traits_type::eof()));
            REQUIRE(oss.str() == "'\\xff'");
            REQUIRE(oss.good());
        }
    }

    SECTION("allows choosing custom escape and delimiter characters")
    {
        std::ostringstream oss;
        oss << strings::literal('\t', '^', '|');
        REQUIRE(oss.str() == "^|t^");

        SECTION("but enforces encoding by throwing exception for")
        {
            SECTION("unprintable 7-bit ASCII values")
            {
                SECTION("used as delimiter")
                {
                    REQUIRE_THROWS_MATCHES(
                        oss << strings::literal('\t', '\v'),
                        std::invalid_argument,
                        Catch::Message(
                            "literal delim and escape must be printable 7-bit ASCII characters"));
                }

                SECTION("used as escape")
                {
                    REQUIRE_THROWS_MATCHES(
                        oss << strings::literal('\t', '\'', '\v'),
                        std::invalid_argument,
                        Catch::Message(
                            "literal delim and escape must be printable 7-bit ASCII characters"));
                }
            }

            SECTION("values out of 7-bit ASCII range")
            {
                SECTION("used as delimiter")
                {
                    REQUIRE_THROWS_MATCHES(
                        oss << strings::literal('\t', '\x80'),
                        std::invalid_argument,
                        Catch::Message(
                            "literal delim and escape must be printable 7-bit ASCII characters"));
                }

                SECTION("used as escape")
                {
                    REQUIRE_THROWS_MATCHES(
                        oss << strings::literal('\t', '\'', '\x80'),
                        std::invalid_argument,
                        Catch::Message(
                            "literal delim and escape must be printable 7-bit ASCII characters"));
                }
            }
        }
    }

    SECTION("supports differing parameter and stream char type")
    {
        SECTION("by using literal prefixes for")
        {
            std::ostringstream oss;

            SECTION("wchar_t")
            {
                oss << strings::literal(L"test");
                REQUIRE(oss.str() == "L\"test\"");
            }

#if (__cplusplus > 201703L)
            SECTION("char8_t")
            {
                oss << strings::literal(u8"test");
                REQUIRE(oss.str() == "u8\"test\"");
            }

#endif
            SECTION("char16_t")
            {
                oss << strings::literal(u"test");
                REQUIRE(oss.str() == "u\"test\"");
            }

            SECTION("char32_t")
            {
                oss << strings::literal(U"test");
                REQUIRE(oss.str() == "U\"test\"");
            }
        }

        SECTION("where parameter char size > stream char size:")
        {
            std::ostringstream oss;

            SECTION("printable 7-bit ASCII, delim, escape, and standard escapes "
                    "inserted same regardless of char types")
            {
                oss << strings::literal(L"t\\\"\t");
                REQUIRE(oss.str() == "L\"t\\\\\\\"\\t\"");
            }

            SECTION("hex escape width scales to parameter char type width")
            {
                oss << strings::literal(L"\x01\xfe\xfffffffe");
                REQUIRE(oss.str() == "L\"\\x00000001\\x000000fe\\xfffffffe\"");
            }
        }

        SECTION("where parameter char size < stream char size:")
        {
            std::wostringstream woss;

            SECTION("printable 7-bit ASCII, delim, escape, and standard escapes "
                    "inserted same regardless of char types")
            {
                woss << strings::literal("t\\\"\t");
                REQUIRE(woss.str() == L"\"t\\\\\\\"\\t\"");
            }

            SECTION("hex escape width scales to parameter char type width")
            {
                woss << strings::literal("\x01\xfe");
                REQUIRE(woss.str() == L"\"\\x01\\xfe\"");
            }
        }
    }
}

TEST_CASE("strings::literal() parsing/input streaming escaped literals",
          "[literal][strings][input]")
{
    SECTION("supports parameter type",
            "(example parameter char type = char)")
    {
        std::istringstream iss;

        SECTION("char&")
        {
            char c;
            iss.str("'t'");
            iss >> strings::literal(c);
            REQUIRE(c == 't');
        }

        // TBD yet to implement
        // SECTION("char* (char[])",
        //         "(beware stack smashing on static array or segfault on dynamic array overruns)")
        // {
        //     char s[5];
        //     iss.str("\"test\"");
        //     iss >> strings::literal(s);
        //     REQUIRE(std::string(s) == "test");
        // }

        SECTION("std::basic_string<char>&")
        {
            std::basic_string<char> s;
            iss.str("\"test\"");
            iss >> strings::literal(s);
            REQUIRE(s == "test");
        }
    }

    SECTION("only expects printable 7-bit ASCII to decode")
    {
        std::istringstream iss;
        std::string s;

        iss.str("\"t\\\\\\\"\\t\\x01\\xfe\"");
        iss >> strings::literal(s);
        REQUIRE(s == "t\\\"\t\x01\xfe");

        SECTION("setting failbit for invalid encoding:")
        {
            char c;
            iss.clear();

            SECTION("unprintable 7-bit ASCII values with standard escape")
            {
                iss.str("'\t'");
                iss >> strings::literal(c);
                REQUIRE(iss.fail());
            }

            SECTION("unprintable 7-bit ASCII values without standard escape")
            {
                iss.str("'\x01'");
                iss >> strings::literal(c);
                REQUIRE(iss.fail());
            }

            SECTION("values oustide 7-bit ASCII range")
            {
                iss.str("'\x80'");
                iss >> strings::literal(c);
                REQUIRE(iss.fail());
            }
        }

        SECTION("eof value extracted from malformed encoding sets failbit",
                "(not eofbit as expected, may depend on stream char type)")
        {
            iss.clear();

            SECTION("EOF")
            {
                char c;
                char s[] { "'_'" };
                s[1] = std::istringstream::traits_type::eof();
                iss.str(s);
                iss >> strings::literal(c);
                REQUIRE((iss.fail() && !iss.eof()));
            }

            SECTION("WEOF")
            {
                wchar_t wc;
                wchar_t ws[] { L"'_'" };
                ws[1] = std::wistringstream::traits_type::eof();
                std::wistringstream wiss { ws };
                wiss >> strings::literal(wc);
                REQUIRE((wiss.fail() && !wiss.eof()));
            }
        }
    }

    SECTION("allows choosing custom escape and delimiter characters")
    {
        std::istringstream iss;
        char c;

        iss.str("^|t^");
        iss >> strings::literal(c, '^', '|');
        REQUIRE(c == '\t');

        SECTION("but enforces encoding by throwing exception for")
        {
            SECTION("unprintable 7-bit ASCII values")
            {
                SECTION("used as delimiter")
                {
                    REQUIRE_THROWS_MATCHES(
                        iss >> strings::literal(c, '\v'),
                        std::invalid_argument,
                        Catch::Message(
                            "literal delim and escape must be printable 7-bit ASCII characters"));
                }

                SECTION("used as escape")
                {
                    REQUIRE_THROWS_MATCHES(
                        iss >> strings::literal(c, '\'', '\v'),
                        std::invalid_argument,
                        Catch::Message(
                            "literal delim and escape must be printable 7-bit ASCII characters"));
                }
            }

            SECTION("values out of 7-bit ASCII range")
            {
                SECTION("used as delimiter")
                {
                    REQUIRE_THROWS_MATCHES(
                        iss >> strings::literal(c, '\x80'),
                        std::invalid_argument,
                        Catch::Message(
                            "literal delim and escape must be printable 7-bit ASCII characters"));
                }

                SECTION("used as escape")
                {
                    REQUIRE_THROWS_MATCHES(
                        iss >> strings::literal(c, '\'', '\x80'),
                        std::invalid_argument,
                        Catch::Message(
                            "literal delim and escape must be printable 7-bit ASCII characters"));
                }
            }
        }
    }

    SECTION("supports differing parameter and stream char type")
    {
        SECTION("by using literal prefixes for")
        {
            std::istringstream iss;

            SECTION("wchar_t")
            {
                std::wstring ws;
                iss.str("L\"test\"");
                iss >> strings::literal(ws);
                REQUIRE(ws == L"test");
            }

#if (__cplusplus > 201703L)
            SECTION("char8_t")
            {
                std::u8string u8s;
                iss.str("u8\"test\"");
                iss >> strings::literal(u8s);
                REQUIRE(u8s == u8"test");
            }

#endif
            SECTION("char16_t")
            {
                std::u16string u16s;
                iss.str("u\"test\"");
                iss >> strings::literal(u16s);
                REQUIRE(u16s == u"test");
            }

            SECTION("char32_t")
            {
                std::u32string u32s;
                iss.str("U\"test\"");
                iss >> strings::literal(u32s);
                REQUIRE(u32s == U"test");
            }
        }

        SECTION("where parameter char size > stream char size:")
        {
            std::istringstream iss;
            std::wstring ws;

            SECTION("printable 7-bit ASCII, delim, escape, and standard escapes "
                    "extracted same regardless of char types")
            {
                iss.str("L\"t\\\\\\\"\\t\"");
                iss >> strings::literal(ws);
                REQUIRE(ws == L"t\\\"\t");
            }

            SECTION("decoding succeeds on hex escape widths scaled to parameter char type width")
            {
                iss.str("L\"\\x00000001\\x000000fe\\xfffffffe\"");
                iss >> strings::literal(ws);
                REQUIRE(ws == L"\x01\xfe\xfffffffe");
            }

            SECTION("failbit is set on hex escape widths not scaled to parameter char type width")
            {
                iss.str("L\"\\x01\\xfe\\xfffffffe\"");
                iss >> strings::literal(ws);
                REQUIRE(iss.fail());
            }
        }

        SECTION("where parameter char size < stream char size:")
        {
            std::wistringstream wiss;
            std::string s;

            SECTION("printable 7-bit ASCII, delim, escape, and standard escapes "
                    "extracted same regardless of char types")
            {
                wiss.str(L"\"t\\\\\\\"\\t\"");
                wiss >> strings::literal(s);
                REQUIRE(s == "t\\\"\t");
            }

            SECTION("decoding of hex escapes scaled to parameter char type width",
                    "(any following hexadecimal characters are parsed as literals)")
            {
                wiss.str(L"\"\\x01\\xfe\\xfffffffe\"");
                wiss >> strings::literal(s);
                const char c_str[] { '\x01', '\xfe', '\xff',
                        'f', 'f', 'f', 'f', 'f', 'e', 0 };
                REQUIRE(s == c_str);
            }
        }
    }
}

TEST_CASE("strings::quoted() printing/output streaming quoted strings",
          "[quoted][strings][output]")
{
    SECTION("supports parameter type",
            "(example parameter char type = char)")
    {
        std::ostringstream oss;

        SECTION("char&")
        {
            char c { 't' };
            oss << strings::quoted(c);
            REQUIRE(oss.str() == "'t'");
        }

        SECTION("const char&")
        {
            oss << strings::quoted('t');
            REQUIRE(oss.str() == "'t'");
        }

        SECTION("char* (char[])")
        {
            char s[5] { "test" };
            oss << strings::quoted(s);
            REQUIRE(oss.str() == "\"test\"");
        }

        SECTION("const char* (const char[])")
        {
            oss << strings::quoted("test");
            REQUIRE(oss.str() == "\"test\"");
        }

        SECTION("std::basic_string<char>&")
        {
            std::basic_string<char> s { "test" };
            oss << strings::quoted(s);
            REQUIRE(oss.str() == "\"test\"");
        }

        SECTION("const std::basic_string<char>&")
        {
            const std::basic_string<char> s { "test" };
            oss << strings::quoted(s);
            REQUIRE(oss.str() == "\"test\"");
        }

#if (__cplusplus >= 201703L)
        SECTION("std::basic_string_view<char>&")
        {
            std::basic_string_view<char> s { "test" };
            oss << strings::quoted(s);
            REQUIRE(oss.str() == "\"test\"");
        }

        SECTION("const std::basic_string_view<char>&")
        {
            const std::basic_string_view<char> s { "test" };
            oss << strings::quoted(s);
            REQUIRE(oss.str() == "\"test\"");
        }
#endif  // C++17 and above
    }

    SECTION("encodes")
    {
        std::ostringstream oss;

        SECTION("delimiter character as escaped")
        {
            oss << strings::quoted('\'');
            REQUIRE(oss.str() == "'\\\''");
        }

        SECTION("escape character as escaped")
        {
            oss << strings::quoted('\\');
            REQUIRE(oss.str() == "'\\\\'");
        }

        SECTION("all characters other than delimiter/escape unmodified")
        {
            oss << strings::quoted("t\t\x01\xfe");
            REQUIRE(oss.str() == "\"t\t\x01\xfe\"");
        }

        SECTION("EOF unmodified",
                "(but in testing this does not set eofbit or failbit)")
        {
            oss << strings::quoted(char(std::ostringstream::traits_type::eof()));
            REQUIRE(oss.str() == "'\xff'");
            REQUIRE(oss.good());

            std::wostringstream woss;
            woss << strings::quoted(wchar_t(std::wostringstream::traits_type::eof()));
            REQUIRE(woss.str() == L"L'\xffffffff'");
            REQUIRE(woss.good());
        }
    }

    SECTION("allows choosing custom escape and delimiter characters of any value, including")
    {
        std::ostringstream oss;

        SECTION("printable 7-bit ASCII")
        {
            oss << strings::quoted('^', '^', '|');
            REQUIRE(oss.str() == "^|^^");
        }

        SECTION("unprintable 7-bit ASCII")
        {
            oss << strings::quoted('\v', '\v', '\b');
            REQUIRE(oss.str() == "\v\b\v\v");
        }

        SECTION("values out of 7-bit ASCII range")
        {
            oss << strings::quoted('\x80', '\x80', '\xfe');
            REQUIRE(oss.str() == "\x80\xfe\x80\x80");
        }
    }

    SECTION("supports differing parameter and stream char type")
    {
        SECTION("by using literal prefixes for")
        {
            std::ostringstream oss;
            std::wostringstream woss;

            SECTION("wchar_t")
            {
                woss << strings::quoted(L"test");
                REQUIRE(woss.str() == L"L\"test\"");
            }

#if (__cplusplus > 201703L)
            SECTION("char8_t")
            {
                oss << strings::quoted(u8"test");
                REQUIRE(oss.str() == "u8\"test\"");
            }

#endif
            SECTION("char16_t")
            {
                woss << strings::quoted(u"test");
                REQUIRE(woss.str() == L"u\"test\"");
            }

            SECTION("char32_t")
            {
                woss << strings::quoted(U"test");
                REQUIRE(woss.str() == L"U\"test\"");
            }
        }

        SECTION("where parameter char size <= stream char size")
        {
            // TBD fix how signed chars convert to unsigned char types?
            //   (currently char(\xfe) (-2) -> wchar_t(\xfffffffe) (-2),
            //    rather than wchar_t(\x000000fe) (254))
            // For reference (in GNU ISO C++20):
            //   std::is_signed<char>: 1
            //   std::is_signed<wchar_t>: 1
            //   std::is_signed<char8_t>: 0
            //   std::is_signed<char16_t>: 0
            //   std::is_signed<char32_t>: 0
            std::wostringstream woss;
            woss << strings::quoted("t\\\"\t\x01\xfe");
            REQUIRE(woss.str() == L"\"t\\\\\\\"\t\x01\xfffffffe\"");
            REQUIRE(woss.good());
        }

        SECTION("unless parameter char size > stream char size",
                "(failbit is set due to possibility of char value overflow)")
        {
            std::ostringstream oss;
            oss << strings::quoted(L"test");
            REQUIRE(oss.fail());
        }
    }
}

TEST_CASE("strings::quoted() parsing/input streaming quoted strings",
          "[quoted][strings][input]")
{
    SECTION("supports parameter type",
            "(example parameter char type = char)")
    {
        std::istringstream iss;

        SECTION("char&")
        {
            char c;
            iss.str("'t'");
            iss >> strings::quoted(c);
            REQUIRE(c == 't');
        }

        // TBD yet to implement
        // SECTION("char* (char[])",
        //         "(beware stack smashing on static array or segfault on dynamic array overruns)")
        // {
        //     char s[5];
        //     iss.str("\"test\"");
        //     iss >> strings::quoted(s);
        //     REQUIRE(std::string(s) == "test");
        // }

        SECTION("std::basic_string<char>&")
        {
            std::basic_string<char> s;
            iss.str("\"test\"");
            iss >> strings::quoted(s);
            REQUIRE(s == "test");
        }
    }

    SECTION("only expects escape and delimiter to be escaped")
    {
        std::istringstream iss;
        std::string s;

        iss.str("\"t\\\\\\\"\t\x01\xfe\"");
        iss >> strings::quoted(s);
        REQUIRE(s == "t\\\"\t\x01\xfe");

        SECTION("setting failbit for invalid encoding:")
        {
            char c;
            iss.clear();

            SECTION("any character other than delimiter and escape escaped")
            {
                iss.str("'\\t'");
                iss >> strings::quoted(c);
                REQUIRE(iss.fail());
            }
        }

        SECTION("eof value extracted from quoted string does not set eofbit as "
                "expected, and only sets failbit depending on stream char type")
        {
            iss.clear();

            SECTION("EOF")
            {
                char c;
                char s[] { '\'', std::istringstream::traits_type::eof(), '\'', 0 };
                iss.str(s);
                iss >> strings::quoted(c);
                REQUIRE(iss.good());
            }

            SECTION("WEOF")
            {
                wchar_t wc;
                wchar_t ws[] { L'\'', wchar_t(std::wistringstream::traits_type::eof()), L'\'', 0 };
                std::wistringstream wiss { ws };
                wiss >> strings::quoted(wc);
                REQUIRE((wiss.fail() && !wiss.eof()));
            }
        }
    }

    SECTION("allows choosing custom escape and delimiter characters of any value:")
    {
        std::istringstream iss;
        char c;

        SECTION("printable 7-bit ASCII values")
        {
            iss.str("^||^");
            iss >> strings::quoted(c, '^', '|');
            REQUIRE(c == '|');
        }

        SECTION("unprintable 7-bit ASCII values")
        {
            iss.str("\v\b\b\v");
            iss >> strings::quoted(c, '\v', '\b');
            REQUIRE(c == '\b');
        }

        SECTION("values out of 7-bit ASCII range")
        {
            iss.str("\x80\x81\x81\x80");
            iss >> strings::quoted(c, '\x80', '\x81');
            REQUIRE(c == '\x81');
        }
    }

    SECTION("supports differing parameter and stream char type")
    {
        SECTION("by using literal prefixes for")
        {
            std::istringstream iss;

            SECTION("wchar_t")
            {
                std::wstring ws;
                iss.str("L\"test\"");
                iss >> strings::quoted(ws);
                REQUIRE(ws == L"test");
            }

#if (__cplusplus > 201703L)
            SECTION("char8_t")
            {
                std::u8string u8s;
                iss.str("u8\"test\"");
                iss >> strings::quoted(u8s);
                REQUIRE(u8s == u8"test");
            }

#endif
            SECTION("char16_t")
            {
                std::u16string u16s;
                iss.str("u\"test\"");
                iss >> strings::quoted(u16s);
                REQUIRE(u16s == u"test");
            }

            SECTION("char32_t")
            {
                std::u32string u32s;
                iss.str("U\"test\"");
                iss >> strings::quoted(u32s);
                REQUIRE(u32s == U"test");
            }
        }

        SECTION("where parameter char size > stream char size:")
        {
            std::istringstream iss;
            std::wstring ws;

            SECTION("delimiter and escape chars escaped, all others extracted directly")
            {
                // TBD fix how signed chars convert to unsigned char types?
                //   (currently char(\x80) (-128) -> wchar_t(\xffffff80) (-128),
                //    rather than wchar_t(\x00000080) (128))
                // For reference (in GNU ISO C++20):
                //   std::is_signed<char>: 1
                //   std::is_signed<wchar_t>: 1
                //   std::is_signed<char8_t>: 0
                //   std::is_signed<char16_t>: 0
                //   std::is_signed<char32_t>: 0
                iss.str("L\"t\\\\\\\"\t\x01\x80\"");
                iss >> strings::quoted(ws);
                REQUIRE(ws == L"t\\\"\t\x01\xffffff80");
            }
        }

        SECTION("but not when parameter char size < stream char size",
                "(prevents potential overflow in char conversions)")
        {
            std::wistringstream wiss;
            std::string s;

            wiss.str(L"\"test\"");
            wiss >> strings::quoted(s);
            REQUIRE(wiss.fail());
        }
    }
}

TEST_CASE("Strings: printing/output streaming string types inside compatible "
          "containers uses strings::literal()/quoted()"
          "[literal][quoted][strings][output]")
{
    SECTION("with element type")
    {
        std::ostringstream oss;

        // note that constness of vector conferred to elements
        SECTION("char&")
        {
            std::vector<char> vc { { 't' } };
            oss << vc;
            REQUIRE(oss.str() == "['t']");
        }

        SECTION("const char&")
        {
            const std::vector<char> vcc { { 't' } };
            oss << vcc;
            REQUIRE(oss.str() == "['t']");
        }

        SECTION("char* (char[])")
        {
            // workaround for not being able to assign string literal to char*
            std::unique_ptr<char[]> up( new char[5] { 't', 'e', 's', 't', 0 } );
            std::vector<char*> vca { { up.get() } };
            oss << vca;
            REQUIRE(oss.str() == "[\"test\"]");
        }

        SECTION("const char* (const char[])")
        {
            const std::vector<const char*> vcca { { "test" } };
            oss << vcca;
            REQUIRE(oss.str() == "[\"test\"]");
        }

        SECTION("std::basic_string<char>&")
        {
            std::vector<std::string> vs { { "test" } };
            oss << vs;
            REQUIRE(oss.str() == "[\"test\"]");
        }

        SECTION("const std::basic_string<char>&")
        {
            const std::vector<std::string> vcs { { "test" } };
            oss << vcs;
            REQUIRE(oss.str() == "[\"test\"]");
        }

#if (__cplusplus >= 201703L)
        SECTION("std::basic_string_view<char>&")
        {
            std::vector<std::string_view> vsv { { "test" } };
            oss << vsv;
            REQUIRE(oss.str() == "[\"test\"]");
        }

        SECTION("const std::basic_string_view<char>&")
        {
            const std::vector<std::string_view> vcsv { { "test" } };
            oss << vcsv;
            REQUIRE(oss.str() == "[\"test\"]");
        }

#endif  // C++17 and above
    }

    SECTION("defaults to strings::literal()")
    {
        std::ostringstream oss;
        std::vector<std::string> vs { { "tes\t" } };
        oss << vs;
        REQUIRE(oss.str() == "[\"tes\\t\"]");
    }

    SECTION("can be set to strings::quoted() with iomanip strings::quotedrepr")
    {
        std::ostringstream oss;
        oss << strings::quotedrepr;
        std::vector<std::string> vs { { "tes\t" } };
        oss << vs;
        REQUIRE(oss.str() == "[\"tes\t\"]");
    }

    SECTION("can be set to strings::literal() with iomanip strings::literalrepr")
    {
        std::ostringstream oss;
        oss << strings::quotedrepr;   // change from default
        oss << strings::literalrepr;  // restores default
        std::vector<std::string> vs { { "tes\t" } };
        oss << vs;
        REQUIRE(oss.str() == "[\"tes\\t\"]");
    }
}

TEST_CASE("Strings: parsing/input streaming string types inside compatible "
          "containers uses strings::literal()/quoted()"
          "[literal][quoted][strings][output]")
{
    SECTION("with element type")
    {
        std::istringstream iss;

        SECTION("char&")
        {
            std::vector<char> vc;
            iss.str("['t']");
            iss >> vc;
            REQUIRE(vc == std::vector<char> { { 't' } });
        }

        // TBD yet to be implemented
        // SECTION("char* (char[])", "(beware buffer overruns)")
        // {
        //     std::vector<char[5]> vca;
        //     iss.str("[\"test\"]");
        //     iss >> vca;
        //     REQUIRE(vca == std::vector<char[5]> { { "test" } });
        // }

        SECTION("std::basic_string<char>&")
        {
            std::vector<std::string> vs;
            iss.str("[\"test\"]");
            iss >> vs;
            REQUIRE(vs == std::vector<std::string> { { "test" } });
        }
    }

    SECTION("defaults to strings::literal()")
    {
        std::istringstream iss;
        std::vector<std::string> vs;
        iss.str("[\"tes\\t\"]");
        iss >> vs;
        REQUIRE(vs == std::vector<std::string> { { "tes\t" } });
    }

    SECTION("can be set to strings::quoted() with iomanip strings::quotedrepr")
    {
        std::istringstream iss;
        iss >> strings::quotedrepr;
        std::vector<std::string> vs;
        iss.str("[\"tes\t\"]");
        iss >> vs;
        REQUIRE(vs == std::vector<std::string> { { "tes\t" } });
    }

    SECTION("can be set to strings::literal() with iomanip strings::literalrepr")
    {
        std::istringstream iss;
        iss >> strings::quotedrepr;   // change from default
        iss >> strings::literalrepr;  // restores default
        std::vector<std::string> vs;
        iss.str("[\"tes\\t\"]");
        iss >> vs;
        REQUIRE(vs == std::vector<std::string> { { "tes\t" } });
    }
}

/*
TEST_CASE("Delimiters: validate char defaults", "[decorator]")
{
    SECTION("Verify char delimiters for a non-specialized container type")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<int[], char>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     "[" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  "," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, " " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     "]" ));
    }

    SECTION("Verify char delimiters for a std::set<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::set<int>, char>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     "{" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  "," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, " " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     "}" ));
    }

    SECTION("Verify char delimiters for a std::pair<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::pair<int, float>, char>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     "(" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  "," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, " " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     ")" ));
    }

    SECTION("Verify char delimiters for a std::tuple<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::tuple<int, float>, char>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     "<" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  "," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, " " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     ">" ));
    }
}

TEST_CASE("Delimiters: validate wchar_t defaults", "[decorator]")
{
    SECTION("Verify wchar_t delimiters for a non-specialized container type")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<int[], wchar_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     L"[" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  L"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, L" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     L"]" ));
    }

    SECTION("Verify wchar_t delimiters for a std::set<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::set<int>, wchar_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     L"{" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  L"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, L" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     L"}" ));
    }

    SECTION("Verify wchar_t delimiters for a std::pair<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::pair<int, float>, wchar_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     L"(" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  L"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, L" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     L")" ));
    }

    SECTION("Verify wchar_t delimiters for a std::tuple<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::tuple<int, float>, wchar_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     L"<" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  L"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, L" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     L">" ));
    }
}

#if __cplusplus > 201703L
TEST_CASE("Delimiters: validate char8_t defaults", "[decorator]")
{
    SECTION("Verify char8_t delimiters for a non-specialized container type")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<int[], char8_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u8"[" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u8"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u8" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u8"]" ));
    }

    SECTION("Verify char8_t delimiters for a std::set<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::set<int>, char8_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u8"{" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u8"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u8" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u8"}" ));
    }

    SECTION("Verify char8_t delimiters for a std::pair<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::pair<int, float>, char8_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u8"(" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u8"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u8" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u8")" ));
    }

    SECTION("Verify char8_t delimiters for a std::tuple<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::tuple<int, float>, char8_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u8"<" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u8"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u8" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u8">" ));
    }
}
#endif  // above C++17

TEST_CASE("Delimiters: validate char16_t defaults", "[decorator]")
{
    SECTION("Verify char16_t delimiters for a non-specialized container type")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<int[], char16_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u"[" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u"]" ));
    }

    SECTION("Verify char16_t delimiters for a std::set<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::set<int>, char16_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u"{" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u"}" ));
    }

    SECTION("Verify char16_t delimiters for a std::pair<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::pair<int, float>, char16_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u"(" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u")" ));
    }

    SECTION("Verify char16_t delimiters for a std::tuple<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::tuple<int, float>, char16_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u"<" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u">" ));
    }
}

TEST_CASE("Delimiters: validate char32_t defaults", "[decorator]")
{
    SECTION("Verify char32_t delimiters for a non-specialized container type")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<int[], char32_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     U"[" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  U"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, U" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     U"]" ));
    }

    SECTION("Verify char32_t delimiters for a std::set<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::set<int>, char32_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     U"{" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  U"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, U" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     U"}" ));
    }

    SECTION("Verify char32_t delimiters for a std::pair<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::pair<int, float>, char32_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     U"(" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  U"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, U" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     U")" ));
    }

    SECTION("Verify char32_t delimiters for a std::tuple<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::tuple<int, float>, char32_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     U"<" ));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  U"," ));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, U" " ));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     U">" ));
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
*/
