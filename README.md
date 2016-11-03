# Container Printer

A simple header-only solution to enable easier `std::ostream` printing of various Standard Library data structures. Using this library, a `std::vector<int>`, for instance, might be printed as follows:

```C++
const auto std::vector<int> vector { 1, 2, 3, 4 };
std::cout << vector << std::endl;
```

By virtue of Generic Programming, the following data structures should also work:

* `std::pair<...>`
* `std::tuple<...>`
* `std::set<...>`
* `std::vector<...>`
* `std::map<...>`
* `std::unordered_map<...>`
