#pragma once

#include <utility>

/**
* @brief ScopeExit
*/
template<typename LambdaType>
struct ScopeExit
{
   ScopeExit(LambdaType&& lambda) noexcept :
      m_lambda{ std::move(lambda) }
   {
   }

   ~ScopeExit()
   {
      m_lambda();
   }

   ScopeExit(const ScopeExit&) = delete;
   ScopeExit& operator=(const ScopeExit&) = delete;

   ScopeExit(ScopeExit&&) = default;
   ScopeExit& operator=(ScopeExit&&) = default;

private:
   LambdaType m_lambda;
};

namespace
{
   struct DummyStruct
   {
      // Left intentionally empty.
   };
}

/**
* Enables the use of braces to define the contents of the RAII object.
*
* @param[in] function              The lambda to be executed upon destruction of the ScopeExit
*                                  object.
*
* @returns An RAII object encapsulating the lambda.
*/
template<typename LambdaType>
inline ScopeExit<LambdaType> operator+(const DummyStruct&, LambdaType&& lambda)
{
   return ScopeExit<LambdaType>{ std::forward<LambdaType>(lambda) };
}

/**
* Concatenate preprocessor tokens A and B without expanding macro definitions
* (however, if invoked from a macro, macro arguments are expanded).
*
* @see http://stackoverflow.com/a/5256500/694056
*/
#define PPCAT_NX(A, B) A ## B

/**
* Concatenate preprocessor tokens A and B after macro-expanding them.
*/
#define PPCAT(A, B) PPCAT_NX(A, B)

#define ON_SCOPE_EXIT \
   auto PPCAT(scope_exit_, __LINE__) = DummyStruct{ } + [=] ()