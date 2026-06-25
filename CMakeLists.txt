cmake_minimum_required(VERSION 3.16)

project(caustin)
enable_testing()

add_executable(
	caustin
	src/main.c
	src/file_utils.c
	src/matcher.c
	src/string_list.c
)

set_property(TARGET caustin PROPERTY C_STANDARD 11)
set_property(TARGET caustin PROPERTY C_STANDARD_REQUIRED ON)
set_property(TARGET caustin PROPERTY C_EXTENSIONS OFF)

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
	target_compile_options(caustin PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_test(NAME caustin_functional COMMAND ${CMAKE_SOURCE_DIR}/tests/run_checks.sh)
