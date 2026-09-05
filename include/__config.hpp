#pragma once

#include <__autoconfig.hpp>

#ifndef __has_feature
#  define __has_feature(x) 0
#endif

#if __has_feature(cxx_rtti)
#  define _STD_HAS_RTTI 1
#else
#  define _STD_HAS_RTTI 0
#endif

#if __has_feature(cxx_exceptions)
#  define _STD_HAS_EH 1
#else
#  define _STD_HAS_EH 0
#endif

#ifdef __cplusplus

void __ignore_all_args(auto&&...);

#  if _STD_HAS_EH
#    define _TRY        try
#    define _CATCH(...) catch (__VA_ARGS__)
#    define _CATCHALL   catch (...)
#    define _THROW(...) throw __VA_ARGS__
#  else
#    define _TRY        if constexpr (true)
#    define _CATCH(...) else if constexpr (void(__VA_ARGS__), false)
#    define _CATCHALL   else if constexpr (false)
// TODO We should use terminate()
#    define _THROW(...) __ignore_all_args(__VA_ARGS__), _Exit(1)
#  endif

#  define _DELETE_NO_EXCEPTIONS delete ("Building without exception support")

namespace std {
namespace __detail {}
namespace ranges {
namespace __detail {
using namespace ::std::__detail;
}
}  // namespace ranges
}  // namespace std

#  define _STD_RETURN(...)                                                                         \
      noexcept(noexcept(__VA_ARGS__))->decltype(__VA_ARGS__) requires requires { __VA_ARGS__; } {  \
          return __VA_ARGS__;                                                                      \
      }

#  include <version>

#endif  // __cplusplus
