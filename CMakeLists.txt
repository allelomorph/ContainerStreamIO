# newest features used: FetchContent v3.11, FetchContent_MakeAvailable v3.14
cmake_minimum_required(VERSION 3.14)

project(ConatainerStreamIO LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  add_compile_options(/W4 /bigobj)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# note: Debug/Release modes can be used with -DCMAKE_BUILD_TYPE=(Debug|Release)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wall -Wextra -Wpedantic -Werror)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og --coverage")
endif()

# get list of compiler supported C++ standards as list of year suffixes,
#   minus C++98, which is not supported by project source
set(targetable_cxx_stds ${CMAKE_CXX_COMPILE_FEATURES})
list(FILTER targetable_cxx_stds INCLUDE REGEX cxx_std_..)
list(TRANSFORM targetable_cxx_stds REPLACE "^cxx_std_" "")
list(GET targetable_cxx_stds 0 first_cxx_std)
if (first_cxx_std EQUAL 98)
  list(POP_FRONT targetable_cxx_stds)
endif()
list(GET targetable_cxx_stds 0 first_cxx_std)
if (first_cxx_std GREATER 17)
  message(FATAL_ERROR "Compiler backwards compatibility to at least C++17 required to build ${PROJECT_NAME}")
endif()

# Original build intent:
#   - update to use of modern (at time of writing, v3.4.0) Catch2, commit hash
#       6e79e682b726f524310d55dec8ddac4e9c52fb5f
#   - limit build times by only building Catch2 and unit tests once
#   - build and test project code in every supported standard from C++11 onward
# Challenges:
#   - Catch2 v3 requires at least C++14 to build, so cmake will require any
#       target that links it to be compiled in C++14 regardless of other options
#   - after some research still cannot find a way, at least with FetchContent/
#       ExternalProject, to have two versions of the same dependency without a
#       naming conflict
#   - project is header-only by design, and so its compilation cannot be
#       separated from unit tests
# Current compromise:
#   - update to latest version of Catch2 v2 at time of writing, which supports
#       C++11, and use this version accross build process
#   - allow recompilation of unit tests for every C++ standard supported by
#       project header, despite cost in overall build time
Include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  # 2.13.10
  GIT_TAG        182c910b4b63ff587a3440e08f84f70497e49a81
  )
set(CATCH_VERSION_MAJOR 2)
# enables Catch::Catch2WithMain build in v2 (must be CACHE (BOOL|STATIC) due to CMP0077)
set(CATCH_BUILD_STATIC_LIBRARY ON CACHE BOOL
  "enables static build of Catch2 v2, including Catch::Catch2WithMain")
FetchContent_MakeAvailable(Catch2)

set(SOURCES
  ${CMAKE_SOURCE_DIR}/tests/unit_tests.cpp
  ${CMAKE_SOURCE_DIR}/source/container_stream_io.hh
  )

macro(setupTestsTarget cxx_std catch_version_major)
  set(NEW_TGT cpp${cxx_std}_tests)
  add_executable(${NEW_TGT} ${SOURCES})
  # eg with gcc and cxx_std of 17, uses `--std=c++17` instead of `--std=gnu++17`
  set_target_properties(${NEW_TGT} PROPERTIES
    CXX_STANDARD ${cxx_std}
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    )
  target_link_libraries(${NEW_TGT} PRIVATE Catch2::Catch2WithMain)
  target_include_directories(${NEW_TGT}
    PUBLIC ${CMAKE_SOURCE_DIR}/source
    )
  # underscore prefix avoids shadowing CATCH_VERSION_MAJOR defined later by Catch
  target_compile_definitions(${NEW_TGT}
    PUBLIC _CATCH_VERSION_MAJOR=${catch_version_major}
    )
  #[[
  # TBD can we add a similar forced pass of static checkers? and make this not bash syntax dependent?
  # requires passing tests for build success, see https://stackoverflow.com/a/44310094
  add_custom_command(
    OUTPUT ${NEW_TGT}_passed
    COMMAND ./${NEW_TGT} > ${NEW_TGT}_output 2>&1 || (cat ${NEW_TGT}_output && false)
    COMMAND cmake -E touch ${NEW_TGT}_passed
    DEPENDS ${NEW_TGT}
    USES_TERMINAL
    )
  add_custom_target(${NEW_TGT}_run
    ALL
    DEPENDS ${NEW_TGT}_passed
    )
  ]]
endmacro()

foreach(CXX_STD ${targetable_cxx_stds})
  setupTestsTarget(${CXX_STD} ${CATCH_VERSION_MAJOR})
endforeach()
