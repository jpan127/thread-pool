cmake_minimum_required(VERSION 3.9)

project(thread_pool)

# Includes
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -Wall -Werror -Wextra $ENV{STDLIB}")

# Lib
file(GLOB SOURCES "src/*.cpp")
add_library(thread_pool SHARED ${SOURCES})
target_link_libraries(thread_pool pthread)

# Tests
file(GLOB TESTS "test/*.cpp")
add_executable(tests ${TESTS})
target_link_libraries(tests thread_pool)
target_include_directories(tests PUBLIC
    ${PROJECT_SOURCE_DIR}/modules/catch2
    ${PROJECT_SOURCE_DIR}/test
)
