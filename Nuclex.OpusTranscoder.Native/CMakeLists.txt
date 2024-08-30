#!/usr/bin/cmake
cmake_minimum_required (VERSION 3.8)

# -------------------------------------------------------------------------------------------------

project(
  NuclexOpusTranscoderNative
  VERSION 1.0.0
  DESCRIPTION "Converts audio to Opus and fixes clipping caused by the conversion"
)

option(
  BUILD_DOCS
  "Whether to generate documentation via Doxygen"
  OFF
)

option(
  BUILD_UNIT_TESTS
  "Whether to build the unit test executable. This will require an extra \
  compilation of the entire source tree as well as the GoogleTest library."
  ON
)

option(
  BUILD_BENCHMARK
  "Whether to build the benchmark executable. This will require an extra \
  compilation of the entire source tree as well as the Celero library."
  OFF
)

# -------------------------------------------------------------------------------------------------

# Qt: automatically run the "Meta-Object Compiler" which reads C++ header files and
# generates additional code from Qt's C++ extensions.
#   https://doc.qt.io/qt-5/moc.html
set(CMAKE_AUTOMOC ON)

# Qt: automatically run the "User Interface Compiler" on user interface definition
# files (.ui) and generate matching C++ header files.
#   https://doc.qt.io/qt-5/uic.html
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS UserInterface)

# Qt: automatically run the "Resource Compiler" on Qt resource files (.qrc) to read
# all referenced resources and generate C++ sources storing the file contents.
#   https://doc.qt.io/qt-5/rcc.html
set(CMAKE_AUTORCC ON)

# find includes in the corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# This sets a bunch of compile flags and defined ${NUCLEX_COMPILER_TAG} to
# say something like linux-gcc-13.2-amd64-debug. You should have this directory
# if you do a full clone of a project that is using this third-party library build.
include("../build-system/cmake/cplusplus.cmake")

# The Unix build pipeline doesn't automatically include threading, so search for
# the pthreads library in order to link against it later on.
#   https://en.wikipedia.org/wiki/Pthreads
find_package(Threads REQUIRED)

# Locate Qt, the cross-platform User Interface and base API abstraction library
# we're using for all UI stuff
find_package(Qt5 COMPONENTS REQUIRED Widgets Sql)

# Add Nuclex.Support.Native as a sub-project, we link it for utility methods.
if(NOT (TARGET NuclexSupportNative))
  add_subdirectory(
    ${PROJECT_SOURCE_DIR}/../Nuclex.Support.Native
    ${CMAKE_BINARY_DIR}/NuclexSupportNative
  )
endif()

# Add Nuclex.Audio.Native as a sub-project, we link it for audio processing
if(NOT (TARGET NuclexAudioNative))
  add_subdirectory(
    ${PROJECT_SOURCE_DIR}/../Nuclex.Audio.Native
    ${CMAKE_BINARY_DIR}/NuclexAudioNative
  )
endif()

# Add libsndfile as a sub-project, we use it to read audio files
if(NOT (TARGET SndFile))
  add_subdirectory(
    ${PROJECT_SOURCE_DIR}/../third-party/nuclex-sndfile
    ${CMAKE_BINARY_DIR}/nuclex-sndfile
  )
endif()

# Add WavPack as a sub-project, we use it to read WavPack audio files
if(NOT (TARGET WavPack))
  add_subdirectory(
    ${PROJECT_SOURCE_DIR}/../third-party/nuclex-wavpack
    ${CMAKE_BINARY_DIR}/nuclex-wavpack
  )
endif()

# Add OPUS as a sub-project, we use it to encode OPUS audio files
if(NOT (TARGET Opus))
  add_subdirectory(
    ${PROJECT_SOURCE_DIR}/../third-party/nuclex-opus
    ${CMAKE_BINARY_DIR}/nuclex-opus
  )
endif()

# Add OpusFile as a sub-project, we use it to encode OPUS audio files
if(NOT (TARGET OpusFile))
  add_subdirectory(
    ${PROJECT_SOURCE_DIR}/../third-party/nuclex-opusfile
    ${CMAKE_BINARY_DIR}/nuclex-opusfile
  )
endif()

message(STATUS "Enabled options for Nuclex.OpusTranscoder.Native:")
message(STATUS "  ⚫ Build core library")

if(BUILD_UNIT_TESTS)
  message(STATUS "  ⚫ Build unit tests")

  # Add GoogleTest as a sub-project so we can link our unit test executable
  if(NOT (TARGET GoogleTest))
    add_subdirectory(
      ${PROJECT_SOURCE_DIR}/../third-party/nuclex-googletest
      ${CMAKE_BINARY_DIR}/nuclex-googletest
    )
  endif()
endif()

if(BUILD_BENCHMARK)
  message(STATUS "  ⚫ Build benchmark")

  # Add Celero as a sub-project so we can link our benchmark executable
  if(NOT (TARGET Celero))
    add_subdirectory(
      ${PROJECT_SOURCE_DIR}/../third-party/nuclex-celero
      ${CMAKE_BINARY_DIR}/nuclex-celero
    )
  endif()
endif()

# Use CMake's own package for locating Doxygen on the system
if(BUILD_DOCS)
  find_package(Doxygen)
endif()

# -------------------------------------------------------------------------------------------------

# Project structure
#
#   ProjectName/
#     Source/                   All source files, using deeper directories as needed
#     Include/ProjectName/      All public headers, using deeper directories as needed
#     Tests/                    All unit tests, using deeper directories as needed
#     Benchmarks/               All benchmark files, using deeper directories as needed
#
# CMake documentation:
#   |  Note: We do not recommend using GLOB to collect a list of
#   |  source files from your source tree. If no CMakeLists.txt file
#   |  changes when a source is added or removed then the generated
#   |  build system cannot know when to ask CMake to regenerate.
#
# As so very often, CMake becomes a hurdle rather than helping.
# I'm not going to manually maintain a list of source files. Rebuilds
# where files are added, removed or renamed need to be from scratch.
#
file(
  GLOB_RECURSE userInterfaceFiles
  CONFIGURE_DEPENDS
  "UserInterface/*.*"
)
file(
  GLOB_RECURSE sourceFiles
  CONFIGURE_DEPENDS
  "Source/*.cpp"
  "Source/*.c"
)
file(
  GLOB_RECURSE headerFiles
  CONFIGURE_DEPENDS
  "Include/Nuclex/OpusTranscoder/*.h"
)
file(
  GLOB_RECURSE unittestFiles
  CONFIGURE_DEPENDS
  "Tests/*.cpp"
)
file(
  GLOB_RECURSE benchmarkFiles
  CONFIGURE_DEPENDS
  "Benchmarks/*.cpp"
)

# -------------------------------------------------------------------------------------------------

function(add_third_party_libraries target_name)

  target_link_libraries(
    ${target_name}
    PUBLIC NuclexSupportNative
    PUBLIC NuclexAudioNative
    PRIVATE SndFile
    PRIVATE WavPack
    PRIVATE Opus
    PRIVATE Qt5::Widgets
    PRIVATE Qt5::Sql
    PRIVATE Threads::Threads

    PRIVATE ogg
    PRIVATE vorbis
    PRIVATE FLAC
  )

  # On Unix systems, the application and unit test executable should look for
  # dependencies in its own directory first.
  set_target_properties(
    ${target_name} PROPERTIES
    BUILD_RPATH_USE_ORIGIN ON
    BUILD_WITH_INSTALL_RPATH ON
    INSTALL_RPATH_USE_LINK_PATH OFF
    INSTALL_RPATH "\${ORIGIN}"
  )

endfunction()

# -------------------------------------------------------------------------------------------------

if(BUILD_UNIT_TESTS)

  # Executable that runs the unit tests (main() supplied by GoogleTest)
  add_executable(NuclexOpusTranscoderNativeTests)

  # Enable compiler warnings only if this library is compiles on its own.
  # If it's used as a sub-project, the including project's developers aren't
  # interested in seeing warnings from a project they're not maintaining.
  if(${CMAKE_PROJECT_NAME} STREQUAL "NuclexOpusTranscoderNative")
    enable_target_compiler_warnings(NuclexOpusTranscoderNativeTests)
  else()
    disable_target_compiler_warnings(NuclexOpusTranscoderNativeTests)
  endif()

  # Let the code know it's not being compiled into a shared library
  # (this disables visibility/exports, thus allowing the compiler detect
  # additional unused code and warn about it)
  target_compile_definitions(
    NuclexOpusTranscoderNativeTests
    PRIVATE NUCLEX_OPUSTRANSCODER_EXECUTABLE
    PRIVATE NUCLEX_OPUSTRANSCODER_UNIT_TEST_EXECUTABLE
  )

  # Add directory with public headers to include path
  target_include_directories(
    NuclexOpusTranscoderNativeTests
    PUBLIC "Include"
  )

  # Add public headers and sources (normal + unit tests) to compilation list
  # (headers, too, in case CMake is used to generate an IDE project)
  target_sources(
    NuclexOpusTranscoderNativeTests
    PRIVATE ${headerFiles}
    PRIVATE ${sourceFiles}
    PRIVATE ${unittestFiles}
  )

  # Link GoogleTest and the main() function supplied by GoogleTest
  # Also link against PThreads
  target_link_libraries(
    NuclexOpusTranscoderNativeTests
    PRIVATE GoogleTest::Static
    PRIVATE GoogleTest::Main
  )

  add_third_party_libraries(NuclexOpusTranscoderNativeTests)

  # On Windows, we want the executable to be named Nuclex.OpusTranscoder.Native.Tests.exe
  if(WIN32)
    set_target_properties(
      NuclexOpusTranscoderNativeTests
      PROPERTIES OUTPUT_NAME "Nuclex.OpusTranscoder.Native.Tests"
    )
  endif()

endif() # if BUILD_UNIT_TESTS enabled

# -------------------------------------------------------------------------------------------------

# name of the .exe file, window flag and the list of things to compile
add_executable(NuclexOpusTranscoderNative)

# Enable compiler warnings only if this application is compiled on its own.
# If it's used as a sub-project, the including project's developers aren't
# interested in seeing warnings from a project they're not maintaining.
if(${CMAKE_PROJECT_NAME} STREQUAL "NuclexOpusTranscoderNative")
  enable_target_compiler_warnings(NuclexOpusTranscoderNative)
else()
  disable_target_compiler_warnings(NuclexOpusTranscoderNative)
endif()

# Add directory with public headers to include path
target_include_directories(
  NuclexOpusTranscoderNative
  PUBLIC "Include"
)

qt5_add_resources(QRC_FILES Resources/OpusTranscoder.qrc)

# Add public headers and sources to compilation list
# (headers, too, in case CMake is used to generate an IDE project)
target_sources(
  NuclexOpusTranscoderNative
  PUBLIC ${headerFiles}
  PRIVATE ${sourceFiles}
  PRIVATE ${userInterfaceFiles}
  PRIVATE ${QRC_FILES}
)

# Add include directories and static libraries the application depends on
add_third_party_libraries(NuclexOpusTranscoderNative)

# -------------------------------------------------------------------------------------------------

set_property(GLOBAL PROPERTY QUIET_INSTALL ON)

#file(
#	COPY ${PROJECT_SOURCE_DIR}/OpusTranscoder.ini
#	DESTINATION ${PROJECT_SOURCE_DIR}/bin/${NUCLEX_COMPILER_TAG}/
#)

# Install the executable into a subdirectory of this CMakeLists.txt file
# under ./bin/linux-gcc9.3-amd64-debug/ (the second-level directory is called
# "compiler tag" and dynamically formed -- it ensures that when linking
# a pre-compiled shared library, the correct library is used).
install(
  TARGETS NuclexOpusTranscoderNative
  ARCHIVE DESTINATION ${PROJECT_SOURCE_DIR}/bin/${NUCLEX_COMPILER_TAG}
  LIBRARY DESTINATION ${PROJECT_SOURCE_DIR}/bin/${NUCLEX_COMPILER_TAG}
  RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/bin/${NUCLEX_COMPILER_TAG}
)

# Install .pdb files on Windows platforms for the main application
install_debug_symbols(NuclexOpusTranscoderNative)

# Install unit tests in same location as shared library.
if(BUILD_UNIT_TESTS)
  install(
    TARGETS NuclexOpusTranscoderNativeTests
    RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/bin/${NUCLEX_COMPILER_TAG}
  )

  # Install .pdb files on Windows platforms for the unit tests, too.
  install_debug_symbols(NuclexOpusTranscoderNativeTests)
endif()

# Do the same for Nuclex.Support.Native. Since we depend on this library
# and have set the rpath accordingly, it needs to be in the same directory
install(
  TARGETS NuclexSupportNative
  ARCHIVE DESTINATION ${PROJECT_SOURCE_DIR}/bin/${NUCLEX_COMPILER_TAG}
  LIBRARY DESTINATION ${PROJECT_SOURCE_DIR}/bin/${NUCLEX_COMPILER_TAG}
  RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/bin/${NUCLEX_COMPILER_TAG}
)

# Do the same for Nuclex.Audio.Native. Since we depend on this library
# and have set the rpath accordingly, it needs to be in the same directory
install(
  TARGETS NuclexAudioNative
  ARCHIVE DESTINATION ${PROJECT_SOURCE_DIR}/bin/${NUCLEX_COMPILER_TAG}
  LIBRARY DESTINATION ${PROJECT_SOURCE_DIR}/bin/${NUCLEX_COMPILER_TAG}
  RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/bin/${NUCLEX_COMPILER_TAG}
)

# -------------------------------------------------------------------------------------------------

if(BUILD_DOCS)

  if(NOT DOXYGEN_FOUND)
    message(FATAL_ERROR "Can't build documentation because Doxygen was not found")
  endif()

  add_custom_target(
    NuclexOpusTranscoderNativeDocs ALL
    COMMAND ${DOXYGEN_EXECUTABLE} "Nuclex.OpusTranscoder.Native.doxygen.cfg"
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  )

endif()

# -------------------------------------------------------------------------------------------------
