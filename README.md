# Container Printer

A simple header-only solution to enable easier `std::ostream` printing of various Standard Library data structures. Using this library, a `std::vector<int>`, for instance, might be printed as follows:

```C++
const auto std::vector<int> vector { 1, 2, 3, 4 };
std::cout << vector << std::endl;
```

By virtue of template meta-programming (TMP), the following data structures should also work:

* `std::pair<...>`
* `std::tuple<...>`
* `std::set<...>`
* `std::multiset<...>`
* `std::vector<...>`
* `std::map<...>`
* `std::unordered_map<...>`

Additionally, any custom data structure that conforms to the `Iterator` concept and provides public `begin()`, `end()`, and `empty()` member functions should also work.

See the included unit tests for more examples.
