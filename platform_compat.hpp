#pragma once

// Cross-platform compatibility header for emphf
// Handles SSE availability and other architecture-specific features

// Architecture detection
#if defined(__x86_64__) || defined(_M_X64) || defined(__amd64__)
    #define EMPHF_ARCH_X86_64 1
    #define EMPHF_HAS_SSE 1
#elif defined(__aarch64__) || defined(_M_ARM64) || defined(__arm64__)
    #define EMPHF_ARCH_ARM64 1
    #define EMPHF_HAS_SSE 0
#else
    #define EMPHF_ARCH_UNKNOWN 1
    #define EMPHF_HAS_SSE 0
#endif

// Platform detection
#if defined(__APPLE__)
    #define EMPHF_PLATFORM_MACOS 1
#elif defined(__linux__)
    #define EMPHF_PLATFORM_LINUX 1
#elif defined(_WIN32)
    #define EMPHF_PLATFORM_WINDOWS 1
#else
    #define EMPHF_PLATFORM_UNKNOWN 1
#endif

// SSE headers - only include on x86_64
#if EMPHF_HAS_SSE
    #include <xmmintrin.h>
    #include <emmintrin.h>
    // Additional SSE headers can be included here as needed
#endif

// POPCOUNT support
#ifndef EMPHF_USE_POPCOUNT
    #if EMPHF_HAS_SSE
        #define EMPHF_USE_POPCOUNT 1
    #else
        #define EMPHF_USE_POPCOUNT 0
    #endif
#endif

// Memory alignment helpers
#if EMPHF_ARCH_X86_64
    #define EMPHF_MEMORY_ALIGNMENT 16  // SSE requires 16-byte alignment
#else
    #define EMPHF_MEMORY_ALIGNMENT 8   // Default to 8-byte alignment
#endif

// Compiler-specific attributes
#if defined(__GNUC__) || defined(__clang__)
    #define EMPHF_FORCE_INLINE __attribute__((always_inline)) inline
    #define EMPHF_LIKELY(x) __builtin_expect(!!(x), 1)
    #define EMPHF_UNLIKELY(x) __builtin_expect(!!(x), 0)
#elif defined(_MSC_VER)
    #define EMPHF_FORCE_INLINE __forceinline
    #define EMPHF_LIKELY(x) (x)
    #define EMPHF_UNLIKELY(x) (x)
#else
    #define EMPHF_FORCE_INLINE inline
    #define EMPHF_LIKELY(x) (x)
    #define EMPHF_UNLIKELY(x) (x)
#endif
