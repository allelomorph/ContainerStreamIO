#include "../ContainerPrinter/ContainerPrinter.hpp"

#include "Stopwatch.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>

namespace
{
   void DoMoreWorkAtCompileTime()
   {
      const std::vector<int> vector{ 1, 2, 3, 4 };
      std::cout << vector << std::flush;
   }

   void DoMoreWorkAtRunTime()
   {
      const std::vector<int> container{ 1, 2, 3, 4 };

      const char* prefix = "[";
      const char* separator = ", ";
      const char* suffix = "]";

      auto begin = std::begin(container);
      std::cout << prefix << *begin;
      std::advance(begin, 1);

      std::for_each(begin, std::end(container),
         [&] (const auto& value)
      {
         std::cout << separator << value;
      });

      std::cout << suffix;
   }
}

int main()
{
   std::stringstream narrowBuffer;
   auto* const oldNarrowBuffer = std::cout.rdbuf(narrowBuffer.rdbuf());

   DoMoreWorkAtCompileTime();

   DoMoreWorkAtRunTime();

   std::cout.rdbuf(oldNarrowBuffer);

   return 0;
}

