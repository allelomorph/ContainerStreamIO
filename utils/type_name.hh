#ifndef TYPE_NAME_HH
#define TYPE_NAME_HH

/*
 * Adapted from:
 * https://stackoverflow.com/a/20170989
 * https://stackoverflow.com/a/56766138
 * GNU ISO C++ header string_view
 *
 * See also alternate solution via Boost type_id_with_cvr:
 * https://stackoverflow.com/a/28621907
 * https://www.boost.org/doc/libs/1_82_0/doc/html/boost/typeindex/type_id_with_cvr.html
 * https://github.com/boostorg/type_index/tree/develop/include/boost/type_index
 *
 * For reference on Visual Studio version/_MSC_VER values versus ISO C++
 *   standard version/__cplusplus value:
 * https://learn.microsoft.com/en-us/cpp/overview/visual-cpp-language-conformance
 * https://learn.microsoft.com/en-us/cpp/preprocessor/predefined-macros
 * https://dev.to/yumetodo/list-of-mscver-and-mscfullver-8nd
 */

// undefined evaluates to 0
#if (defined(_MSC_VER) && _MSC_VER < 1928) || \
    (defined(__cplusplus) && __cplusplus < 201703L)


#  ifndef _MSC_VER
#    if __cplusplus < 201103L
#      error "type_name only backwards compatible to ISO C++11 standard"
#    endif
#  else  // _MSC_VER
#    if _MSC_VER < 1900
#      error "type_name only backwards compatible to ISO C++11 standard (VS 2015)"
#    endif
#  endif  // _MSC_VER

#  define CSTR_SIZE(char_arr) (sizeof(char_arr) - 1)

#  include <stdexcept>
#  include <ostream>

template<typename CharT>
class basic_static_string {
    static_assert(std::is_same<CharT, char>::value ||
                  std::is_same<CharT, wchar_t>::value ||
                  std::is_same<CharT, char16_t>::value ||
                  std::is_same<CharT, char32_t>::value,
                  "basic_static_string can only be used with char types");

    const CharT* _p;
    std::size_t _sz;

public:
    using const_iterator = const CharT*;

    template <std::size_t N>
    constexpr basic_static_string(const CharT(&a)[N]) noexcept
        : _p(a), _sz(N - 1) {}

    constexpr basic_static_string(const CharT* p, const std::size_t N) noexcept
        : _p(p), _sz(N) {}

    constexpr const CharT* data() const noexcept { return _p; }
    constexpr std::size_t size() const noexcept { return _sz; }

    constexpr const_iterator begin() const noexcept { return _p; }
    constexpr const_iterator end()   const noexcept { return _p + _sz; }

    constexpr CharT operator[](const std::size_t n) const {
        return n < _sz ? _p[n] : throw std::out_of_range("basic_static_string");
    }
};

template<typename CharT>
inline std::basic_ostream<CharT>& operator<<(
    std::basic_ostream<CharT>& os, const basic_static_string<CharT>& s) {
    return os.write(s.data(), s.size());
}

using static_string = basic_static_string<char>;
using wstatic_string = basic_static_string<wchar_t>;
using u16static_string = basic_static_string<char16_t>;
using u32static_string = basic_static_string<char32_t>;

template <class T>
constexpr static_string type_name() {
    // return static_string(name.data() + prefix.size(),
    //                      name.size() - prefix.size() - suffix.size());
#  ifdef __clang__
    return static_string(
        __PRETTY_FUNCTION__ + CSTR_SIZE("static_string type_name() [with T = "),
        CSTR_SIZE(__PRETTY_FUNCTION__) -
        CSTR_SIZE("static_string type_name() [with T = ") -
        CSTR_SIZE("; static_string = basic_static_string<char>]"));
#  elif defined(__GNUC__)
    return static_string(
        __PRETTY_FUNCTION__ + CSTR_SIZE("constexpr static_string type_name() [with T = "),
        CSTR_SIZE(__PRETTY_FUNCTION__) -
        CSTR_SIZE("constexpr static_string type_name() [with T = ") -
        CSTR_SIZE("; static_string = basic_static_string<char>]"));
#  elif defined(_MSC_VER)
    return static_string(
        __FUNC_SIG__ + CSTR_SIZE("static_string __cdecl type_name<"),
        CSTR_SIZE(__FUNC_SIG__) -
        CSTR_SIZE("static_string __cdecl type_name<") -
        CSTR_SIZE(">(void)"));
#  endif
}


#else  // C++17 and up


#  include <string_view>

template <class T>
constexpr auto type_name() {
    std::string_view name, prefix, suffix;
#  ifdef __clang__
    name = __PRETTY_FUNCTION__;
    prefix = "auto type_name() [T = ";
    suffix = "]";
#  elif defined(__GNUC__)
    name = __PRETTY_FUNCTION__;
    prefix = "constexpr auto type_name() [with T = ";
    suffix = "]";
#  elif defined(_MSC_VER)
    name = __FUNCSIG__;
    prefix = "auto __cdecl type_name<";
    suffix = ">(void)";
#  endif
    name.remove_prefix(prefix.size());
    name.remove_suffix(suffix.size());
    return name;
}


#endif  // C++17 and up


#endif  // TYPE_NAME_HH
