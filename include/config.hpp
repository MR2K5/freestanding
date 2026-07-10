#pragma once

#ifdef __cplusplus

#  ifdef _STD_HAS_EXCEPTIONS
#    define _TRY        try
#    define _CATCH(...) catch (__VA_ARGS__)
#    define _CATCHALL   catch (...)
#    define _THROW(...) throw __VA_ARGS__
#  else
#    define _TRY        if constexpr (true)
#    define _CATCH(...) else if constexpr (void(__VA_ARGS__), false)
#    define _CATCHALL   else if constexpr (false)
#    define _THROW(...) void(__VA_ARGS__), _Exit(1)
#  endif

#define _DELETE_NO_EXCEPTIONS delete("Building without exception support")

#endif  // __cplusplus
