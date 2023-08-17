cmake_minimum_required(VERSION 3.10)

enable_language(CXX)

project(ContainerPrinter)

if (UNIX)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Werror -Wpedantic --coverage")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -Wextra -Werror -Wpedantic")
endif (UNIX)

if (WIN32)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W4 /bigobj")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /W4 /bigobj")
endif (WIN32)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE ON)

set(SOURCES
    tests/unit_tests.cpp
    source/container_printer.h)

set(SOURCE_DIR
    source)

set(THIRD_PARTY
    third-party/catch2/single_include)

add_executable(tests ${SOURCES})

target_include_directories(tests PUBLIC ${SOURCE_DIR} ${THIRD_PARTY})

set_target_properties(tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

if (UNIX)
    target_link_libraries(tests stdc++)
endif (UNIX)