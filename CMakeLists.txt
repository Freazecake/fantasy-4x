cmake_minimum_required(VERSION 3.14)
project("game")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SOURCES 
    "src/*.cpp" 
    "include/*.hpp"
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE 
    include
    src
    assets
)

enable_testing()
add_subdirectory(tests)