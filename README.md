# Container Printer

A simple header-only solution to enable easier `std::ostream` printing of various Standard Library data structures. Using this library, a `std::vector<int>`, for instance, might be printed as follows:

```C++
const std::vector<int> vector { 1, 2, 3, 4 };
std::cout << vector << std::endl;
```

By virtue of generic programming, the following data structures should also work:

* `std::pair<...>`
* `std::tuple<...>`
* `std::set<...>`
* `std::multiset<...>`
* `std::vector<...>`
* `std::map<...>`
* `std::unordered_map<...>`

Additionally, any custom data structure that conforms to the [Iterator](http://en.cppreference.com/w/cpp/concept/Iterator) concept and provides public `begin()`, `end()`, and `empty()` member functions should also work.

# Custom Formatting

If you'd like to modify the prefix, delimiter, or suffix strings emitted to the output stream, or even the container elements themselves, you can provide your own custom formatter. This custom formatter should be either a `class` or `struct` with the following function signatures:

* `[static] void print_prefix(StreamType&)`
* `[static] void print_element(StreamType&, ElementType&)`
* `[static] void print_delimiter(StreamType&)`
* `[static] void print_suffix(StreamType&)`

Any one of these functions can be static, but certainly don't have to be. In fact, you custom formatter `class` or `struct` may even be stateful.

Here's an example of a very simple custom formatter (for wide character streams):

```C++
struct custom_formatter
{
    template <typename StreamType> static void print_prefix(StreamType& stream) noexcept
    {
        stream << L"$$ ";
    }

    template <typename StreamType, typename ElementType>
    static void print_element(StreamType& stream, const ElementType& element) noexcept
    {
        stream << element;
    }

    template <typename StreamType> static void print_delimiter(StreamType& stream) noexcept
    {
        stream << L" | ";
    }

    template <typename StreamType> static void print_suffix(StreamType& stream) noexcept
    {
        stream << L" $$";
    }
};
```
And here is an example of how that formatter might be used (taken from `UnitTests.cpp`):

```C++
 SECTION("Printing a populated std::vector<...> to a wide stream.")
 {
    const auto container = std::vector<int>{ 1, 2, 3, 4 };

    container_printer::to_stream(std::wcout, container, custom_formatter{ });
    std::wcout << std::flush;

    REQUIRE(std_cout_buffer.str() == std::wstring{ L"$$ 1 | 2 | 3 | 4 $$" });
 }
```

Note that by templating the individual functions on the `custom_formatter`, instead of the `struct` as a whole, we can allow the compiler to deduce all the necessary template arguments for us at the call-site, thereby allowing us to write cleaner code.

# Usage

Just include the `container_printer.h` header, and you should be good to go.

See the included unit tests for more examples.