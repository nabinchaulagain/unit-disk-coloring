cmake_minimum_required(VERSION 3.16)
project(unit_disk_coloring LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(udg_coloring
    src/coloring.cpp
    src/graph_io.cpp
    src/mylnikov.cpp
    src/unit_disk_graph.cpp
)
target_include_directories(udg_coloring PUBLIC include)
target_compile_options(udg_coloring PRIVATE -Wall -Wextra -Wpedantic)

add_executable(dsatur src/main.cpp)
target_link_libraries(dsatur PRIVATE udg_coloring)
target_compile_options(dsatur PRIVATE -Wall -Wextra -Wpedantic)

add_executable(generate_udg src/generate_udg.cpp)
target_link_libraries(generate_udg PRIVATE udg_coloring)
target_compile_options(generate_udg PRIVATE -Wall -Wextra -Wpedantic)

add_executable(import_mylnikov src/import_mylnikov.cpp)
target_link_libraries(import_mylnikov PRIVATE udg_coloring)
target_compile_options(import_mylnikov PRIVATE -Wall -Wextra -Wpedantic)

enable_testing()
add_executable(coloring_tests tests/coloring_tests.cpp)
target_link_libraries(coloring_tests PRIVATE udg_coloring)
target_compile_options(coloring_tests PRIVATE -Wall -Wextra -Wpedantic)
add_test(NAME coloring_tests COMMAND coloring_tests)
