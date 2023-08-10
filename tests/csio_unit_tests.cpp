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

#if (__cplusplus < 201103L)
#error "csio_unit_tests.cpp only supports C++11 and above"
#endif  // pre-C++11

namespace
{

template <typename Type>
class vector_wrapper : public std::vector<Type>
{
    using std::vector<Type>::vector;
};

template <typename Type>
class stack_wrapper : public std::stack<Type>
{};

#if __cplusplus >= 201703L

template <typename CharT>
using common_str_type = std::common_type_t<std::basic_string_view<CharT>>;

#else

template <typename CharT>
using common_str_type = typename std::common_type<std::basic_string<CharT>>::type;

#endif

inline bool idiomatic_strcmp(
    const common_str_type<char> s1, const common_str_type<char> s2)
{
    return s1 == s2;
}

inline bool idiomatic_strcmp(
    const common_str_type<wchar_t> s1, const common_str_type<wchar_t> s2)
{
    return s1 == s2;
}

#if __cplusplus > 201703L
inline bool idiomatic_strcmp(
    const common_str_type<char8_t> s1, const common_str_type<char8_t> s2)
{
    return s1 == s2;
}
#endif

inline bool idiomatic_strcmp(
    const common_str_type<char16_t> s1, const common_str_type<char16_t> s2)
{
    return s1 == s2;
}

inline bool idiomatic_strcmp(
    const common_str_type<char32_t> s1, const common_str_type<char32_t> s2)
{
    return s1 == s2;
}

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

} // namespace

using namespace container_stream_io;

TEST_CASE("Traits: detect as parseable (input stream extractable)",
          "[traits][input]")
{
    SECTION("non-nested")
    {
        SECTION("C arrays of non-char type")
        {
            REQUIRE(traits::is_parseable_as_container<int[5]>::value == true);
        }

        SECTION("supported STL containers")
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
            REQUIRE(traits::is_parseable_as_container<std::unordered_set<int>>::value == true);
            REQUIRE(traits::is_parseable_as_container<std::unordered_multiset<int>>::value == true);
        }

        SECTION("custom iterable container class",
                "(iterable being defiend as having members (typename)iterator, "
                "begin(), end(), and empty())")
        {
            REQUIRE(traits::is_parseable_as_container<vector_wrapper<int>>::value == true);
        }
    }

    SECTION("nesting")
    {
        SECTION("of supported STL containers")
        {
            REQUIRE(traits::is_parseable_as_container<std::map<int, float>>::value == true);
            REQUIRE(traits::is_parseable_as_container<std::multimap<int, float>>::value == true);
            REQUIRE(traits::is_parseable_as_container<std::unordered_map<int, float>>::value == true);
            REQUIRE(traits::is_parseable_as_container<std::unordered_multimap<int, float>>::value == true);
        }

        SECTION("containers with C array at some level")
        {
            SECTION("CharT[][]",
                    "(example char type = char)")
            {
                REQUIRE(traits::is_parseable_as_container<char[2][2]>::value == true);
            }

            SECTION("NonCharT[][]")
            {
                REQUIRE(traits::is_parseable_as_container<int[2][2]>::value == true);
            }

            SECTION("StlContainerT<>[]")
            {
                REQUIRE(traits::is_parseable_as_container<std::vector<int>[2]>::value == true);
            }

            SECTION("StlContainerType<Type[]>")
            {
                REQUIRE(traits::is_parseable_as_container<std::vector<int[2]>>::value == true);
            }
        }
    }
}

TEST_CASE("Traits: detect as not parseable (input stream extractable)",
          "[traits][input]")
{
    SECTION("containers interpretable as strings")
    {
        REQUIRE(traits::is_parseable_as_container<char[5]>::value == false);
        REQUIRE(traits::is_parseable_as_container<std::basic_string<char>>::value == false);
#if __cplusplus >= 201703L
        REQUIRE(traits::is_parseable_as_container<std::basic_string_view<char>>::value == false);
#endif
    }

    SECTION("STL containers that are not iterable")
    {
        REQUIRE(traits::is_parseable_as_container<std::stack<int>>::value == false);
        REQUIRE(traits::is_parseable_as_container<std::queue<int>>::value == false);
        REQUIRE(traits::is_parseable_as_container<std::priority_queue<int>>::value == false);
    }

    SECTION("custom non-iterable container class",
            "(iterable being defiend as having members (typename)iterator, "
            "begin(), end(), and empty())")
    {
        REQUIRE(traits::is_parseable_as_container<stack_wrapper<int>>::value == false);
    }
}

TEST_CASE("Traits: detect as printable (output stream insertable)",
          "[traits][output]")
{
    SECTION("non-nested")
    {
        SECTION("C arrays of non-char type")
        {
            REQUIRE(traits::is_printable_as_container<int[5]>::value == true);
        }

        SECTION("supported STL containers")
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
            REQUIRE(traits::is_printable_as_container<std::unordered_set<int>>::value == true);
            REQUIRE(traits::is_printable_as_container<std::unordered_multiset<int>>::value == true);
        }

        SECTION("custom iterable container class",
                "(iterable being defiend as having members (typename)iterator, "
                "begin(), end(), and empty())")
        {
            REQUIRE(traits::is_printable_as_container<vector_wrapper<int>>::value == true);
        }
    }

    SECTION("nesting")
    {
        SECTION("of supported STL containers")
        {
            REQUIRE(traits::is_printable_as_container<std::map<int, float>>::value == true);
            REQUIRE(traits::is_printable_as_container<std::multimap<int, float>>::value == true);
            REQUIRE(traits::is_printable_as_container<std::unordered_map<int, float>>::value == true);
            REQUIRE(traits::is_printable_as_container<std::unordered_multimap<int, float>>::value == true);
        }

        SECTION("containers with C array at some level")
        {
            SECTION("CharT[][]",
                    "(example char type = char)")
            {
                REQUIRE(traits::is_printable_as_container<char[2][2]>::value == true);
            }

            SECTION("NonCharT[][]")
            {
                REQUIRE(traits::is_printable_as_container<int[2][2]>::value == true);
            }

            SECTION("StlContainerT<>[]")
            {
                REQUIRE(traits::is_printable_as_container<std::vector<int>[2]>::value == true);
            }

            SECTION("StlContainerType<Type[]>")
            {
                REQUIRE(traits::is_printable_as_container<std::vector<int[2]>>::value == true);
            }
        }
    }
}

TEST_CASE("Traits: detect as not printable (output stream insertable)",
          "[traits][output]")
{
    SECTION("containers interpretable as strings")
    {
        REQUIRE(traits::is_printable_as_container<char[5]>::value == false);
        REQUIRE(traits::is_printable_as_container<std::basic_string<char>>::value == false);
#if __cplusplus >= 201703L
        REQUIRE(traits::is_printable_as_container<std::basic_string_view<char>>::value == false);
#endif
    }

    SECTION("STL containers that are not iterable")
    {
        REQUIRE(traits::is_printable_as_container<std::stack<int>>::value == false);
        REQUIRE(traits::is_printable_as_container<std::queue<int>>::value == false);
        REQUIRE(traits::is_printable_as_container<std::priority_queue<int>>::value == false);
    }

    SECTION("custom non-iterable container class",
            "(iterable being defiend as having members (typename)iterator, "
            "begin(), end(), and empty())")
    {
        REQUIRE(traits::is_printable_as_container<stack_wrapper<int>>::value == false);
    }
}

TEST_CASE("Traits: detect non-char types",
          "[traits][strings]")
{
    REQUIRE(traits::is_char_type<int>::value == false);
    REQUIRE(traits::is_char_type<std::vector<int>>::value == false);
}

TEST_CASE("Traits: detect char types",
          "[traits][strings]")
{
    REQUIRE(traits::is_char_type<char>::value == true);
    REQUIRE(traits::is_char_type<wchar_t>::value == true);
#if __cplusplus > 201703L
    REQUIRE(traits::is_char_type<char8_t>::value == true);
#endif
    REQUIRE(traits::is_char_type<char16_t>::value == true);
    REQUIRE(traits::is_char_type<char32_t>::value == true);
}

TEST_CASE("Traits: detect non-string types",
          "[traits][strings]")
{
    REQUIRE(traits::is_string_type<int>::value == false);
    REQUIRE(traits::is_string_type<int*>::value == false);
    REQUIRE(traits::is_string_type<int[5]>::value == false);
    REQUIRE(traits::is_string_type<std::vector<int>>::value == false);
}

TEST_CASE("Traits: detect string types",
          "[traits][strings]")
{
    SECTION("C strings",
            "(when passsing by const &, constness may be applied to pointer "
            "rather than char, so we include const varieties)")
    {
        REQUIRE(traits::is_c_string_type<char*>::value == true);
        REQUIRE(traits::is_c_string_type<const char*>::value == true);
        REQUIRE(traits::is_c_string_type<char[5]>::value == true);
        REQUIRE(traits::is_c_string_type<const char[5]>::value == true);

        REQUIRE(traits::is_string_type<char*>::value == true);
        REQUIRE(traits::is_string_type<const char*>::value == true);
        REQUIRE(traits::is_string_type<char[5]>::value == true);
        REQUIRE(traits::is_string_type<const char[5]>::value == true);
    }

    SECTION("STL strings",
            "(intended for resolutions when passing by const &, so no need to "
            "include const varieties)")
    {
        REQUIRE(traits::is_stl_string_type<std::basic_string<char>>::value == true);
        REQUIRE(traits::is_stl_string_type<const std::basic_string<char>>::value == false);
#if __cplusplus >= 201703L
        REQUIRE(traits::is_stl_string_type<std::basic_string_view<char>>::value == true);
        REQUIRE(traits::is_stl_string_type<const std::basic_string_view<char>>::value == false);
#endif

        REQUIRE(traits::is_string_type<std::basic_string<char>>::value == true);
        REQUIRE(traits::is_string_type<const std::basic_string<char>>::value == false);
#if __cplusplus >= 201703L
        REQUIRE(traits::is_string_type<std::basic_string_view<char>>::value == true);
        REQUIRE(traits::is_string_type<const std::basic_string_view<char>>::value == false);
#endif
    }
}

TEST_CASE("Traits: detect emplace methods",
          "[traits]")
{
    REQUIRE(traits::has_iterless_emplace<std::vector<int>>::value == false);
    REQUIRE(traits::has_emplace_back<std::vector<int>>::value == true);

    REQUIRE(traits::has_iterless_emplace<std::set<int>>::value == true);
    REQUIRE(traits::has_emplace_back<std::set<int>>::value == false);

    REQUIRE(traits::has_iterless_emplace<int>::value == false);
    REQUIRE(traits::has_emplace_back<int>::value == false);
}

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
        SECTION("eofbit being set by eof value extracted from malformed encoding "
                "depends on stream char type")
        {
            iss.clear();

            SECTION("EOF")
            {
                char c;
                char s[] { '\'', std::istringstream::traits_type::eof(), '\'', 0 };
                iss.str(s);
                iss >> strings::literal(c);
                // failure due to value outside 7-bit ASCII range, not eof
                REQUIRE((iss.fail() && !iss.eof()));
            }

            SECTION("WEOF")
            {
                wchar_t wc;
                wchar_t ws[] { L'L', L'\'', wchar_t(std::wistringstream::traits_type::eof()), L'\'', 0 };
                std::wistringstream wiss { ws };
                wiss >> strings::literal(wc);
                // failbit set along with eofbit
                REQUIRE((wiss.fail() && wiss.eof()));
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

        SECTION("eofbit being set by eof value extracted from quoted string "
                "depends on stream char type")
        {
            iss.clear();

            SECTION("EOF")
            {
                char c;
                char s[] { '\'', std::istringstream::traits_type::eof(), '\'', 0 };
                iss.str(s);
                iss >> strings::quoted(c);
                REQUIRE(iss.good());
                REQUIRE(c == std::istringstream::traits_type::eof());
            }

            SECTION("WEOF")
            {
                wchar_t wc;
                wchar_t ws[] { L'L', L'\'', wchar_t(std::wistringstream::traits_type::eof()), L'\'', 0 };
                std::wistringstream wiss { ws };
                wiss >> strings::quoted(wc);
                REQUIRE((wiss.fail() && wiss.eof()));
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

        SECTION("char&")
        {
            std::vector<char> vc { { 't' } };
            oss << vc;
            REQUIRE(oss.str() == "['t']");
        }

        // note that constness of vector conferred to elements
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

TEST_CASE("Delimiters: validate char defaults for", "[decorator]")
{
    SECTION("non-specialized container type")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<int[], char>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     "["));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  ","));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, " "));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     "]"));
    }

    SECTION("std::set<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::set<int>, char>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     "{"));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  ","));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, " "));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     "}"));
    }

    SECTION("std::pair<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::pair<int, float>, char>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     "("));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  ","));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, " "));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     ")"));
    }

    SECTION("std::tuple<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::tuple<int, float>, char>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     "<"));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  ","));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, " "));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     ">"));
    }
}

TEST_CASE("Delimiters: validate wchar_t defaults for", "[decorator]")
{
    SECTION("non-specialized container type")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<int[], wchar_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     L"["));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  L","));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, L" "));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     L"]"));
    }

    SECTION("std::set<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::set<int>, wchar_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     L"{"));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  L","));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, L" "));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     L"}"));
    }

    SECTION("std::pair<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::pair<int, float>, wchar_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     L"("));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  L","));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, L" "));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     L")"));
    }

    SECTION("std::tuple<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::tuple<int, float>, wchar_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     L"<"));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  L","));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, L" "));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     L">"));
    }
}

#if __cplusplus > 201703L
TEST_CASE("Delimiters: validate char8_t defaults for", "[decorator]")
{
    SECTION("non-specialized container type")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<int[], char8_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u8"["));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u8","));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u8" "));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u8"]"));
    }

    SECTION("std::set<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::set<int>, char8_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u8"{"));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u8","));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u8" "));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u8"}"));
    }

    SECTION("std::pair<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::pair<int, float>, char8_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u8"("));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u8","));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u8" "));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u8")"));
    }

    SECTION("std::tuple<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::tuple<int, float>, char8_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u8"<"));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u8","));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u8" "));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u8">"));
    }
}
#endif  // above C++17

TEST_CASE("Delimiters: validate char16_t defaults for", "[decorator]")
{
    SECTION("non-specialized container type")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<int[], char16_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u"["));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u","));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u" "));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u"]"));
    }

    SECTION("std::set<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::set<int>, char16_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u"{"));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u","));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u" "));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u"}"));
    }

    SECTION("std::pair<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::pair<int, float>, char16_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u"("));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u","));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u" "));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u")"));
    }

    SECTION("std::tuple<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::tuple<int, float>, char16_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     u"<"));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  u","));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, u" "));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     u">"));
    }
}

TEST_CASE("Delimiters: validate char32_t defaults for", "[decorator]")
{
    SECTION("non-specialized container type")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<int[], char32_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     U"["));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  U","));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, U" "));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     U"]"));
    }

    SECTION("std::set<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::set<int>, char32_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     U"{"));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  U","));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, U" "));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     U"}"));
    }

    SECTION("std::pair<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::pair<int, float>, char32_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     U"("));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  U","));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, U" "));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     U")"));
    }

    SECTION("std::tuple<...>")
    {
        constexpr auto delimiters {
            container_stream_io::decorator::delimiters<
            std::tuple<int, float>, char32_t>::values };

        REQUIRE(idiomatic_strcmp(delimiters.prefix,     U"<"));
        REQUIRE(idiomatic_strcmp(delimiters.separator,  U","));
        REQUIRE(idiomatic_strcmp(delimiters.whitespace, U" "));
        REQUIRE(idiomatic_strcmp(delimiters.suffix,     U">"));
    }
}

TEST_CASE("Printing/output streaming non-nested container types",
          "[output]")
{
    SECTION("does not support CharType[]",
            "(instead prints with STL operator<<(CharType*))")
    {
        std::ostringstream oss;
        char s[] { "tes\t" };
        oss << s;
        REQUIRE(oss.str() == "tes\t");
    }

    SECTION("supports NonCharType[]")
    {
        std::ostringstream oss;
        int a[] { 1, 2, 3, 4, 5 };
        oss << a;
        REQUIRE(oss.str() == "[1, 2, 3, 4, 5]");
    }

    SECTION("supports STL container")
    {
        std::ostringstream oss;

        SECTION("std::array")
        {
            std::array<int, 5> a { 1, 2, 3, 4, 5 };
            oss << a;
            REQUIRE(oss.str() == "[1, 2, 3, 4, 5]");
        }

        SECTION("std::vector")
        {
            std::vector<int> v { 1, 2, 3, 4, 5 };
            oss << v;
            REQUIRE(oss.str() == "[1, 2, 3, 4, 5]");
        }

        SECTION("std::pair")
        {
            std::pair<int, double> p { 1, 1.5 };
            oss << p;
            REQUIRE(oss.str() == "(1, 1.5)");
        }

        SECTION("std::tuple")
        {
            std::tuple<int, double, short> t { 1, 1.5, 2 };
            oss << t;
            REQUIRE(oss.str() == "<1, 1.5, 2>");
        }

        SECTION("std::deque")
        {
            std::deque<int> d { 1, 2, 3, 4, 5 };
            oss << d;
            REQUIRE(oss.str() == "[1, 2, 3, 4, 5]");
        }

        SECTION("std::forward_list")
        {
            std::forward_list<int> fl { 1, 2, 3, 4, 5 };
            oss << fl;
            REQUIRE(oss.str() == "[1, 2, 3, 4, 5]");
        }

        SECTION("std::list")
        {
            std::list<int> l { 1, 2, 3, 4, 5 };
            oss << l;
            REQUIRE(oss.str() == "[1, 2, 3, 4, 5]");
        }

        SECTION("std::set")
        {
            std::set<int> s { 1, 2, 3, 4, 5 };
            oss << s;
            REQUIRE(oss.str() == "{1, 2, 3, 4, 5}");
        }

        SECTION("std::multiset")
        {
            std::multiset<int> ms { 1, 2, 3, 4, 5 };
            oss << ms;
            REQUIRE(oss.str() == "{1, 2, 3, 4, 5}");
        }

        SECTION("std::unordered_set",
                "(unordered by definition, so serialization can be unpredictable - "
                "testable here due to how often order is reverse of insertion order)")
        {
            std::unordered_set<int> us { 1, 2, 3, 4, 5 };
            oss << us;
            REQUIRE(oss.str() == "[5, 4, 3, 2, 1]");
        }

        SECTION("std::unordered_multiset",
                "(unordered by definition, so serialization can be unpredictable - "
                "testable here due to how often order is reverse of insertion order)")
        {
            std::unordered_multiset<int> ums { 1, 2, 3, 4, 5 };
            oss << ums;
            REQUIRE(oss.str() == "[5, 4, 3, 2, 1]");
        }
    }

    SECTION("supports custom container classes, provided they are iterable",
            "(iterable being defiend as having members (typename)iterator, "
            "begin(), end(), and empty())")
    {
        std::ostringstream oss;
        vector_wrapper<int> vri { { 1, 2, 3, 4, 5 } };
        oss << vri;
        REQUIRE(oss.str() == "[1, 2, 3, 4, 5]");
    }
}

TEST_CASE("Printing/output streaming nested container types",
          "[output]")
{
    SECTION("supports C arrays in nesting configurations like")
    {
        std::ostringstream oss;

        SECTION("CharType[][] !!! currently failing with elements not printed using literal()",
                "(considered array of char*)")
        {
            char sa[2][5] { { "tes\t" }, { "\test" } };
            oss << sa;
            REQUIRE(oss.str() == "[\"tes\\t\", \"\\test\"]");
        }

        SECTION("NonCharType[][]")
        {
            int aa[2][3] { { 1, 2, 3 }, { 4, 5, 6 } };
            oss << aa;
            REQUIRE(oss.str() == "[[1, 2, 3], [4, 5, 6]]");
        }

        SECTION("StlContainerType<>[]")
        {
            std::vector<int> av[] { { 1, 2, 3 }, { 4, 5, 6 } };
            oss << av;
            REQUIRE(oss.str() == "[[1, 2, 3], [4, 5, 6]]");
        }

        SECTION("StlContainerType<Type[]>")
        {
            std::ostringstream oss;
            // bracketed initializer lists not successful
            std::vector<int[3]> va { 2 };
            va[0][0] = 1; va[0][1] = 2; va[0][2] = 3;
            va[1][0] = 4; va[1][1] = 5; va[1][2] = 6;
            oss << va;
            REQUIRE(oss.str() == "[[1, 2, 3], [4, 5, 6]]");
        }
    }

    SECTION("supports STL container combinations like")
    {
        std::ostringstream oss;

        SECTION("std::map")
        {
            std::map<int, float> m { { 1, 1.5 }, { 2, 2.5 } };
            oss << m;
            REQUIRE(oss.str() == "[(1, 1.5), (2, 2.5)]");
        }

        SECTION("std::multimap")
        {
            std::multimap<int, float> mm { { 1, 1.5 }, { 2, 2.5 } };
            oss << mm;
            REQUIRE(oss.str() == "[(1, 1.5), (2, 2.5)]");
        }

        SECTION("std::unordered_map",
                "(unordered by definition, so serialization can be unpredictable - "
                "testable here due to how often order is reverse of insertion order)")
        {
            std::unordered_map<int, float> um { { 1, 1.5 }, { 2, 2.5 } };
            oss << um;
            REQUIRE(oss.str() == "[(2, 2.5), (1, 1.5)]");
        }

        SECTION("std::unordered_multimap",
                "(unordered by definition, so serialization can be unpredictable - "
                "testable here due to how often order is reverse of insertion order)")
        {
            std::unordered_multimap<int, float> umm { { 1, 1.5 }, { 2, 2.5 } };
            oss << umm;
            REQUIRE(oss.str() == "[(2, 2.5), (1, 1.5)]");
        }
    }

    SECTION("supports custom container classes, provided they are iterable",
            "(iterable being defiend as having members (typename)iterator, "
            "begin(), end(), and empty())")
    {
        std::ostringstream oss;
        vector_wrapper<vector_wrapper<int>> vrvr { { 1, 2, 3 }, { 4, 5, 6 } };
        oss << vrvr;
        REQUIRE(oss.str() == "[[1, 2, 3], [4, 5, 6]]");
    }
}

TEST_CASE("Parsing/input streaming non-nested container types",
          "[input]")
{
    SECTION("does not support CharType[]",
            "(instead parses with STL operator>>(CharType*))")
    {
        std::istringstream iss { "test" };
        char s[5];
        iss >> s;
        REQUIRE(idiomatic_strcmp(s, "test"));
    }

    SECTION("supports NonCharType[]")
    {
        std::istringstream iss { "[1, 2, 3]" };
        int a[3];
        iss >> a;
        REQUIRE(a[0] == 1);
        REQUIRE(a[1] == 2);
        REQUIRE(a[2] == 3);
    }

    SECTION("supports STL container")
    {
        std::istringstream iss;

        SECTION("std::array")
        {
            iss.str("[1, 2, 3, 4, 5]");
            std::array<int, 5> a;
            iss >> a;
            REQUIRE(a == std::array<int, 5>{ 1, 2, 3, 4, 5 });
        }

        SECTION("std::vector")
        {
            iss.str("[1, 2, 3, 4, 5]");
            std::vector<int> v;
            iss >> v;
            REQUIRE(v == std::vector<int>{ 1, 2, 3, 4, 5 });
        }

        SECTION("std::pair")
        {
            iss.str("(1, 1.5)");
            std::pair<int, double> p;
            iss >> p;
            REQUIRE(p == std::pair<int, double>{ 1, 1.5 });
        }

        SECTION("std::tuple")
        {
            iss.str("<1, 1.5, 2>");
            std::tuple<int, double, short> t;
            iss >> t;
            REQUIRE(t == std::tuple<int, double, short>{ 1, 1.5, 2 });
        }

        SECTION("std::deque")
        {
            iss.str("[1, 2, 3, 4, 5]");
            std::deque<int> d;
            iss >> d;
            REQUIRE(d == std::deque<int>{ 1, 2, 3, 4, 5 });
        }

        SECTION("std::forward_list")
        {
            iss.str("[1, 2, 3, 4, 5]");
            std::forward_list<int> fl;
            iss >> fl;
            REQUIRE(fl == std::forward_list<int>{ 1, 2, 3, 4, 5 });
        }

        SECTION("std::list")
        {
            iss.str("[1, 2, 3, 4, 5]");
            std::list<int> l;
            iss >> l;
            REQUIRE(l == std::list<int>{ 1, 2, 3, 4, 5 });
        }

        SECTION("std::set")
        {
            iss.str("{1, 2, 3, 4, 5}");
            std::set<int> s;
            iss >> s;
            REQUIRE(s == std::set<int>{ 1, 2, 3, 4, 5 });
        }

        SECTION("std::multiset")
        {
            iss.str("{1, 2, 3, 4, 5}");
            std::multiset<int> ms;
            iss >> ms;
            REQUIRE(ms == std::multiset<int>{ 1, 2, 3, 4, 5 });
        }

        SECTION("std::unordered_set",
                "(unordered by definition, so relationship between element order "
                "in serialization and extracted order is unpredictable)")
        {
            iss.str("[1, 2, 3, 4, 5]");
            std::unordered_set<int> us;
            iss >> us;
            REQUIRE(us == std::unordered_set<int>{ 1, 2, 3, 4, 5 });
        }

        SECTION("std::unordered_multiset",
                "(unordered by definition, so relationship between element order "
                "in serialization and extracted order is unpredictable)")
        {
            iss.str("[1, 2, 3, 4, 5]");
            std::unordered_multiset<int> ums;
            iss >> ums;
            REQUIRE(ums == std::unordered_multiset<int>{ 1, 2, 3, 4, 5 });
        }
    }

    SECTION("supports custom container classes, provided they are iterable",
            "(iterable being defiend as having members (typename)iterator, "
            "begin(), end(), and empty())")
    {
        std::istringstream iss { "[1, 2, 3, 4, 5]" };
        vector_wrapper<int> vr;
        iss >> vr;
        REQUIRE(vr == vector_wrapper<int>{ { 1, 2, 3, 4, 5 } });
    }
}

TEST_CASE("Parsing/input streaming nested container types",
          "[input]")
{
    SECTION("supports C arrays in nesting configurations like")
    {
        std::istringstream iss;

        // !!! why not failing (to use literal()) when equivalent output test fails?
        SECTION("CharType[][]",
                "(considered array of char*)")
        {
            iss.str("[\"tes\\t\", \"\\test\"]");
            char sa[2][5];
            iss >> sa;
            REQUIRE(idiomatic_strcmp(sa[0], "tes\t"));
            REQUIRE(idiomatic_strcmp(sa[1], "\test"));
        }

        SECTION("NonCharType[][]")
        {
            iss.str("[[1, 2], [3, 4]]");
            int aa[2][2];
            iss >> aa;
            REQUIRE(aa[0][0] == 1);
            REQUIRE(aa[0][1] == 2);
            REQUIRE(aa[1][0] == 3);
            REQUIRE(aa[1][1] == 4);
        }

        SECTION("StlContainerType<>[]")
        {
            iss.str("[[1, 2, 3], [4, 5, 6]]");
            std::vector<int> av[2];
            iss >> av;
            REQUIRE(av[0] == std::vector<int>{1, 2, 3});
            REQUIRE(av[1] == std::vector<int>{4, 5, 6});
        }

        // !!! can this be remedied with some workaround for T[] members?
        //   (why does this fail when T[][] works?)
        SECTION("StlContainerType<Type[]>")
        {
            iss.str("[[1, 2], [3, 4]]");
            std::vector<int[2]> va;
            // iss >> va;  // won't compile
            REQUIRE(!std::is_move_constructible<
                    typename std::vector<int[2]>::value_type>::value);
        }
    }

    SECTION("supports STL container combinations like")
    {
        std::istringstream iss;

        SECTION("std::map")
        {
            iss.str("[(1, 1.5), (2, 2.5)]");
            std::map<int, float> m;
            iss >> m;
            REQUIRE(m == std::map<int, float>{ { 1, 1.5 }, { 2, 2.5 } });
        }

        SECTION("std::multimap")
        {
            iss.str("[(1, 1.5), (2, 2.5)]");
            std::multimap<int, float> mm;
            iss >> mm;
            REQUIRE(mm == std::multimap<int, float>{ { 1, 1.5 }, { 2, 2.5 } });
        }

        SECTION("std::unordered_map",
                "(unordered by definition, so relationship between element order "
                "in serialization and extracted order is unpredictable)")
        {
            iss.str("[(2, 2.5), (1, 1.5)]");
            std::unordered_map<int, float> um;
            iss >> um;
            REQUIRE(um == std::unordered_map<int, float>{ { 1, 1.5 }, { 2, 2.5 } });
        }

        SECTION("std::unordered_multimap",
                "(unordered by definition, so relationship between element order "
                "in serialization and extracted order is unpredictable)")
        {
            iss.str("[(2, 2.5), (1, 1.5)]");
            std::unordered_multimap<int, float> umm { { 1, 1.5 }, { 2, 2.5 } };
            iss >> umm;
            REQUIRE(umm == std::unordered_multimap<int, float>{ { 1, 1.5 }, { 2, 2.5 } });
        }
    }

    SECTION("supports custom container classes, provided they are iterable",
            "(iterable being defiend as having members (typename)iterator, "
            "begin(), end(), and empty())")
    {
        std::istringstream iss { "[[1, 2, 3], [4, 5, 6]]" };
        vector_wrapper<vector_wrapper<int>> vrvr { { 1, 2, 3 }, { 4, 5, 6 } };
        iss >> vrvr;
        REQUIRE(vrvr == vector_wrapper<vector_wrapper<int>>{ { 1, 2, 3 }, { 4, 5, 6 } });
    }
}

TEST_CASE("Supported container types should not change after being encoded and "
          "then decoded",
          "[output][input]")
{
    SECTION("when non-nested")
    {
        std::stringstream ss;

        SECTION("NonCharType[]")
        {
            int a[] { 1, 2, 3 };
            ss << a;
            int _a[3];
            ss >> _a;
            REQUIRE(_a[0] == a[0]);
            REQUIRE(_a[1] == a[1]);
            REQUIRE(_a[2] == a[2]);
        }

        SECTION("std::array")
        {
            std::array<int, 5> a { 1, 2, 3, 4, 5 };
            ss << a;
            std::array<int, 5> _a;
            ss >>_a;
            REQUIRE(_a == a);
        }

        SECTION("std::vector")
        {
            std::vector<int> v { 1, 2, 3, 4, 5 };
            ss << v;
            std::vector<int> _v;
            ss >>_v;
            REQUIRE(_v == v);
        }

        SECTION("std::pair")
        {
            std::pair<int, double> p { 1, 1.5 };
            ss << p;
            std::pair<int, double> _p;
            ss >>_p;
            REQUIRE(_p == p);
        }

        SECTION("std::tuple")
        {
            std::tuple<int, double, short> t { 1, 1.5, 2 };
            ss << t;
            std::tuple<int, double, short> _t;
            ss >> _t;
            REQUIRE(_t == t);
        }

        SECTION("std::deque")
        {
            std::deque<int> d { 1, 2, 3, 4, 5 };
            ss << d;
            std::deque<int> _d;
            ss >> _d;
            REQUIRE(_d == d);
        }

        SECTION("std::forward_list")
        {
            std::forward_list<int> fl { 1, 2, 3, 4, 5 };
            ss << fl;
            std::forward_list<int> _fl;
            ss >> _fl;
            REQUIRE(_fl == fl);
        }

        SECTION("std::list")
        {
            std::list<int> l { 1, 2, 3, 4, 5 };
            ss << l;
            std::list<int> _l;
            ss >> _l;
            REQUIRE(_l == l);
        }

        SECTION("std::set")
        {
            std::set<int> s { 1, 2, 3, 4, 5 };
            ss << s;
            std::set<int> _s { 1, 2, 3, 4, 5 };
            ss >> _s;
            REQUIRE(_s == s);
        }

        SECTION("std::multiset")
        {
            std::multiset<int> ms { 1, 2, 3, 4, 5 };
            ss << ms;
            std::multiset<int> _ms;
            ss >> _ms;
            REQUIRE(_ms == ms);
        }

        SECTION("std::unordered_set",
                "(unordered by definition, so serialization can be unpredictable, "
                "but resulting contents should be equivalent)")
        {
            std::unordered_set<int> us { 1, 2, 3, 4, 5 };
            ss << us;
            std::unordered_set<int> _us;
            ss >> _us;
            REQUIRE(_us == us);
        }

        SECTION("std::unordered_multiset",
                "(unordered by definition, so serialization can be unpredictable, "
                "but resulting contents should be equivalent)")
        {
            std::unordered_multiset<int> ums { 1, 2, 3, 4, 5 };
            ss << ums;
            std::unordered_multiset<int> _ums;
            ss >> _ums;
            REQUIRE(_ums == ums);
        }

        SECTION("iterable custom container class",
                "(iterable being defiend as having members (typename)iterator, "
                "begin(), end(), and empty())")
        {
            vector_wrapper<int> vr { { 1, 2, 3, 4, 5 } };
            ss << vr;
            vector_wrapper<int> _vr { { 1, 2, 3, 4, 5 } };
            ss >> _vr;
            REQUIRE(_vr == vr);
        }
    }

    SECTION("when nested with C arrays in configurations like")
    {
        std::stringstream ss;

        SECTION("CharType[][] !!! currently failing with elements not printed using literal()",
                "(considered array of char*)")
        {
            char sa[2][5] { { "tes\t" }, { "\test" } };
            ss << sa;
            char _sa[2][5];
            ss >> _sa;
            REQUIRE(idiomatic_strcmp(_sa[0], sa[0]));
            REQUIRE(idiomatic_strcmp(_sa[1], sa[1]));
        }

        SECTION("NonCharType[][]")
        {
            int aa[2][2] { { 1, 2 }, { 3, 4 } };
            ss << aa;
            int _aa[2][2];
            ss >> _aa;
            REQUIRE(_aa[0][0] == aa[0][0]);
            REQUIRE(_aa[0][1] == aa[0][1]);
            REQUIRE(_aa[1][0] == aa[1][0]);
            REQUIRE(_aa[1][1] == aa[1][1]);
        }

        SECTION("StlContainerType<>[]")
        {
            std::vector<int> av[] { { 1, 2, 3 }, { 4, 5, 6 } };
            ss << av;
            std::vector<int> _av[2];
            ss >> _av;
            REQUIRE(_av[0] == av[0]);
            REQUIRE(_av[1] == av[1]);
        }

        // TBD fix istreaming into containers with non-move-constructible elements
        // SECTION("StlContainerType<Type[]>")
        // {
        //     // bracketed initializer lists not successful
        //     std::vector<int[2]> va { 2 };
        //     va[0][0] = 1; va[0][1] = 2;
        //     va[1][0] = 3; va[1][1] = 4;
        //     ss << va;
        //     std::vector<int[2]> _va { 2 };
        //     // ss >> _va;  // won't compile
        //     REQUIRE(_aa[0][0] == aa[0][0]);
        //     REQUIRE(_aa[0][1] == aa[0][1]);
        //     REQUIRE(_aa[1][0] == aa[1][0]);
        //     REQUIRE(_aa[1][1] == aa[1][1]);
        // }
    }

    SECTION("with nested STL container combinations like")
    {
        std::stringstream ss;

        SECTION("std::map")
        {
            std::map<int, float> m { { 1, 1.5 }, { 2, 2.5 } };
            ss << m;
            std::map<int, float> _m;
            ss >> _m;
            REQUIRE(_m == m);
        }

        SECTION("std::multimap")
        {
            std::multimap<int, float> mm { { 1, 1.5 }, { 2, 2.5 } };
            ss << mm;
            std::multimap<int, float> _mm;
            ss >> _mm;
            REQUIRE(_mm == mm);
        }

        SECTION("std::unordered_map",
                "(unordered by definition, so serialization can be unpredictable, "
                "but resulting contents should be equivalent)")
        {
            std::unordered_map<int, float> um { { 1, 1.5 }, { 2, 2.5 } };
            ss << um;
            std::unordered_map<int, float> _um;
            ss >> _um;
            REQUIRE(_um == um);
        }

        SECTION("std::unordered_multimap",
                "(unordered by definition, so serialization can be unpredictable, "
                "but resulting contents should be equivalent)")
        {
            std::unordered_multimap<int, float> umm { { 1, 1.5 }, { 2, 2.5 } };
            ss << umm;
            std::unordered_multimap<int, float> _umm;
            ss >> _umm;
            REQUIRE(_umm == umm);
        }
    }
}

TEST_CASE("Printing with custom formatter",
          "[output]")
{
    SECTION("to wide stream")
    {
        std::wostringstream woss;

        SECTION("std::vector")
        {
            const auto container { std::vector<int>{ 1, 2, 3, 4 } };
            container_stream_io::output::to_stream(
                woss, container, custom_formatter{}) << std::flush;
            REQUIRE(woss.str() == L"$$ 1 | 2 | 3 | 4 $$");
        }

        SECTION("std::tuple<>")
        {
            const auto container { std::tuple<int, double, short>{ 1, 1.5, 2 } };
            container_stream_io::output::to_stream(
                woss, container, custom_formatter{}) << std::flush;
            REQUIRE(woss.str() == L"$$ 1 | 1.5 | 2 $$");
        }

        SECTION("std::pair")
        {
            const auto container { std::pair<int, double> { 1, 1.5 } };
            container_stream_io::output::to_stream(
                woss, container, custom_formatter{}) << std::flush;
            REQUIRE(woss.str() == L"$$ 1 | 1.5 $$");
        }
    }
}
