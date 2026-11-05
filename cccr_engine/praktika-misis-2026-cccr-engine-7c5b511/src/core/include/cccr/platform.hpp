#pragma once

/// @brief Marks a symbol as part of the shared-library public API when built
///        as a DLL / shared object.
#ifdef CCCR_PLATFORM_WINDOWS
#ifdef CCCR_BUILD_SHARED
#define CCCR_API __declspec(dllexport)
#else
#define CCCR_API __declspec(dllimport)
#endif
#elif defined(CCCR_PLATFORM_LINUX)
#ifdef CCCR_BUILD_SHARED
#define CCCR_API __attribute__((visibility("default")))
#else
#define CCCR_API
#endif
#else
#define CCCR_API
#endif

/// @brief Always exports a symbol from a shared library (visibility default).
#ifdef CCCR_PLATFORM_WINDOWS
#define CCCR_EXPORT __declspec(dllexport)
#elif defined(CCCR_PLATFORM_LINUX)
#define CCCR_EXPORT __attribute__((visibility("default")))
#else
#define CCCR_EXPORT
#endif

// TODO: Replace with std::breakpoint() from <debugging> in C++26
// https://en.cppreference.com/w/cpp/utility/breakpoint.html

// NOLINTBEGIN(hicpp-no-assembler)

/// @brief Triggers a debugger breakpoint on the current platform.
#if defined(_MSC_VER) && (_MSC_VER >= 1300)
#define CCCR_DEBUG_BREAK() __debugbreak()

#elif defined(__arm64__) && defined(__APPLE__)
#include <unistd.h>
#include <csignal>
#define CCCR_DEBUG_BREAK() ::kill(::getpid(), SIGINT)

#elif defined(__arm64__) && (defined(__GNUC__) || defined(__clang__))
#define CCCR_DEBUG_BREAK() __asm__("brk 0")

#elif defined(__arm__) && defined(__APPLE__)
#define CCCR_DEBUG_BREAK() __asm__("trap")

#elif defined(__arm__) && (defined(__GNUC__) || defined(__clang__))
#define CCCR_DEBUG_BREAK() __asm__("bkpt 0")

#elif defined(__arm__) && defined(__ARMCC_VERSION)
#define CCCR_DEBUG_BREAK() __breakpoint(0)

#elif (defined(__x86_64__) || defined(__i386__)) && \
    (defined(__GNUC__) || defined(__clang__))
#define CCCR_DEBUG_BREAK() __asm__("int3")

#elif (defined(_M_X64) || defined(_M_IX86)) && defined(_MSC_VER)
#define CCCR_DEBUG_BREAK() __asm int 3

#elif defined(__powerpc__) || defined(__ppc__) || defined(_ARCH_PPC)
#define CCCR_DEBUG_BREAK() __asm__(".long 0")

#elif defined(__riscv) || defined(__riscv__)
#define CCCR_DEBUG_BREAK() __asm__("ebreak")

#elif defined(__wasm__)
#define CCCR_DEBUG_BREAK() __asm__("unreachable")

#elif defined(__has_builtin) && __has_builtin(__builtin_trap)
#define CCCR_DEBUG_BREAK() __builtin_trap()

#else
#include <csignal>
#define CCCR_DEBUG_BREAK() ::std::raise(SIGTRAP)
#endif

// NOLINTEND(hicpp-no-assembler)
