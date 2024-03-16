cmake_minimum_required(VERSION 3.0.0)
project(GraphTest VERSION 0.1.0 LANGUAGES C CXX)
project(GraphDebugger VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()


add_executable(GraphTest main.cpp)
add_library(GraphDebugger SHARED 
    src/glad.c
    src/Shader.cpp
    src/ShaderProgram.cpp
    src/VertexArray.cpp
    src/Window.cpp
    src/GraphTab.cpp
    src/Graph.cpp
    src/utils.cpp
    src/GraphArrangement.cpp
)


target_compile_options(GraphDebugger PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wformat=2 -Wconversion -Wunused-parameter -std=c++20)

add_subdirectory(glfw)
target_link_libraries(GraphDebugger PUBLIC glfw)
target_link_libraries(GraphTest PUBLIC GraphDebugger)
include_directories(include)
include_directories(${CMAKE_CURRENT_LIST_DIR})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
