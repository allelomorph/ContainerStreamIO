# Container Stream I/O
A header-only solution to enable both input and output streaming of various Standard Library data structures. Based on [Tim Severeijns' Container Printer](https://github.com/TimSevereijns/ContainerPrinter).

## Goals
Originally:
* level up understanding of C++ template metaprogramming/SFINAE techniques
* create some means of printing STL containers for debugging

After finding Severeijns' solution:
* using Container Printer as a primer and then extending it to also provide:
  * input streaming
  * escaped strings in the style of `std::quoted`, or of string literals in source
  * no need for strings/chars as container elements to match char type of stream

## Features

### Container Streaming

With this header included, you can insert STL containers in an output stream. For example,
```C++
const std::vector<int> v { 1, 2, 3, 4 };
std::cout << v << std::endl;
```
would print
```
[1, 2, 3, 4]
```
to `cout`. Or, if input streaming text that mostly conforms to the output stream encoding scheme (whitespace can vary,) you can extract an equivalent container, elements included:
```C++
std::vector<int> v;
std::cin >> v;
```
followed by entering:
```
[1, 2, 3, 4]
```
should make `v` equivalent to `std::vector<int>{1, 2, 3, 4}`.

This ability extends to string and file streams as well, and thus can act as somewhat of a specialized JSON alternative for encoding/decoding containers.

#### Supported Types
Both input and output streaming of many STL containers (of those available in the C++17 Containers library) is supported:
* `std::array`
* `std::vector`
* `std::deque`
* `std::forward_list`
* `std::list`
* `std::set`
* `std::multiset`
* `std::map`
* `std::multimap`
* `std::unordered_set`
* `std::unordered_multiset`
* `std::unordered_map`
* `std::unordered_multimap`

in addition to:
* `std::pair`
* `std::tuple`
* `T[]` (C arrays)

but currently not:
* `std::stack`
* `std::queue`
* `std::priority_queue`

Additionally, any custom data structure that conforms to the [Iterator](http://en.cppreference.com/w/cpp/concept/Iterator) concept and provides public `begin()`, `end()`, and `empty()` member functions can be output streamed. Custom data structures with public members `value_type`, `clear()`, and either `emplace()` (without a placement iterator) or `emplace_back()` can be input streamed.

#### Nested Containers
Nesting STL containers of just about any combination are supported both for input and output streaming, indeed maps and sets already have pairs as elements. Streaming of container elements of containers is recursive, so the only real limit is working memory. C arrays are also supported in many but not all nesting relationships\*:
|    | input | output |
| -- | :---: | :----: |
| `CharT[]...[]` | yes | yes |
| `NonCharT[]...[]` | yes | yes |
| `StlContainerT<T>[]` | yes | yes |
| `StlContainerT<T[]>` | no (`T[]` not move-assignable) | yes |

\* (These relationships can of course be further recombined, eg `StlContainerT<T[]>[]` or `StlContainerT<T[][]>`.)

### Escaped Strings
Strings or chars outside containers will be streamed as they normally would, using their default STL stream operators. But to represent string or char elements inside compatible containers two encodings are introduced:

#### Quoted
Essentially the same as `std::quoted`:
* delimiter char is string/char prefix and suffix
* escape char is prefix to itself and the delimiter
* delimiter and escape can be set to any value in range of char type (not restricted to 7-bit ASCII)

plus:
* string/char will have a literal prefix indicating its char type (eg `L` for `wchar_t`, `u` for `char16_t`, etc.)

Quoted encoding can also be used outside of containers by streaming with `container_stream_io::strings::quoted()`, eg:
```C++
// default delimiter/escape
std::cout << container_stream_io::strings::quoted("test\\\"") << std::endl;
// setting own delimiter/escape
std::cout << container_stream_io::strings::quoted("test|^", '^', '|') << std::endl;
```
should print to `cout`:
```
"test\\\""
^test|||^^
```

#### Literal
Aims to generally emulate the representations of string/char literals in C++ source, with some adjustments:
* encoding limited to printable 7-bit ASCII:
  * only printable ASCII delimiter/escape allowed
  * delimiter and escape escaped just as with quoted
  * standard unprintable ASCII escape sequences (eg `\t`, `\n`) escaped
  * other unprintable ASCII and values beyond 7-bit range (`0x7f`) hex escaped as `\x`...
    * hex escapes are fixed width and proportional to string char size
* literal prefixes as with quoted

Literal encoding can also be used outside of containers by streaming with `container_stream_io::strings::literal()`, eg:
```C++
// default delimiter/escape
std::cout << container_stream_io::strings::literal("test\\\"\t\x01\xfe") << std::endl;
// setting own delimiter/escape
std::cout << container_stream_io::strings::literal("test|^\t\x01\xfe", '^', '|') << std::endl;
```
should print to `cout`:
```
"test\\\"\t\x01\xfe"
^test|||^|t|x01|xfe^
```

#### Setting Default Behavior
By default, string/char container elements are streamed with `literal()`. But this default behavior can be toggled by streaming one of the two provided I/O manipulators, `container_stream_io::strings::quotedrepr` and `container_stream_io::strings::literalrepr`, eg:
```C++
std::cout << container_stream_io::strings::quotedrepr;
```
would make any containers printed to `cout` with string/char elements encode them as quoted from that point on, or until `literalrepr` was streamed to the same stream. Note that this will have to be set separately for every stream, so if also extracting from `cin`, `quotedrepr` would have to be streamed to `cin` before the encoding would match `cout` in the previous example.

#### Stream vs Element Char Types
Conveniently, unlike with the default STL stream operators, when using these encodings there is not always a need to match the string char type to the stream char type. Streaming char type mismatches are supported under the following conditions:
|     | input | output |
| --- | :---: | :----: |
| quoted | size of stream char type <= size of string char type | size of stream char type >= size of string char type |
| literal | any combination | any combination |


### Custom Formatting
If you'd like to modify the tokens used between and around the container elements, or even how those elements themselves are encoded, you can provide your own custom formatter, either for input or for output. This custom formatter should be a class or struct with the following function signatures either for input:
* `[static] void parse_prefix(StreamType&)`
* `[static] void parse_element(StreamType&, ElementType&)`
* `[static] void parse_delimiter(StreamType&)`
* `[static] void parse_suffix(StreamType&)`

or for output:
* `[static] void print_prefix(StreamType&)`
* `[static] void print_element(StreamType&, ElementType&)`
* `[static] void print_delimiter(StreamType&)`
* `[static] void print_suffix(StreamType&)`

Any one of these functions can be static, but certainly don't have to be. In fact, you custom formatter class/struct may even be stateful.

As an example, here's a very simple custom formatter (limited to wide character output streams):
```C++
struct custom_formatter
{
    template <typename StreamType>
    void print_prefix(StreamType& stream) const noexcept
    {
        stream << L"$$ ";
    }

    template <typename StreamType, typename ElementType>
    void print_element(StreamType& stream, const ElementType& element) const noexcept
    {
        stream << element;
    }

    template <typename StreamType>
    void print_separator(StreamType& stream) const noexcept
    {
        stream << L" | ";
    }

    template <typename StreamType>
    void print_suffix(StreamType& stream) const noexcept
    {
        stream << L" $$";
    }
};
```
Using a custom formatter requires calling `to_stream`/`from_stream` directly (rather than using the stream operators, which invoke the default formatter):
```C++
std::vector<int> v { 1, 2, 3, 4 };
container_printer::to_stream(std::wcout, v, custom_formatter{});
```
printing to wcout:
```
$$ 1 | 2 | 3 | 4 $$
```
Two items to note in the above example formatter struct:
* member functions must be const as a consequence of calling `to_stream`/`from_stream` directly
* templating the member functions instead of the struct as a whole allows for parameter deduction at the call-site, preventing the need for template arguments for every struct instantiation

## Usage
All that's required is inclusion of `container_printer.hh` in the relevant source of your project.

Please see included [unit tests](./tests/unit_tests.cpp) for more examples of features and usage.
