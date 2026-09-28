#pragma once
#if !defined(_MSC_VER)
# if defined(__i386__)
#  define __fastcall __attribute__((fastcall))
#  define __stdcall __attribute__((stdcall))
#  define __cdecl __attribute__((cdecl))
#  define __thiscall __attribute__((thiscall))
# else
#  define __fastcall
#  define __stdcall
#  define __cdecl
#  define __thiscall
# endif
#endif

#if defined(_MSC_VER)
#define KINOKO_NOINLINE __declspec(noinline)
#else
#define KINOKO_NOINLINE __attribute__((noinline))
#endif

#if defined(_MSC_VER)
#include <intrin.h>
#define KINOKO_RETURN_ADDRESS() _ReturnAddress()
#else
#define KINOKO_RETURN_ADDRESS() __builtin_return_address(0)
#endif
