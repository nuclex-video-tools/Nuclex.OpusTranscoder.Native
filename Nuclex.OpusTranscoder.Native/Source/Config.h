#pragma region Apache License 2.0
/*
Nuclex Opus Transcoder
Copyright (C) 2024 Markus Ewald / Nuclex Development Labs

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
#pragma endregion // Apache License 2.0

#ifndef NUCLEX_OPUSTRANSCODER_CONFIG_H
#define NUCLEX_OPUSTRANSCODER_CONFIG_H

// --------------------------------------------------------------------------------------------- //

// Platform recognition
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
  #error The Nuclex.OpusTranscoder.Native application does not support WinRT
#elif defined(WIN32) || defined(_WIN32)
  #define NUCLEX_OPUSTRANSCODER_WINDOWS 1
#else
  #define NUCLEX_OPUSTRANSCODER_LINUX 1
#endif

// --------------------------------------------------------------------------------------------- //

// Compiler support checking
#if defined(_MSC_VER)
  #if (_MSC_VER < 1910) // Visual Studio 2017 has the C++17 features we use
    #error At least Visual Studio 2017 is required to compile Nuclex.OpusTranscoder.Native
  #elif defined(_MSVC_LANG) && (_MSVC_LANG >= 201703L)
    #define NUCLEX_OPUSTRANSCODER_CXX17 1
  #endif
#elif defined(__clang__) && defined(__clang_major__)
  #if (__clang_major__ < 5) // clang 5.0 has the C++17 features we use
    #error At least clang 5.0 is required to compile Nuclex.OpusTranscoder.Native
  #elif defined(__cplusplus) && (__cplusplus >= 201703L)
    #define NUCLEX_OPUSTRANSCODER_CXX17 1
  #endif
#elif defined(__GNUC__)
  #if (__GNUC__ < 8) // GCC 8.0 has the C++17 features we use
    #error At least GCC 8.0 is required to compile Nuclex.OpusTranscoder.Native
  #elif defined(__cplusplus) && (__cplusplus >= 201703L)
    #define NUCLEX_OPUSTRANSCODER_CXX17 1
  #endif
#else
  #error Unknown compiler. Nuclex.OpusTranscoder.Native is tested with GCC, clang and MSVC only
#endif

// This library uses writable std::string::data(), 'if constexpr' and new C++17
// containers, so anything earlier than C++ 17 will only result in compilation errors.
#if !defined(NUCLEX_OPUSTRANSCODER_CXX17)
  #error The Nuclex.OpusTranscoder.Native application must be compiled in at least C++17 mode
#endif

// We've got tons of u8"hello" strings that will become char8_t in C++20 and fail to build!
// Bail out instead of letting the user scratch their head over weird compiler errors.
#if defined(_MSVC_LANG) && (_MSVC_LANG >= 202002)
  #error The Nuclex.OpusTranscoder.Native application does not work in C++20 mode yet
#elif defined(__cplusplus) && (__cplusplus >= 202002)
  #error The Nuclex.OpusTranscoder.Native application does not work in C++20 mode yet
#endif

// --------------------------------------------------------------------------------------------- //

// Endianness detection
#if defined(_MSC_VER) // MSVC is always little endian, including Windows on ARM
  #define NUCLEX_OPUSTRANSCODER_LITTLE_ENDIAN 1
#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) // GCC
  #define NUCLEX_OPUSTRANSCODER_LITTLE_ENDIAN 1
#elif defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__) // GCC
  #define NUCLEX_OPUSTRANSCODER_BIG_ENDIAN 1
#else
  #error Could not determine whether platform is big or little endian
#endif

// --------------------------------------------------------------------------------------------- //

// Decides whether symbols are imported from a dll (client app) or exported to
// a dll (Nuclex.OpusTranscoder.Native application). The NUCLEX_OPUSTRANSCODER_SOURCE symbol
// is defined by all source files of the application, so you don't have to worry about a thing.
#if defined(_MSC_VER)

  #if defined(NUCLEX_OPUSTRANSCODER_STATICLIB) || defined(NUCLEX_OPUSTRANSCODER_EXECUTABLE)
    #define NUCLEX_OPUSTRANSCODER_API
  #else
    #if defined(NUCLEX_OPUSTRANSCODER_SOURCE)
      // If we are building the DLL, export the symbols tagged like this
      #define NUCLEX_OPUSTRANSCODER_API __declspec(dllexport)
    #else
      // If we are consuming the DLL, import the symbols tagged like this
      #define NUCLEX_OPUSTRANSCODER_API __declspec(dllimport)
    #endif
  #endif
  // MSVC doesn't have to import/export the whole type, it includes the vtable
  // and RTTI stuff automatically when at least one member is exported. Exporting
  // the whole type would only cause useless things to be exported as well.
  #define NUCLEX_OPUSTRANSCODER_TYPE

#elif defined(__GNUC__) || defined(__clang__)

  #if defined(NUCLEX_OPUSTRANSCODER_STATICLIB) || defined(NUCLEX_OPUSTRANSCODER_EXECUTABLE)
    #define NUCLEX_OPUSTRANSCODER_API
    #define NUCLEX_OPUSTRANSCODER_TYPE
  #else
    // If you use -fvisibility=hidden in GCC, exception handling and RTTI would break
    // if visibility wasn't set during export _and_ import because GCC would immediately
    // forget all type infos encountered. See http://gcc.gnu.org/wiki/Visibility
    #define NUCLEX_OPUSTRANSCODER_API __attribute__ ((visibility ("default")))
    #define NUCLEX_OPUSTRANSCODER_TYPE __attribute__ ((visibility ("default")))
  #endif

#else

  #error Unknown compiler, please implement shared library macros for your system

#endif

// --------------------------------------------------------------------------------------------- //

// Optimization macros
#if defined(__GNUC__) || defined(__clang__)

  #if !defined(likely)
    #define likely(x) __builtin_expect((x), 1)
  #endif
  #if !defined(unlikely)
    #define unlikely(x) __builtin_expect((x), 0)
  #endif

#else

  #if !defined(likely)
    #define likely(x) (x)
  #endif
  #if !defined(unlikely)
    #define unlikely(x) (x)
  #endif

#endif

// --------------------------------------------------------------------------------------------- //

// Silences unused variable warning, but only in release builds
// (NDEBUG is also what decides whether the assert() macro does anything)
#if defined(NDEBUG)
  #define NUCLEX_OPUSTRANSCODER_NDEBUG_UNUSED(x) (void)x
#else
  #define NUCLEX_OPUSTRANSCODER_NDEBUG_UNUSED(x) {}
#endif

// --------------------------------------------------------------------------------------------- //

#if defined(_MSC_VER)
  #define NUCLEX_OPUSTRANSCODER_CPU_YIELD _mm_pause()
#elif defined(__arm__)
  #define NUCLEX_OPUSTRANSCODER_CPU_YIELD asm volatile("yield")
#else
  #define NUCLEX_OPUSTRANSCODER_CPU_YIELD __builtin_ia32_pause()
#endif

// --------------------------------------------------------------------------------------------- //

#endif // NUCLEX_OPUSTRANSCODER_CONFIG_H
