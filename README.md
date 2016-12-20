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

Just include the `ContainerPrinter.hpp` header, and you should be good to go. It should also be noted that this project was developed using Visual Studio 2015 (Update 3), and as such, makes use of various language features from both `C++11` and `C++14` which older compilers may not fully support. 

See the included unit tests for more examples.
