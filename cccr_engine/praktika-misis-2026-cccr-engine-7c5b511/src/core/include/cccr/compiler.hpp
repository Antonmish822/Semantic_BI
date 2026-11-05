#pragma once

#include <version>

#if defined(__GNUC__) || defined(__clang__)
/// @brief Branch-prediction hint that @p x is likely true.
/// @param x Boolean expression
#define CCCR_EXPECT_TRUE(x) __builtin_expect(!!(x), 1)
/// @brief Branch-prediction hint that @p x is likely false.
/// @param x Boolean expression
#define CCCR_EXPECT_FALSE(x) __builtin_expect(!!(x), 0)
#else
/// @brief Branch-prediction hint that @p x is likely true.
/// @param x Boolean expression
#define CCCR_EXPECT_TRUE(x) (x)
/// @brief Branch-prediction hint that @p x is likely false.
/// @param x Boolean expression
#define CCCR_EXPECT_FALSE(x) (x)
#endif

#if defined(__cpp_lib_move_only_function) && \
    __cpp_lib_move_only_function >= 202110L
/// @brief Defined when `std::move_only_function` is available.
#define CCCR_MOVEONLY_FUNCTION_AVAILABLE
#endif

#if defined(__cpp_lib_containers_ranges) && \
    __cpp_lib_containers_ranges >= 202202L
/// @brief Defined when container range constructors/inserts are available.
#define CCCR_CONTAINERS_RANGES_AVAILABLE
#endif

#if defined(__cpp_lib_flat_map) && __cpp_lib_flat_map >= 202207L
/// @brief Defined when `std::flat_map` is available.
#define CCCR_STL_FLAT_MAP_AVAILABLE
#elif defined(CCCR_USE_STL_FLAT_MAP)
/// @brief Defined when `std::flat_map` is available.
#define CCCR_STL_FLAT_MAP_AVAILABLE
#endif

#ifdef _MSC_VER
/// @brief Forces the following function to be inlined when supported.
#define CCCR_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
/// @brief Forces the following function to be inlined when supported.
#define CCCR_FORCE_INLINE __attribute__((always_inline)) inline
#else
/// @brief Forces the following function to be inlined when supported.
#define CCCR_FORCE_INLINE inline
#endif

#if defined(__clang__)
/// @brief Attribute requesting always-inline when supported.
#define CCCR_ALWAYS_INLINE [[clang::always_inline]]
#elif defined(__GNUC__)
/// @brief Attribute requesting always-inline when supported.
#define CCCR_ALWAYS_INLINE [[gnu::always_inline]]
#elif defined(_MSC_VER)
/// @brief Attribute requesting always-inline when supported.
#define CCCR_ALWAYS_INLINE [[msvc::forceinline]]
#else
/// @brief Attribute requesting always-inline when supported.
#define CCCR_ALWAYS_INLINE
#endif

#ifdef _MSC_VER
/// @brief Prevents the following function from being inlined when supported.
#define CCCR_NO_INLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
/// @brief Prevents the following function from being inlined when supported.
#define CCCR_NO_INLINE __attribute__((noinline))
#else
/// @brief Prevents the following function from being inlined when supported.
#define CCCR_NO_INLINE
#endif
